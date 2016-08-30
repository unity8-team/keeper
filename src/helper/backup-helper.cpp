/*
 * Copyright (C) 2016 Canonical, Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3, as published
 * by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranties of
 * MERCHANTABILITY, SATISFACTORY QUALITY, or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authors:
 *     Xavi Garcia <xavi.garcia.mena@canonical.com>
 *     Charles Kerr <charles.kerr@canonical.com>
 */

#include <helper/backup-helper.h>
#include <service/app-const.h> // HELPER_TYPE

#include <QByteArray>
#include <QDebug>
#include <QLocalSocket>
#include <QMap>
#include <QObject>
#include <QScopedPointer>
#include <QString>
#include <QTimer>
#include <QVector>

#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <functional> // std::bind()


class BackupHelperPrivate
{
public:

    explicit BackupHelperPrivate(
        BackupHelper* backup_helper
    )
        : q_ptr(backup_helper)
        , timer_(new QTimer())
        , storage_framework_socket_(new QLocalSocket())
        , helper_socket_(new QLocalSocket())
        , read_socket_(new QLocalSocket())
        , upload_buffer_{}
    {
        // listen for inactivity
        QObject::connect(timer_.data(), &QTimer::timeout,
            std::bind(&BackupHelperPrivate::on_inactivity_detected, this)
        );

        // listen for data ready to read
        QObject::connect(read_socket_.data(), &QLocalSocket::readyRead,
            std::bind(&BackupHelperPrivate::on_ready_read, this)
        );

        // fire up the sockets
        int fds[2];
        int rc = socketpair(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK, 0, fds);
        if (rc == -1)
        {
            // TODO throw exception.
            qWarning() << "BackupHelperPrivate: error creating socket pair to communicate with helper ";
            return;
        }

        // helper socket is for the client.
        helper_socket_->setSocketDescriptor(fds[1], QLocalSocket::ConnectedState, QIODevice::WriteOnly);

        read_socket_->setSocketDescriptor(fds[0], QLocalSocket::ConnectedState, QIODevice::ReadOnly);
    }

    ~BackupHelperPrivate() = default;

    Q_DISABLE_COPY(BackupHelperPrivate)

    void start(QStringList const& urls)
    {
        q_ptr->Helper::start(urls);
        reset_inactivity_timer();
    }

    void set_storage_framework_socket(std::shared_ptr<QLocalSocket> const& sf_socket)
    {
        n_read_ = 0;
        n_uploaded_ = 0;
        read_error_ = false;
        write_error_ = false;
        cancelled_ = false;

        storage_framework_socket_ = sf_socket;

        // listen for data uploaded
        storage_framework_socket_connection_.reset(
            new QMetaObject::Connection(
                QObject::connect(
                    storage_framework_socket_.get(), &QLocalSocket::bytesWritten,
                    std::bind(&BackupHelperPrivate::on_data_uploaded, this, std::placeholders::_1)
                )
            ),
            [](QMetaObject::Connection* c){
                QObject::disconnect(*c);
            }
        );

        // TODO xavi is going to remove this line
        q_ptr->set_state(Helper::State::STARTED);

        reset_inactivity_timer();

        storage_framework_socket_open_ = true;
    }

    void stop()
    {
        cancelled_ = true;
        q_ptr->Helper::stop();
    }

    void on_helper_process_stopped()
    {
        check_for_done();
        stop_inactivity_timer();
    }

    int get_helper_socket() const
    {
        return int(helper_socket_->socketDescriptor());
    }

    void on_storage_framework_finished()
    {
        qDebug() << "storage framework has finished for the current helper...";
        storage_framework_socket_open_ = false;
        check_for_done();
    }

    QString to_string(Helper::State state) const
    {
        return state == Helper::State::STARTED
            ? QStringLiteral("saving")
            : q_ptr->Helper::to_string(state);
    }

private:

    void on_inactivity_detected()
    {
        stop_inactivity_timer();
        qWarning() << "Inactivity detected in the helper...stopping it";
        stop();
    }

    void on_ready_read()
    {
        process_more();
    }

    void on_data_uploaded(qint64 n)
    {
        n_uploaded_ += n;
        q_ptr->record_data_transferred(n);
        qDebug("n_read %zu n_uploaded %zu (newly uploaded %zu)", size_t(n_read_), size_t(n_uploaded_), size_t(n));
        process_more();
        check_for_done();
    }

    void process_more()
    {
        char readbuf[UPLOAD_BUFFER_MAX_];
        for(;;)
        {
            // try to fill the upload buf
            int max_bytes = UPLOAD_BUFFER_MAX_ - upload_buffer_.size();
            if (max_bytes > 0) {
                const auto n = read_socket_->read(readbuf, max_bytes);
                if (n > 0) {
                    n_read_ += n;
                    upload_buffer_.append(readbuf, int(n));
                    qDebug("upload_buffer_.size() is %zu after reading %zu from helper", size_t(upload_buffer_.size()), size_t(n));
                }
                else if (n < 0) {
                    read_error_ = true;
                    stop();
                    return;
                }
            }

            // try to empty the upload buf
            const auto n = storage_framework_socket_->write(upload_buffer_);
            if (n > 0) {
                upload_buffer_.remove(0, int(n));
                qDebug("upload_buffer_.size() is %zu after writing %zu to cloud", size_t(upload_buffer_.size()), size_t(n));
                continue;
            }
            else {
                if (n < 0) {
                    write_error_ = true;
                    qWarning() << "Write error:" << storage_framework_socket_->errorString();
                    stop();
                }
                break;
            }
        }

        reset_inactivity_timer();
    }

    void reset_inactivity_timer()
    {
        static constexpr int MAX_TIME_WAITING_FOR_DATA {BackupHelper::MAX_INACTIVITY_TIME};
        timer_->start(MAX_TIME_WAITING_FOR_DATA);
    }

    void stop_inactivity_timer()
    {
        timer_->stop();
    }

    void check_for_done()
    {
        if (n_uploaded_ == q_ptr->expected_size())
        {
            if (storage_framework_socket_open_)
                q_ptr->set_state(Helper::State::DATA_COMPLETE);
            else
                q_ptr->set_state(Helper::State::COMPLETE);
        }
        else if (read_error_ || write_error_ || (n_uploaded_ > q_ptr->expected_size()))
        {
            q_ptr->set_state(Helper::State::FAILED);
        }
        else if (cancelled_)
        {
            q_ptr->set_state(Helper::State::CANCELLED);
        }
    }

    /***
    ****
    ***/

    static constexpr int UPLOAD_BUFFER_MAX_ {1024*16};

    BackupHelper * const q_ptr;
    QScopedPointer<QTimer> timer_;
    std::shared_ptr<QLocalSocket> storage_framework_socket_;
    QScopedPointer<QLocalSocket> helper_socket_;
    QScopedPointer<QLocalSocket> read_socket_;
    QByteArray upload_buffer_;
    qint64 n_read_ = 0;
    qint64 n_uploaded_ = 0;
    bool read_error_ = false;
    bool write_error_ = false;
    bool cancelled_ = false;
    bool storage_framework_socket_open_ = false;
    std::shared_ptr<QMetaObject::Connection> storage_framework_socket_connection_;
};

/***
****
***/

BackupHelper::BackupHelper(
    QString const & appid,
    clock_func const & clock,
    QObject * parent
)
    : Helper(appid, clock, parent)
    , d_ptr(new BackupHelperPrivate(this))
{
}

BackupHelper::~BackupHelper() =default;

void
BackupHelper::start(QStringList const& url)
{
    Q_D(BackupHelper);

    d->start(url);
}

void
BackupHelper::stop()
{
    Q_D(BackupHelper);

    d->stop();
}

void
BackupHelper::on_helper_process_stopped()
{
    Q_D(BackupHelper);

    d->on_helper_process_stopped();
}

void
BackupHelper::set_storage_framework_socket(std::shared_ptr<QLocalSocket> const &sf_socket)
{
    Q_D(BackupHelper);

    d->set_storage_framework_socket(sf_socket);
}

int
BackupHelper::get_helper_socket() const
{
    Q_D(const BackupHelper);

    return d->get_helper_socket();
}

QString
BackupHelper::to_string(Helper::State state) const
{
    Q_D(const BackupHelper);

    return d->to_string(state);
}

void
BackupHelper::on_storage_framework_finished()
{
    Q_D(BackupHelper);

    return d->on_storage_framework_finished();
}

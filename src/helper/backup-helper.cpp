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

#include "util/connection-helper.h"
#include "helper/backup-helper.h"
#include "service/app-const.h" // HELPER_TYPE

#include <QByteArray>
#include <QDebug>
#include <QLocalSocket>
#include <QMap>
#include <QObject>
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
    {
        // listen for inactivity
        QObject::connect(&timer_, &QTimer::timeout,
            std::bind(&BackupHelperPrivate::on_inactivity_detected, this)
        );

        // listen for data ready to read
        QObject::connect(&read_socket_, &QLocalSocket::readyRead,
            std::bind(&BackupHelperPrivate::on_ready_read, this)
        );

        // fire up the sockets
        int fds[2];
        int rc = socketpair(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK, 0, fds);
        if (rc == -1)
        {
            qWarning() <<  QStringLiteral("Error creating socket to communicate with helper");;
            Q_EMIT(q_ptr->error(keeper::KeeperError::HELPER_SOCKET_ERROR));
            return;
        }

        // helper socket is for the client.
        helper_socket_.setSocketDescriptor(fds[1], QLocalSocket::ConnectedState, QIODevice::WriteOnly);

        read_socket_.setSocketDescriptor(fds[0], QLocalSocket::ConnectedState, QIODevice::ReadOnly);
    }

    ~BackupHelperPrivate() = default;

    Q_DISABLE_COPY(BackupHelperPrivate)

    void start(QStringList const& urls)
    {
        q_ptr->Helper::start(urls);
        reset_inactivity_timer();
    }

    void set_uploader(std::shared_ptr<Uploader> const& uploader)
    {
        n_read_ = 0;
        n_uploaded_ = 0;
        read_error_ = false;
        write_error_ = false;
        cancelled_ = false;

        uploader_ = uploader;

        connections_.remember(QObject::connect(
            uploader_->socket().get(), &QLocalSocket::bytesWritten,
            std::bind(&BackupHelperPrivate::on_data_uploaded, this, std::placeholders::_1)
        ));

        // TODO xavi is going to remove this line
        q_ptr->Helper::on_helper_started();

        reset_inactivity_timer();
    }

    void stop()
    {
        cancelled_ = true;
        q_ptr->Helper::stop();
    }

    int get_helper_socket() const
    {
        return int(helper_socket_.socketDescriptor());
    }

    QString to_string(Helper::State state) const
    {
        return state == Helper::State::STARTED
            ? QStringLiteral("saving")
            : q_ptr->Helper::to_string(state);
    }

    void on_state_changed(Helper::State state)
    {
        switch (state)
        {
            case Helper::State::CANCELLED:
            case Helper::State::FAILED:
                qDebug() << "cancelled/failed, calling uploader_.reset()";
                uploader_.reset();
                break;

            case Helper::State::DATA_COMPLETE: {
                qDebug() << "Backup helper finished, calling uploader_.commit()";
                connections_.connect_oneshot(
                    uploader_.get(),
                    &Uploader::commit_finished,
                    std::function<void(bool)>{[this](bool success){
                        qDebug() << "Commit finished";
                        if (!success)
                        {
                            write_error_ = true;
                            Q_EMIT(q_ptr->error(keeper::KeeperError::COMMITTING_DATA_ERROR));
                        }
                        else
                            uploader_committed_file_name_ = uploader_->file_name();
                        uploader_.reset();
                        check_for_done();
                    }}
                );
                uploader_->commit();
                break;
            }

            //case Helper::State::NOT_STARTED:
            //case Helper::State::STARTED:
            default:
                break;
        }
    }

    void on_helper_finished()
    {
        stop_inactivity_timer();
        check_for_done();
    }

    QString get_uploader_committed_file_name() const
    {
        return uploader_committed_file_name_;
    }

private:

    void on_inactivity_detected()
    {
        stop_inactivity_timer();
        qWarning() << "Inactivity detected in the helper...stopping it";
        Q_EMIT(q_ptr->error(keeper::KeeperError::HELPER_INACTIVITY_DETECTED));
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
        process_more();
        check_for_done();
    }

    void process_more()
    {
        if (!uploader_)
            return;

        char readbuf[UPLOAD_BUFFER_MAX_];
        auto socket = uploader_->socket();
        for(;;)
        {
            // try to fill the upload buf
            int max_bytes = UPLOAD_BUFFER_MAX_ - upload_buffer_.size();
            if (max_bytes > 0) {
                const auto n = read_socket_.read(readbuf, max_bytes);
                if (n > 0) {
                    n_read_ += n;
                    upload_buffer_.append(readbuf, int(n));
                }
                else if (n < 0) {
                    read_error_ = true;
                    Q_EMIT(q_ptr->error(keeper::KeeperError::HELPER_READ_ERROR));
                    stop();
                    return;
                }
            }

            // try to empty the upload buf
            const auto n = socket->write(upload_buffer_);
            if (n > 0) {
                upload_buffer_.remove(0, int(n));
                continue;
            }
            else {
                if (n < 0) {
                    write_error_ = true;
                    qWarning() << "Write error:" << socket->errorString();
                    Q_EMIT(q_ptr->error(keeper::KeeperError::HELPER_WRITE_ERROR));
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
        timer_.start(MAX_TIME_WAITING_FOR_DATA);
    }

    void stop_inactivity_timer()
    {
        timer_.stop();
    }

    void check_for_done()
    {
        if (cancelled_)
        {
            q_ptr->set_state(Helper::State::CANCELLED);
        }
        else if (read_error_ || write_error_ || n_uploaded_ > q_ptr->expected_size())
        {
            if (!q_ptr->is_helper_running())
            {
                if (n_uploaded_ > q_ptr->expected_size())
                {
                    Q_EMIT(q_ptr->error(keeper::KeeperError::HELPER_WRITE_ERROR));
                }
                q_ptr->set_state(Helper::State::FAILED);
            }
        }
        else if (n_uploaded_ == q_ptr->expected_size())
        {
            if (uploader_)
            {
                if (!q_ptr->is_helper_running())
                {
                    // only in the case that the helper process finished we move to the next state
                    // this is to prevent to start the next task too early
                    q_ptr->set_state(Helper::State::DATA_COMPLETE);
                    stop_inactivity_timer();
                }
            }
            else
                q_ptr->set_state(Helper::State::COMPLETE);
        }
    }

    /***
    ****
    ***/

    static constexpr int UPLOAD_BUFFER_MAX_ {1024*16};

    BackupHelper * const q_ptr;
    QTimer timer_;
    std::shared_ptr<Uploader> uploader_;
    QLocalSocket helper_socket_;
    QLocalSocket read_socket_;
    QByteArray upload_buffer_;
    qint64 n_read_ = 0;
    qint64 n_uploaded_ = 0;
    bool read_error_ = false;
    bool write_error_ = false;
    bool cancelled_ = false;
    ConnectionHelper connections_;
    QString uploader_committed_file_name_;
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
BackupHelper::set_uploader(std::shared_ptr<Uploader> const &uploader)
{
    Q_D(BackupHelper);

    d->set_uploader(uploader);
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
BackupHelper::set_state(Helper::State state)
{
    Q_D(BackupHelper);

    Helper::set_state(state);
    d->on_state_changed(state);
}

void BackupHelper::on_helper_finished()
{
    Q_D(BackupHelper);

    Helper::on_helper_finished();
    d->on_helper_finished();
}

QString BackupHelper::get_uploader_committed_file_name() const
{
    Q_D(const BackupHelper);

    return d->get_uploader_committed_file_name();
}

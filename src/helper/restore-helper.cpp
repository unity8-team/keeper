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
#include "helper/restore-helper.h"
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


class RestoreHelperPrivate
{
public:

    explicit RestoreHelperPrivate(
        RestoreHelper* backup_helper
    )
        : q_ptr(backup_helper)
    {
        // listen for inactivity from storage framework
        QObject::connect(&timer_, &QTimer::timeout,
            std::bind(&RestoreHelperPrivate::on_inactivity_detected, this)
        );

        // fire up the sockets
        int fds[2];
        int rc = socketpair(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK, 0, fds);
        if (rc == -1)
        {
            qWarning() <<  QStringLiteral("Error creating socket to communicate with helper");;
            Q_EMIT(q_ptr->error(KeeperError::HELPER_SOCKET_ERROR));
            return;
        }

        // helper socket is for the client.
        // We don't use a QLocalSocket here as it buffers data and it makes the helper miss packets.
        helper_socket_ = fds[1];

        write_socket_.setSocketDescriptor(fds[0], QLocalSocket::ConnectedState, QIODevice::WriteOnly);
    }

    ~RestoreHelperPrivate() = default;

    Q_DISABLE_COPY(RestoreHelperPrivate)

    void start(QStringList const& urls)
    {
        q_ptr->Helper::start(urls);
        reset_inactivity_timer();
    }

    void set_downloader(std::shared_ptr<Downloader> const& downloader)
    {
        n_read_ = 0;
        n_uploaded_ = 0;
        read_error_ = false;
        write_error_ = false;
        cancelled_ = false;

        q_ptr->set_expected_size(downloader->file_size());
        downloader_ = downloader;

        connections_.remember(QObject::connect(
            &write_socket_, &QLocalSocket::bytesWritten,
            std::bind(&RestoreHelperPrivate::on_data_uploaded, this, std::placeholders::_1)
        ));

        // listen for data ready to read
        QObject::connect(downloader_->socket().get(), &QLocalSocket::readyRead,
            std::bind(&RestoreHelperPrivate::on_ready_read, this)
        );

        // TODO investigate why UAL takes so long to call the helper started callback
        // At this point we are sure that the helper started, as it is the helper
        // the ones that asks for a downloader socket.
        q_ptr->Helper::on_helper_started();

        // maybe there's data already to be read
        process_more();

        reset_inactivity_timer();
    }

    void stop()
    {
        write_socket_.disconnectFromServer();
        cancelled_ = true;
        q_ptr->Helper::stop();
    }

    int get_helper_socket() const
    {
        return helper_socket_;
    }

    QString to_string(Helper::State state) const
    {
        return state == Helper::State::STARTED
            ? QStringLiteral("restoring")
            : q_ptr->Helper::to_string(state);
    }

    void on_state_changed(Helper::State state)
    {
        switch (state)
        {
            case Helper::State::CANCELLED:
            case Helper::State::FAILED:
                qDebug() << "cancelled/failed, calling downloader_.reset()";
                downloader_.reset();
                break;

            case Helper::State::DATA_COMPLETE: {
                qDebug() << "Restore helper finished, calling downloader_.finish()";
                write_socket_.disconnectFromServer();
                downloader_->finish();
                downloader_.reset();
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

private:

    void on_inactivity_detected()
    {
        stop_inactivity_timer();
        qWarning() << "Inactivity detected in the helper...stopping it";
        Q_EMIT(q_ptr->error(KeeperError::HELPER_INACTIVITY_DETECTED));
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
        if (!downloader_)
            return;

        char readbuf[UPLOAD_BUFFER_MAX_];
        auto socket = downloader_->socket();
        while(socket->bytesAvailable() || upload_buffer_.size())
        {
            if (socket->bytesAvailable())
            {
                // try to fill the upload buf
                int max_bytes = UPLOAD_BUFFER_MAX_ - upload_buffer_.size();
                if (max_bytes > 0) {
                    const auto n = socket->read(readbuf, max_bytes);
                    if (n > 0) {
                        n_read_ += n;
                        upload_buffer_.append(readbuf, int(n));
                        qDebug("upload_buffer_.size() is %zu after reading %zu from helper", size_t(upload_buffer_.size()), size_t(n));
                    }
                    else if (n < 0) {
                        read_error_ = true;
                        qDebug() << "Read error in restore helper: " << socket->errorString();
                        Q_EMIT(q_ptr->error(KeeperError::HELPER_READ_ERROR));
                        stop();
                        return;
                    }
                }
            }

            if (upload_buffer_.size())
            {
                // try to empty the upload buf
                const auto n = write_socket_.write(upload_buffer_);
                if (n > 0) {
                    upload_buffer_.remove(0, int(n));
                    qDebug("upload_buffer_.size() is %zu after writing %zu to cloud", size_t(upload_buffer_.size()), size_t(n));
                    continue;
                }
                else {
                    if (n < 0) {
                        write_error_ = true;
                        qWarning() << "Write error:" << write_socket_.errorString();
                        Q_EMIT(q_ptr->error(KeeperError::HELPER_WRITE_ERROR));
                        stop();
                    }
                    break;
                }
            }
        }

        reset_inactivity_timer();
    }

    void reset_inactivity_timer()
    {
        static constexpr int MAX_TIME_WAITING_FOR_DATA {RestoreHelper::MAX_INACTIVITY_TIME};
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
                    Q_EMIT(q_ptr->error(KeeperError::HELPER_WRITE_ERROR));
                }
                q_ptr->set_state(Helper::State::FAILED);
            }
        }
        else if (n_uploaded_ == q_ptr->expected_size())
        {
            if (downloader_)
            {
                if (q_ptr->is_helper_running())
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

    RestoreHelper * const q_ptr;
    QTimer timer_;
    std::shared_ptr<Downloader> downloader_;
    int helper_socket_ = -1;
    QLocalSocket write_socket_;
    QByteArray upload_buffer_;
    qint64 n_read_ = 0;
    qint64 n_uploaded_ = 0;
    bool read_error_ = false;
    bool write_error_ = false;
    bool cancelled_ = false;
    ConnectionHelper connections_;
};

/***
****
***/

RestoreHelper::RestoreHelper(
    QString const & appid,
    clock_func const & clock,
    QObject * parent
)
    : Helper(appid, clock, parent)
    , d_ptr(new RestoreHelperPrivate(this))
{
}

RestoreHelper::~RestoreHelper() =default;

void
RestoreHelper::start(QStringList const& url)
{
    Q_D(RestoreHelper);

    d->start(url);
}

void
RestoreHelper::stop()
{
    Q_D(RestoreHelper);

    d->stop();
}

void
RestoreHelper::set_downloader(std::shared_ptr<Downloader> const& downloader)
{
    Q_D(RestoreHelper);

    d->set_downloader(downloader);
}

int
RestoreHelper::get_helper_socket() const
{
    Q_D(const RestoreHelper);

    return d->get_helper_socket();
}

QString
RestoreHelper::to_string(Helper::State state) const
{
    Q_D(const RestoreHelper);

    return d->to_string(state);
}

void
RestoreHelper::set_state(Helper::State state)
{
    Q_D(RestoreHelper);

    Helper::set_state(state);
    d->on_state_changed(state);
}

void RestoreHelper::on_helper_finished()
{
    Q_D(RestoreHelper);

    Helper::on_helper_finished();
    d->on_helper_finished();
}

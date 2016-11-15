#include "test-restore-socket.h"

#include "storage-framework/storage_framework_client.h"
#include "storage-framework/sf-downloader.h"

constexpr int UPLOAD_BUFFER_MAX_ {1024*16};

TestRestoreSocket::TestRestoreSocket(QObject *parent)
    : QObject(parent)
    , sf_client_(new StorageFrameworkClient)
    , file_("/tmp/test-downloader")
{
    file_.open(QIODevice::WriteOnly);
    connect (&future_watcher_, &QFutureWatcher<std::shared_ptr<Downloader>>::finished, [this]{on_socket_received(future_watcher_.result());});
}

TestRestoreSocket::~TestRestoreSocket() = default;

void TestRestoreSocket::start(QString const & dir_name, QString const & file_name)
{
    qDebug() << "asking storage framework for a socket for reading";

    future_watcher_.setFuture(sf_client_->get_new_downloader(dir_name, file_name));
//    connections_.connect_future(
//        sf_client_->get_new_downloader(dir_name, file_name),
//        std::function<void(std::shared_ptr<Downloader> const&)>{
//            [this](std::shared_ptr<Downloader> const& downloader){
//                qDebug() << "Downloader is" << static_cast<void*>(downloader.get());
//                if (downloader) {
//                      this->on_socket_received(downloader);
//                }
//            }
//        }
//    );
}

void TestRestoreSocket::read_data()
{
    if (socket_->bytesAvailable())
    {
        char readbuf[UPLOAD_BUFFER_MAX_];
        // try to fill the upload buf
        int max_bytes = UPLOAD_BUFFER_MAX_;// - upload_buffer_.size();
        if (max_bytes > 0) {
            const auto n = socket_->read(readbuf, max_bytes);
            if (n > 0) {
                n_read_ += n;
                qDebug() << "Read " << n << " bytes. Total: " << n_read_;

                if (file_.isOpen())
                {
                    file_.write(readbuf, max_bytes);
                }
            }
            else if (n < 0) {
                qDebug() << "Read error: " << socket_->errorString();
                return;
            }
        }
    }
}

void TestRestoreSocket::on_disconnected()
{
    qDebug() << "Socket disconnected";
    if (file_.isOpen())
    {
        file_.close();
    }
}

void TestRestoreSocket::on_data_stored(qint64 /*n*/)
{
    read_data();
}

void TestRestoreSocket::on_socket_received(std::shared_ptr<Downloader> const & downloader)
{
    socket_ = downloader->socket();
    QObject::connect(socket_.get(), &QLocalSocket::readyRead, this, &TestRestoreSocket::read_data);
    QObject::connect(socket_.get(), &QLocalSocket::disconnected, this, &TestRestoreSocket::on_disconnected);
    QObject::connect(&file_, &QIODevice::bytesWritten, this, &TestRestoreSocket::on_data_stored);
}

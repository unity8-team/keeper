#pragma once

#include "util/connection-helper.h"

#include <QObject>
#include <QScopedPointer>
#include <QFile>

#include <memory>

class QLocalSocket;
class StorageFrameworkClient;
class Downloader;

class TestRestoreSocket : public QObject
{
    Q_OBJECT
public:
    TestRestoreSocket(QObject *parent = nullptr);
    virtual ~TestRestoreSocket();

    void start(QString const & dir_name, QString const & file_name);

public Q_SLOTS:
    void read_data();
    void on_socket_received(std::shared_ptr<Downloader> const & downloader);
    void on_disconnected();
    void on_data_stored(qint64 n);

private:
    std::shared_ptr<QLocalSocket> socket_;
    QScopedPointer<StorageFrameworkClient> sf_client_;
    ConnectionHelper connections_;
    qint64 n_read_ = 0;
    QFile file_;
    QFutureWatcher<std::shared_ptr<Downloader>> future_watcher_;
};

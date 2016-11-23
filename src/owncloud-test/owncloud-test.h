#pragma once

#include "storage-framework/storage_framework_client.h"

#include <QObject>
#include <QSharedPointer>
#include <QFutureWatcher>

class OwnCloudTest : public QObject
{
    Q_OBJECT
public:
    OwnCloudTest(QSharedPointer<StorageFrameworkClient> const & sf_client, bool create_dir, QObject *parent = nullptr);
    virtual ~OwnCloudTest();

    void get_root_and_start();

public Q_SLOTS:
    void start();
    void accounts_ready();
    void roots_ready();
    void folder_ready();
    void uplodaer_ready();
    void uploader_finished();

private:
    QSharedPointer<StorageFrameworkClient> sf_client_;

    QFutureWatcher<QVector<std::shared_ptr<unity::storage::qt::client::Account>>> accounts_watcher_;
    QFutureWatcher<QVector<std::shared_ptr<unity::storage::qt::client::Root>>> roots_watcher_;
    QFutureWatcher<unity::storage::qt::client::Folder::SPtr> create_folder_watcher_;
    QFutureWatcher<std::shared_ptr<unity::storage::qt::client::Uploader>> create_uploader_watcher_;
    QFutureWatcher<std::shared_ptr<unity::storage::qt::client::File>> uploader_finished_watcher_;
    unity::storage::qt::client::Root::SPtr root_;
    unity::storage::qt::client::Folder::SPtr folder_;
    QString backup_dir_name_;

    bool create_dir_ = false;
};


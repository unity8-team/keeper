#include "storage-framework/storage_framework_client.h"

#include <QCoreApplication>

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);

    StorageFrameworkClient sf_client;

    auto const now = QDateTime::currentDateTime();
    auto backup_dir_name = now.toString(Qt::ISODate);
    QFutureWatcher<std::shared_ptr<Uploader>> uploader_watcher;
    QObject::connect(&uploader_watcher, &QFutureWatcher<std::shared_ptr<Uploader>>::finished, []{ qDebug() << "get uploader finished";});

    uploader_watcher.setFuture(sf_client.get_new_uploader(100, backup_dir_name, backup_dir_name));

    return app.exec();
}

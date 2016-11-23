#include "storage-framework/storage_framework_client.h"
#include "owncloud-test.h"

#include <QCoreApplication>
#include <QSharedPointer>

namespace sf = unity::storage::qt::client;

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);

    QSharedPointer<StorageFrameworkClient> sf_client (new StorageFrameworkClient);

    // change to create a folder first and create the file in it
    bool want_to_create_uploader_in_folder = false;


    OwnCloudTest test(sf_client, want_to_create_uploader_in_folder);
    test.get_root_and_start();
    return app.exec();
}

#include "owncloud-test.h"

namespace sf = unity::storage::qt::client;

OwnCloudTest::OwnCloudTest(QSharedPointer<StorageFrameworkClient> const & sf_client, bool create_dir,  QObject *parent)
    : QObject(parent)
    , sf_client_(sf_client)
    , create_dir_(create_dir)
{
    connect(&accounts_watcher_, &QFutureWatcher<QVector<std::shared_ptr<sf::Account>>>::finished,
            this, &OwnCloudTest::accounts_ready);

    connect(&roots_watcher_, &QFutureWatcher<QVector<std::shared_ptr<sf::Root>>>::finished,
            this, &OwnCloudTest::roots_ready);

    connect(&create_folder_watcher_, &QFutureWatcher<sf::Folder::SPtr>::finished,
            this, &OwnCloudTest::folder_ready);

    connect(&create_uploader_watcher_, &QFutureWatcher<std::shared_ptr<sf::Uploader>>::finished,
            this, &OwnCloudTest::uplodaer_ready);

    connect(&uploader_finished_watcher_, &QFutureWatcher<std::shared_ptr<sf::File>>::finished,
            this, &OwnCloudTest::uploader_finished);

    auto const now = QDateTime::currentDateTime();
    backup_dir_name_ = now.toString(Qt::ISODate);
}

OwnCloudTest::~OwnCloudTest() = default;

void OwnCloudTest::get_root_and_start()
{
    accounts_watcher_.setFuture(sf_client_->runtime_->accounts());
}

void OwnCloudTest::start()
{
    qDebug() << "Root is ready";

    if (create_dir_)
    {
        qDebug() << "Creating directory";
        create_folder_watcher_.setFuture(root_->create_folder(backup_dir_name_));
    }
    else
    {
        create_uploader_watcher_.setFuture(root_->create_file(backup_dir_name_,1));
    }
}

void OwnCloudTest::accounts_ready()
{
    qDebug() << "Accounts are ready";
    try
    {
        auto accounts = accounts_watcher_.result();
        if (!accounts.size())
        {
            qWarning() << "Accounts are empty";
            return;
        }

        auto account = accounts.front();
        roots_watcher_.setFuture(account->roots());
    }
    catch (std::exception &e)
    {
        qDebug() << "Exception getting accounts: " << e.what();
    }
}

void OwnCloudTest::roots_ready()
{
    qDebug() << "Roots are ready";
    try
    {
        auto roots = roots_watcher_.result();
        if (!roots.size())
        {
            qWarning() << "Roots are empty";
            return;
        }
        root_ = roots.front();

        if (!root_)
        {
            qWarning() << "Root is empty";
            return;
        }

        start();
    }
    catch (std::exception &e)
    {
        qDebug() << "Exception getting roots: " << e.what();
    }
}

void OwnCloudTest::folder_ready()
{
    qDebug() << "Folder is ready";
    try
    {
        folder_ = create_folder_watcher_.result();
        if (!folder_)
        {
            qWarning() << "Folder is null";
            return;
        }
        create_uploader_watcher_.setFuture(folder_->create_file(backup_dir_name_, 1));
    }
    catch (std::exception &e)
    {
        qDebug() << "Exception getting folder: " << e.what();
    }
}

void OwnCloudTest::uplodaer_ready()
{
    qDebug() << "Uploader is ready";

    try
    {
        auto uploader = create_uploader_watcher_.result();
        if (!uploader)
        {
            qWarning() << "uploader is null";
            return;
        }
        auto socket = uploader->socket();
        char buffer[1];
        buffer[0] = '9';
        socket->write(buffer, 1);
        uploader_finished_watcher_.setFuture(uploader->finish_upload());

    }
    catch (std::exception &e)
    {
        qDebug() << "Exception getting uploader: " << e.what();
    }
}

void OwnCloudTest::uploader_finished()
{
    qDebug() << "Uploader finished";
    try
    {
        auto file = uploader_finished_watcher_.result();
        if (!file)
        {
            qWarning() << "Uploader file is null";
            return;
        }
        qDebug() << "The file uploaded is: " << file->name();
    }
    catch (std::exception &e)
    {
        qDebug() << "Exception getting uploader finished result: " << e.what();
    }
}

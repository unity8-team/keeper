#include <QCoreApplication>

#include "test-restore-socket.h"

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);

    TestRestoreSocket test_restore;
    test_restore.start("test_dir", "test_file");

    return app.exec();
}

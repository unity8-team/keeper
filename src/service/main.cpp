#include <util/unix-signal-handler.h>
#include <dbus-types.h>

#include <QCoreApplication>
#include <QDBusConnection>

#include <libintl.h>
#include <cstdlib>
#include <ctime>

int
main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
//    DBusTypes::registerMetaTypes();
//    Variant::registerMetaTypes();
    std::srand(unsigned(std::time(0)));

    util::UnixSignalHandler handler([]{
        QCoreApplication::exit(0);
    });
    handler.setupUnixSignalHandlers();

    // boilerplate locale
    bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
    setlocale(LC_ALL, "");
    bindtextdomain(GETTEXT_PACKAGE, LOCALE_DIR);
    textdomain(GETTEXT_PACKAGE);

    if (argc == 2 && QString("--print-address") == argv[1])
    {
        qDebug() << QDBusConnection::systemBus().baseService();
    }

qInfo() << "Hello world!";

    return app.exec();
}

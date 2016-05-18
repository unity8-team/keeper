//#include <util/unix-signal-handler.h>
#include <dbus-types.h>

#include <QCoreApplication>
#include <QDBusConnection>

#include <libintl.h>
#include <cstdlib>
#include <ctime>

//#include <glib.h>

//#include <config.h>


int
main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
//    DBusTypes::registerMetaTypes();
//    Variant::registerMetaTypes();
    std::srand(std::time(0));

#if 0
    util::UnixSignalHandler handler([]{
        QCoreApplication::exit(0);
    });
    handler.setupUnixSignalHandlers();
#endif

    bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
    setlocale(LC_ALL, "");
    bindtextdomain(GETTEXT_PACKAGE, LOCALE_DIR);
    textdomain(GETTEXT_PACKAGE);

    if (argc == 2 && QString("--print-address") == argv[1])
    {
        qDebug() << QDBusConnection::systemBus().baseService();
    }

#if 0
    Factory factory;
    auto menu = factory.newMenuBuilder();
    auto connectivityService = factory.newConnectivityService();
    auto vpnStatusNotifier = factory.newVpnStatusNotifier();
#endif

    return app.exec();
}
 

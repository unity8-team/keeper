#include <dbus-types.h>
#include <util/logging.h>

#include <QCoreApplication>
#include <QDBusConnection>

#include <libintl.h>
#include <cstdlib>
#include <ctime>

int
main(int argc, char **argv)
{
    qInstallMessageHandler(util::loggingFunction);

    QCoreApplication app(argc, argv);
//    DBusTypes::registerMetaTypes();
//    Variant::registerMetaTypes();
    std::srand(unsigned(std::time(nullptr)));

    // boilerplate locale
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

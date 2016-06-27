/*
 * Copyright (C) 2016 Canonical, Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3, as published
 * by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranties of
 * MERCHANTABILITY, SATISFACTORY QUALITY, or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authors:
 *   Charles Kerr <charles.kerr@canoincal.com>
 */

#include "service/backup-choices.h"

#include <click.h>

#include <QDebug>
#include <QJsonDocument>
#include <QStandardPaths>
#include <QString>

#include <array>
#include <iostream>

#include <uuid/uuid.h>

BackupChoices::BackupChoices() //=default;
{
    get_backups();
}

BackupChoices::~BackupChoices() =default;

QVector<Metadata>
BackupChoices::get_backups()
{
    QVector<Metadata> ret;

    // Click Packages

    QString manifests;
#if 0
    GError* error {};
    auto user = click_user_new_for_user(nullptr, nullptr, &error);
    if (user != nullptr)
    {
        auto tmp = click_user_get_manifests_as_string (user, &error);
        manifests = QString::fromUtf8(tmp);
        g_clear_pointer(&manifests, g_free);
        g_clear_object(&user);
    }
#else
    manifests = QString::fromUtf8("[{\"architecture\":\"all\",\"description\":\"eBay (webapp version)\",\"framework\":\"ubuntu-sdk-14.10\",\"hooks\":{\"webapp-ebay\":{\"apparmor\":\"webapp-ebay.json\",\"desktop\":\"webapp-ebay.desktop\"}},\"installed-size\":\"21\",\"maintainer\":\"Webapps Team <webapps@lists.launchpad.net>\",\"name\":\"com.ubuntu.developer.webapps.webapp-ebay\",\"title\":\"webapp-ebay\",\"version\":\"1.0.15\",\"_directory\":\"/opt/click.ubuntu.com/.click/users/@all/com.ubuntu.developer.webapps.webapp-ebay\",\"_removable\":1},{\"architecture\":\"all\",\"description\":\"Facebook (webapp version)\",\"framework\":\"ubuntu-sdk-14.10\",\"hooks\":{\"webapp-facebook\":{\"account-application\":\"webapp-facebook.application\",\"account-service\":\"webapp-facebook.service\",\"apparmor\":\"webapp-facebook.json\",\"content-hub\":\"content-hub/webapp-facebook.json\",\"desktop\":\"webapp-facebook.desktop\",\"urls\":\"facebook.url-dispatcher\"},\"webapp-facebook-helper\":{\"apparmor\":\"facebook-helper-apparmor.json\",\"push-helper\":\"facebook-helper.json\"}},\"installed-size\":\"891\",\"maintainer\":\"Webapps Team <webapps@lists.launchpad.net>\",\"name\":\"com.ubuntu.developer.webapps.webapp-facebook\",\"title\":\"Facebook\",\"version\":\"1.4.1\",\"_directory\":\"/opt/click.ubuntu.com/.click/users/@all/com.ubuntu.developer.webapps.webapp-facebook\",\"_removable\":1},{\"architecture\":\"armhf\",\"description\":\"Gallery application for your photo collection\",\"framework\":\"ubuntu-sdk-15.04.4\",\"hooks\":{\"gallery\":{\"apparmor\":\"gallery.apparmor\",\"content-hub\":\"gallery-content.json\",\"desktop\":\"share/applications/gallery-app.desktop\",\"urls\":\"share/url-dispatcher/urls/gallery-app.url-dispatcher\"}},\"icon\":\"share/icons/gallery-app.svg\",\"installed-size\":\"5240\",\"maintainer\":\"Ubuntu Developers <ubuntu-devel-discuss@lists.ubuntu.com>\",\"name\":\"com.ubuntu.gallery\",\"title\":\"Gallery\",\"version\":\"2.9.1.1322\",\"x-source\":{\"vcs-bzr\":\"lp:gallery-app\",\"vcs-bzr-revno\":\"1322\"},\"x-test\":{\"autopilot\":\"gallery_app\"},\"_directory\":\"/opt/click.ubuntu.com/.click/users/@all/com.ubuntu.gallery\",\"_removable\":1},{\"architecture\":\"all\",\"description\":\"A weather forecast application for Ubuntu with support for multiple online weather data sources\",\"framework\":\"ubuntu-sdk-15.04.3-qml\",\"hooks\":{\"weather\":{\"apparmor\":\"ubuntu-weather-app.apparmor\",\"desktop\":\"share/applications/ubuntu-weather-app.desktop\"}},\"icon\":\"app/weather-app@30.png\",\"installed-size\":\"3020\",\"maintainer\":\"Ubuntu App Cats <ubuntu-touch-coreapps@lists.launchpad.net>\",\"name\":\"com.ubuntu.weather\",\"title\":\"Weather\",\"version\":\"3.3.244\",\"x-source\":{\"vcs-bzr\":\"lp:ubuntu-weather-app/reboot\",\"vcs-bzr-revno\":\"244\"},\"x-test\":{\"autopilot\":\"ubuntu_weather_app\"},\"_directory\":\"/opt/click.ubuntu.com/.click/users/@all/com.ubuntu.weather\",\"_removable\":1},{\"architecture\":\"all\",\"description\":\"A calendar for Ubuntu which syncs with online accounts\",\"framework\":\"ubuntu-sdk-15.04.4\",\"hooks\":{\"calendar\":{\"account-application\":\"com.ubuntu.calendar_calendar.application\",\"apparmor\":\"calendar.apparmor\",\"desktop\":\"com.ubuntu.calendar_calendar.desktop\",\"urls\":\"com.ubuntu.calendar_calendar.url-dispatcher\"},\"calendar-helper\":{\"apparmor\":\"calendar-helper-apparmor.json\",\"push-helper\":\"push-helper.json\"}},\"icon\":\"calendar-app@30.png\",\"installed-size\":\"1332\",\"maintainer\":\"Ubuntu App Cats <ubuntu-touch-coreapps@lists.launchpad.net>\",\"name\":\"com.ubuntu.calendar\",\"title\":\"Calendar\",\"version\":\"0.5.837\",\"x-source\":{\"vcs-bzr\":\"lp:ubuntu-calendar-app\",\"vcs-bzr-revno\":\"837\"},\"x-test\":{\"autopilot\":{\"autopilot_module\":\"calendar_app\",\"depends\":[\"python3-dateutil\",\"address-book-service-dummy\",\"address-book-service-testability\"]}},\"_directory\":\"/custom/click/.click/users/@all/com.ubuntu.calendar\",\"_removable\":1},{\"architecture\":\"all\",\"description\":\"Amazon (webapp version)\",\"framework\":\"ubuntu-sdk-14.10\",\"hooks\":{\"webapp-amazon\":{\"apparmor\":\"webapp-amazon.json\",\"desktop\":\"webapp-amazon.desktop\"}},\"installed-size\":\"43\",\"maintainer\":\"Webapps Team <webapps@lists.launchpad.net>\",\"name\":\"com.ubuntu.developer.webapps.webapp-amazon\",\"title\":\"webapp-amazon\",\"version\":\"1.0.11\",\"x-test\":{\"autopilot\":{\"autopilot_module\":\"amazon_webapp\",\"depends\":[\"click\",\"dpkg-dev\",\"qtdeclarative5-ubuntu-ui-toolkit-plugin\",\"ubuntu-sdk-libs\",\"oxideqt-chromedriver\",\"python3-selenium\",\"webapp-container\"]}},\"_directory\":\"/custom/click/.click/users/@all/com.ubuntu.developer.webapps.webapp-amazon\",\"_removable\":1},{\"architecture\":\"all\",\"description\":\"Gmail (webapp version)\",\"framework\":\"ubuntu-sdk-14.10\",\"hooks\":{\"webapp-gmail\":{\"account-application\":\"webapp-gmail.application\",\"account-service\":\"webapp-gmail.service\",\"apparmor\":\"webapp-gmail.json\",\"desktop\":\"webapp-gmail.desktop\",\"urls\":\"gmail.url-dispatcher\",\"webapp-container\":\"webapp-click.hook\"},\"webapp-gmail-helper\":{\"apparmor\":\"gmail-helper-apparmor.json\",\"push-helper\":\"gmail-helper.json\"}},\"installed-size\":\"22\",\"maintainer\":\"Webapps Team <webapps@lists.launchpad.net>\",\"name\":\"com.ubuntu.developer.webapps.webapp-gmail\",\"title\":\"webapp-gmail\",\"version\":\"1.1.1\",\"_directory\":\"/custom/click/.click/users/@all/com.ubuntu.developer.webapps.webapp-gmail\",\"_removable\":1},{\"architecture\":\"all\",\"description\":\"Twitter (webapp version)\",\"framework\":\"ubuntu-sdk-14.10\",\"hooks\":{\"webapp-twitter\":{\"account-application\":\"webapp-twitter.application\",\"account-service\":\"twitter-webapp.service\",\"apparmor\":\"webapp-twitter.json\",\"content-hub\":\"content-hub/webapp-twitter.json\",\"desktop\":\"webapp-twitter.desktop\",\"urls\":\"twitter.url-dispatcher\"},\"webapp-twitter-helper\":{\"apparmor\":\"twitter-helper-apparmor.json\",\"push-helper\":\"twitter-helper.json\"}},\"installed-size\":\"894\",\"maintainer\":\"Webapps Team <webapps@lists.launchpad.net>\",\"name\":\"com.ubuntu.developer.webapps.webapp-twitter\",\"title\":\"webapp-twitter\",\"version\":\"1.3.2\",\"_directory\":\"/custom/click/.click/users/@all/com.ubuntu.developer.webapps.webapp-twitter\",\"_removable\":1},{\"description\":\"Dropping Letters game\",\"framework\":\"ubuntu-sdk-14.10-qml\",\"hooks\":{\"dropping-letters\":{\"apparmor\":\"apparmor/dropping-letters.json\",\"desktop\":\"dropping-letters.desktop\"}},\"icon\":\"dropping-letters.png\",\"installed-size\":\"2366\",\"maintainer\":\"Ubuntu App Cats <ubuntu-touch-coreapps@lists.launchpad.net>\",\"name\":\"com.ubuntu.dropping-letters\",\"title\":\"dropping-letters\",\"version\":\"0.1.2.2.69\",\"x-test\":{\"autopilot\":\"dropping_letters_app\"},\"_directory\":\"/custom/click/.click/users/@all/com.ubuntu.dropping-letters\",\"_removable\":1},{\"architecture\":[\"armhf\",\"i386\",\"amd64\"],\"description\":\"File Manager application\",\"framework\":\"ubuntu-sdk-14.10\",\"hooks\":{\"filemanager\":{\"apparmor\":\"filemanager.apparmor\",\"content-hub\":\"hub-exporter.json\",\"desktop\":\"com.ubuntu.filemanager.desktop\"}},\"icon\":\"filemanager64.png\",\"installed-size\":\"57307\",\"maintainer\":\"Ubuntu App Cats <ubuntu-touch-coreapps@lists.launchpad.net>\",\"name\":\"com.ubuntu.filemanager\",\"title\":\"File Manager\",\"version\":\"0.4.525\",\"x-source\":{\"vcs-bzr\":\"lp:ubuntu-filemanager-app\",\"vcs-bzr-revno\":\"525\"},\"x-test\":{\"autopilot\":{\"autopilot_module\":\"filemanager\",\"depends\":[\"python3-lxml\",\"python3-fixtures\"]}},\"_directory\":\"/custom/click/.click/users/@all/com.ubuntu.filemanager\",\"_removable\":1},{\"architecture\":\"armhf\",\"description\":\"Ubuntu Notes app, powered by Evernote\",\"framework\":\"ubuntu-sdk-15.04.1\",\"hooks\":{\"evernote-account-plugin\":{\"account-provider\":\"share/accounts/providers/com.ubuntu.reminders_evernote-account-plugin.provider\",\"account-qml-plugin\":\"share/accounts/qml-plugins/evernote\"},\"pushHelper\":{\"apparmor\":\"push-helper.apparmor\",\"push-helper\":\"push-helper.json\"},\"reminders\":{\"account-application\":\"com.ubuntu.reminders_reminders.application\",\"account-service\":\"share/accounts/services/com.ubuntu.reminders_reminders.service\",\"apparmor\":\"reminders.apparmor\",\"content-hub\":\"reminders-contenthub.json\",\"desktop\":\"com.ubuntu.reminders.desktop\",\"urls\":\"reminders.url-dispatcher\"}},\"installed-size\":\"9995\",\"maintainer\":\"Ubuntu App Cats <ubuntu-touch-coreapps@lists.launchpad.net>\",\"name\":\"com.ubuntu.reminders\",\"title\":\"Notes\",\"version\":\"0.5.568\",\"x-source\":{\"vcs-bzr\":\"lp:reminders-app\",\"vcs-bzr-revno\":\"568\"},\"x-test\":{\"autopilot\":{\"autopilot_module\":\"reminders\",\"depends\":[\"account-plugin-evernote-sandbox\",\"libclick-0.4-0\",\"python3-dbus\",\"python3-dbusmock\",\"python3-fixtures\",\"python3-oauthlib\",\"python3-requests-oauthlib\"]}},\"_directory\":\"/custom/click/.click/users/@all/com.ubuntu.reminders\",\"_removable\":1},{\"architecture\":\"armhf\",\"description\":\"YouTube scope\",\"framework\":\"ubuntu-sdk-15.04\",\"hooks\":{\"youtube\":{\"account-application\":\"youtube.application\",\"account-service\":\"youtube.service\",\"apparmor\":\"apparmor.json\",\"scope\":\"youtube\"}},\"icon\":\"youtube/icon.png\",\"installed-size\":\"2562\",\"maintainer\":\"Ubuntu Developers <ubuntu-devel-discuss@lists.ubuntu.com>\",\"name\":\"com.ubuntu.scopes.youtube\",\"title\":\"YouTube scope\",\"version\":\"1.5.1-152\",\"_directory\":\"/custom/click/.click/users/@all/com.ubuntu.scopes.youtube\",\"_removable\":1},{\"architecture\":[\"armhf\",\"i386\",\"amd64\"],\"description\":\"Rss Reader application\",\"framework\":\"ubuntu-sdk-15.04.1-qml\",\"hooks\":{\"shorts\":{\"apparmor\":\"shorts.apparmor\",\"desktop\":\"shorts.desktop\"}},\"icon\":\"shorts.png\",\"installed-size\":\"2071\",\"maintainer\":\"Ubuntu App Cats <ubuntu-touch-coreapps@lists.launchpad.net>\",\"name\":\"com.ubuntu.shorts\",\"title\":\"Shorts\",\"version\":\"0.2.418\",\"x-source\":{\"vcs-bzr\":\"lp:ubuntu-rssreader-app/reboot\",\"vcs-bzr-revno\":\"418\"},\"x-test\":{\"autopilot\":\"shorts_app\"},\"_directory\":\"/custom/click/.click/users/@all/com.ubuntu.shorts\",\"_removable\":1},{\"architecture\":\"all\",\"description\":\"Sudoku game for Ubuntu devices.\",\"framework\":\"ubuntu-sdk-15.04\",\"hooks\":{\"sudoku\":{\"apparmor\":\"sudoku.apparmor\",\"desktop\":\"com.ubuntu.sudoku_sudoku.desktop\"}},\"icon\":\"SudokuGameIcon.png\",\"installed-size\":\"916\",\"maintainer\":\"Ubuntu Core Apps Developers <ubuntu-touch-coreapps@lists.launchpad.net>\",\"name\":\"com.ubuntu.sudoku\",\"title\":\"Sudoku\",\"version\":\"1.1.382\",\"x-source\":{\"vcs-bzr\":\"lp:sudoku-app\",\"vcs-bzr-revno\":\"382\"},\"x-test\":{\"autopilot\":\"sudoku_app\"},\"_directory\":\"/custom/click/.click/users/@all/com.ubuntu.sudoku\",\"_removable\":1},{\"architecture\":\"armhf\",\"description\":\"Terminal application\",\"framework\":\"ubuntu-sdk-15.04.3\",\"hooks\":{\"terminal\":{\"apparmor\":\"terminal.apparmor\",\"desktop\":\"com.ubuntu.terminal.desktop\"}},\"icon\":\"terminal64.png\",\"installed-size\":\"1856\",\"maintainer\":\"Ubuntu App Cats <ubuntu-touch-coreapps@lists.launchpad.net>\",\"name\":\"com.ubuntu.terminal\",\"title\":\"Terminal\",\"version\":\"0.7.198\",\"x-source\":{\"vcs-bzr\":\"lp:ubuntu-terminal-app\",\"vcs-bzr-revno\":\"198\"},\"x-test\":{\"autopilot\":\"ubuntu_terminal_app\"},\"_directory\":\"/custom/click/.click/users/@all/com.ubuntu.terminal\",\"_removable\":1},{\"architecture\":\"armhf\",\"description\":\"Dekko email client for Ubuntu devices\",\"framework\":\"ubuntu-sdk-15.04.3\",\"hooks\":{\"dekko\":{\"account-application\":\"dekko.dekkoproject_dekko.application\",\"account-service\":\"dekko.dekkoproject_dekko.service\",\"apparmor\":\"dekko.apparmor\",\"content-hub\":\"dekko-content.json\",\"desktop\":\"dekko.desktop\",\"urls\":\"dekko.url-dispatcher\"},\"dekko-notify\":{\"apparmor\":\"pushHelper-apparmor.json\",\"push-helper\":\"pushHelper.json\"}},\"installed-size\":\"10503\",\"maintainer\":\"Dan Chapman <dpniel@ubuntu.com>\",\"name\":\"dekko.dekkoproject\",\"title\":\"Dekko\",\"version\":\"0.6.20\",\"_directory\":\"/custom/click/.click/users/@all/dekko.dekkoproject\",\"_removable\":1},{\"architecture\":\"all\",\"description\":\"turn-by-turn GPS Navigation\",\"framework\":\"ubuntu-sdk-15.04.3\",\"hooks\":{\"navigator\":{\"apparmor\":\"app-armor.json\",\"desktop\":\"app.desktop\",\"urls\":\"app-dispatcher.json\"}},\"installed-size\":\"8833\",\"maintainer\":\"Marcos Costales <costales.marcos@gmail.com>\",\"name\":\"navigator.costales\",\"title\":\"uNav\",\"version\":\"0.59\",\"_directory\":\"/custom/click/.click/users/@all/navigator.costales\",\"_removable\":1},{\"architecture\":\"all\",\"description\":\"Powerful and easy to use calculator.\",\"framework\":\"ubuntu-sdk-14.10\",\"hooks\":{\"calculator\":{\"apparmor\":\"ubuntu-calculator-app.apparmor\",\"desktop\":\"share/applications/ubuntu-calculator-app.desktop\"}},\"icon\":\"\",\"installed-size\":\"2226\",\"maintainer\":\"Ubuntu App Cats <ubuntu-touch-coreapps@lists.launchpad.net>\",\"name\":\"com.ubuntu.calculator\",\"title\":\"Calculator\",\"version\":\"2.0.233\",\"x-source\":{\"vcs-bzr\":\"lp:ubuntu-calculator-app\",\"vcs-bzr-revno\":\"233\"},\"x-test\":{\"autopilot\":\"ubuntu_calculator_app\"},\"_directory\":\"/usr/share/click/preinstalled/.click/users/@all/com.ubuntu.calculator\",\"_removable\":1},{\"architecture\":\"armhf\",\"description\":\"An application to take pictures and videos with the device cameras\",\"framework\":\"ubuntu-sdk-15.04.4\",\"hooks\":{\"camera\":{\"apparmor\":\"camera.apparmor\",\"content-hub\":\"camera-contenthub.json\",\"desktop\":\"camera-app.desktop\"}},\"icon\":\"share/icons/camera-app.png\",\"installed-size\":\"1300\",\"maintainer\":\"Ubuntu Developers <ubuntu-devel-discuss@lists.ubuntu.com>\",\"name\":\"com.ubuntu.camera\",\"title\":\"Camera\",\"version\":\"3.0.0.652\",\"x-source\":{\"vcs-bzr\":\"lp:camera-app\",\"vcs-bzr-revno\":\"652\"},\"x-test\":{\"autopilot\":{\"autopilot_module\":\"camera_app\",\"depends\":[\"python3-wand\",\"python3-mediainfodll\"]}},\"_directory\":\"/usr/share/click/preinstalled/.click/users/@all/com.ubuntu.camera\",\"_removable\":1},{\"architecture\":\"armhf\",\"description\":\"A sophisticated clock app for Ubuntu Touch\",\"framework\":\"ubuntu-sdk-15.04.3\",\"hooks\":{\"clock\":{\"apparmor\":\"ubuntu-clock-app.json\",\"desktop\":\"share/applications/ubuntu-clock-app.desktop\",\"urls\":\"share/url-dispatcher/urls/com.ubuntu.clock_clock.url-dispatcher\"}},\"icon\":\"clock@30.png\",\"installed-size\":\"2141\",\"maintainer\":\"Ubuntu App Cats <ubuntu-touch-coreapps@lists.launchpad.net>\",\"name\":\"com.ubuntu.clock\",\"title\":\"Clock\",\"version\":\"3.7.456\",\"x-source\":{\"vcs-bzr\":\"lp:ubuntu-clock-app\",\"vcs-bzr-revno\":\"456\"},\"x-test\":{\"autopilot\":{\"autopilot_module\":\"ubuntu_clock_app\",\"depends\":[\"python3-lxml\"]}},\"_directory\":\"/usr/share/click/preinstalled/.click/users/@all/com.ubuntu.clock\",\"_removable\":1},{\"architecture\":\"all\",\"description\":\"A music application for ubuntu\",\"framework\":\"ubuntu-sdk-15.04.3-qml\",\"hooks\":{\"music\":{\"apparmor\":\"apparmor.json\",\"content-hub\":\"music-app-content.json\",\"desktop\":\"com.ubuntu.music_music.desktop\",\"urls\":\"com.ubuntu.music_music.url-dispatcher\"}},\"icon\":\"app/graphics/music-app@30.png\",\"installed-size\":\"1292\",\"maintainer\":\"Ubuntu App Cats <ubuntu-touch-coreapps@lists.launchpad.net>\",\"name\":\"com.ubuntu.music\",\"title\":\"Music\",\"version\":\"2.4.1003\",\"x-source\":{\"vcs-bzr\":\"lp:music-app\",\"vcs-bzr-revno\":\"1003\"},\"x-test\":{\"autopilot\":\"music_app\"},\"_directory\":\"/usr/share/click/preinstalled/.click/users/@all/com.ubuntu.music\",\"_removable\":1}]");
#endif

    auto loadDoc = QJsonDocument::fromJson(manifests.toUtf8());
    auto tmp = loadDoc.toJson();
    std::cout << tmp.constData();

    // XDG User Directories

    const std::array<QStandardPaths::StandardLocation,4> standard_locations = {
        QStandardPaths::DocumentsLocation,
        QStandardPaths::MoviesLocation,
        QStandardPaths::PicturesLocation,
        QStandardPaths::MusicLocation
    };

    const auto path_str = QString::fromUtf8("path");

    for (const auto& sl : standard_locations)
    {
        const auto name = QStandardPaths::displayName(sl);
        const auto locations = QStandardPaths::standardLocations(sl);
        if (locations.empty())
        {
            qWarning() << "unable to find path for"  << name;
        }
        else
        {
            uuid_t keyuu;
            uuid_generate(keyuu);
            char keybuf[37];
            uuid_unparse(keyuu, keybuf);
            const auto keystr = QString::fromUtf8(keybuf);

            Metadata m(keystr, name);
            m.set_property(path_str, locations.front());
            ret.push_back(m);
        }
    }

    return ret;
}

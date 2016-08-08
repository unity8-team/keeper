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
 *     Charles Kerr <charles.kerr@canonical.com>
 */

#include <helper/static-registry.h>

#include <QtGlobal>
#include <QDebug>
#include <QUrl>
#include <QUrlQuery>

class StaticRegistry::Impl
{
public:

    Impl() =default;
    ~Impl() =default;

    QStringList get_backup_helper_urls(Metadata const& task)
    {
        QStringList ret;

        if (task.has_property(Metadata::TYPE_KEY))
        {
            auto typestr = task.get_property(Metadata::TYPE_KEY).value<QString>();
            if (typestr == Metadata::USER_FOLDER_VALUE)
            {
                ret = get_folder_backup_helper_urls(task);
            }
            else
            {
                qCritical() << "unhandled type" << typestr;
            }
        }
        else
        {
            qCritical() << "task had no" << Metadata::TYPE_KEY << "property";
        }

        return ret;
    }

private:

    QStringList get_folder_backup_helper_urls(Metadata const& task)
    {
        QStringList ret;

#if 0 // proposal: push everything into the query in case we need to extend later

        QUrlQuery query;
        query.addQueryItem("QDBUS_DEBUG", "1");
        query.addQueryItem("G_DBUS_DEBUG", "call,message,signal,return");
        query.addQueryItem("cwd", subtype);

        QUrl url;
        url.setScheme("file");
        url.setPath(QString::fromUtf8(FOLDER_BACKUP_EXEC));
        url.setQuery(query);

        qDebug() << url.toDisplayString();
        qDebug() << url.toEncoded();
        ret.push_back(url.toEncoded());

#else // current impl: multiple urls with predefined meanings

        QString exec;
        QString cwd;

        // This is added for testing purposes only
        auto test_helper = qgetenv("KEEPER_TEST_HELPER");
        if (!test_helper.isEmpty())
        {
            qDebug() << "get_folder_backup_helper_urls: using KEEPER_TEST_HELPER '" << test_helper << "'";
            exec = test_helper;
        }
        else
        {
            exec = QString::fromUtf8(FOLDER_BACKUP_EXEC);
        }

        if (task.has_property(Metadata::PATH_KEY))
        {
            cwd = task.get_property(Metadata::PATH_KEY).value<QString>();
        }
        else if (task.has_property(Metadata::SUBTYPE_KEY))
        {
            cwd = task.get_property(Metadata::PATH_KEY).value<QString>();
        }
        else
        {
            qCritical() << "get_folder_backup_helper_urls: no subtype property found";
        }

        ret.push_back(exec);
        ret.push_back(cwd);

#endif

        return ret;
    }
};

/***
****
***/

StaticRegistry::StaticRegistry():
    impl_{new Impl{}}
{
}

StaticRegistry::~StaticRegistry()
{
}

QStringList
StaticRegistry::get_backup_helper_urls(Metadata const& task)
{
    return impl_->get_backup_helper_urls(task);
}

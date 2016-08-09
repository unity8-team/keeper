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

#include "helper/data-dir-registry.h"

#include <QtGlobal>
#include <QDebug>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QStandardPaths>
#include <QUrl>
#include <QUrlQuery>

#include <array>
#include <utility> // pair

class DataDirRegistry::Impl
{
public:

    Impl():
        registry_{}
    {
        load_registry();
    }

    ~Impl()
    {
    }

    QStringList get_backup_helper_urls(Metadata const& task)
    {
        QStringList ret;

        QString type;
        if (task.get_property(Metadata::TYPE_KEY, type))
        {
            auto it = registry_.find(std::make_pair(type,QStringLiteral("backup")));
            if (it == registry_.end())
            {
                qCritical() << "can't get backup helper urls for unhandled type" << type;
            }
            else
            {
                auto const& info = it.value();
                ret = perform_url_substitution(task, info.urls);
            }
        }
        else
        {
            qCritical() << "task had no" << Metadata::TYPE_KEY << "property";
        }

        return ret;
    }

private:

    // replace "${key}" with task.get_property("key")
    QStringList perform_url_substitution(Metadata const& task, QStringList const& urls_in)
    {
        std::array<QString,6> keys = {
            Metadata::TYPE_KEY,
            Metadata::SUBTYPE_KEY,
            Metadata::NAME_KEY,
            Metadata::PACKAGE_KEY,
            Metadata::TITLE_KEY,
            Metadata::VERSION_KEY
        };

        QStringList urls {urls_in};

        for (auto const& key : keys)
        {
            QString after;
            if (task.get_property(key, after))
            {
                QString before = QStringLiteral("${%1}").arg(key);
                for (auto& url : urls)
                    url.replace(before, after);
            }
        }

        for (auto const& url : urls_in)
            qDebug() << "in: " << url;
        for (auto const& url : urls)
            qDebug() << "out: " << url;

        return urls;
    }

    struct HelperInfo
    {
        QStringList urls;
    };

    // pair is type + action, e.g. "folder" + "backup"
    QMap<std::pair<QString,QString>,HelperInfo> registry_;

    void load_registry()
    {
        // find the registry file
        auto subpath = QStringLiteral(PROJECT_NAME) + QLatin1Char('/') + QStringLiteral(HELPER_REGISTRY);
        auto path = QStandardPaths::locate(QStandardPaths::GenericDataLocation, subpath);
        if (path.isEmpty())
        {
            qCritical() << "unable to find" << subpath;
        }
        else
        {
            qDebug() << "found helper registry at" << path;

            /**
             * Example json:
             *
             * {
             *     "folder": {
             *         "backup-urls": [
             *             "/path/to/helper.sh",
             *             "${subtype}"
             *         ]
             *     }
             * }
             */

            // read the file in
            QFile registry_file(path);
            registry_file.open(QFile::ReadOnly | QFile::Text);
            auto raw_json = registry_file.readAll();
            registry_file.close();

            // parse it into a json document
            QJsonParseError error;
            auto doc = QJsonDocument::fromJson(raw_json, &error);
            if (error.error != QJsonParseError::NoError)
                qCritical() << path << "parse error at offset" << error.offset << error.errorString();

            auto obj = doc.object();
            for (auto tit=obj.begin(), tend=obj.end(); tit!=tend; ++tit)
            {
                auto const type = tit.key();
                auto& info = registry_[std::make_pair(type,QStringLiteral("backup"))];

                auto const props = tit.value().toObject();
                auto const urls_jsonval = props["backup-urls"];
                if (urls_jsonval.isArray())
                {
                    for (auto url_jsonval : urls_jsonval.toArray())
                    {
                        info.urls.push_back(url_jsonval.toString());
                    }
                }

                qDebug() << "loaded" << type << "backup urls from" << path;
                for(auto const url : info.urls)
                    qDebug() << "\turl:" << url;
            }
        }
    }
};

/***
****
***/

DataDirRegistry::DataDirRegistry():
    impl_{new Impl{}}
{
}

DataDirRegistry::~DataDirRegistry()
{
}

QStringList
DataDirRegistry::get_backup_helper_urls(Metadata const& task)
{
    return impl_->get_backup_helper_urls(task);
}

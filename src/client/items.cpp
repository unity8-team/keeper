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
 *     Xavi Garcia Mena <xavi.garcia.mena@canonical.com>
 */

#include "client/keeper-items.h"

#include <QtDBus>
#include <QVariantMap>

namespace keeper
{
// json keys
constexpr const char PROPERTIES_KEY[]   = "properties";

// keys
const QString Item::UUID_KEY = QStringLiteral("uuid");
const QString Item::TYPE_KEY = QStringLiteral("type");
const QString Item::SUBTYPE_KEY = QStringLiteral("subtype");
const QString Item::NAME_KEY = QStringLiteral("name");
const QString Item::PACKAGE_KEY = QStringLiteral("package");
const QString Item::TITLE_KEY = QStringLiteral("title");
const QString Item::VERSION_KEY = QStringLiteral("version");
const QString Item::FILE_NAME_KEY = QStringLiteral("file-name");
const QString Item::DIR_NAME_KEY = QStringLiteral("dir-name");
const QString Item::DISPLAY_NAME_KEY = QStringLiteral("display-name");
const QString Item::STATUS_KEY = QStringLiteral("action");
const QString Item::ERROR_KEY = QStringLiteral("error");
const QString Item::PERCENT_DONE_KEY = QStringLiteral("percent-done");
const QString Item::SPEED_KEY = QStringLiteral("speed");


// values
const QString Item::FOLDER_VALUE = QStringLiteral("folder");
const QString Item::SYSTEM_DATA_VALUE = QStringLiteral("system-data");
const QString Item::APPLICATION_VALUE = QStringLiteral("application");

Item::Item() = default;

Item::Item(QVariantMap const & values)
    : QVariantMap(values)
{
}

Item::~Item() = default;

QVariant Item::get_property_value(QString const & property) const
{
    auto iter = this->find(property);
    if (iter != this->end())
    {
        return (*iter);
    }
    else
    {
        return QVariant();
    }
}

void Item::set_property_value(QString const& property, QVariant const& value)
{
    this->insert(property, value);
}

template<typename T> T Item::get_property(QString const & property, bool * valid) const
{
    auto it = this->find(property);

    if (it == this->end())
    {
        if (valid != nullptr)
            *valid = false;

        return T{};
    }

    if (valid != nullptr)
        *valid = true;

    return it->value<T>();
}

bool Item::is_valid()const
{
    // check for required properties
    for (auto& property : {TYPE_KEY, DISPLAY_NAME_KEY})
        if (!this->has_property(property))
            return false;

    return true;
}

bool Item::has_property(QString const & property) const
{
    return this->contains(property);
}

QString Item::get_uuid(bool *valid) const
{
    return get_property<QString>(UUID_KEY, valid);
}

QString Item::get_type(bool *valid) const
{
    return get_property<QString>(TYPE_KEY, valid);
}

QString Item::get_display_name(bool *valid) const
{
    return get_property<QString>(DISPLAY_NAME_KEY, valid);
}

QString Item::get_dir_name(bool *valid) const
{
    return get_property<QString>(DIR_NAME_KEY, valid);
}

QString Item::get_status(bool *valid) const
{
    return get_property<QString>(STATUS_KEY, valid);
}

double Item::get_percent_done(bool *valid) const
{
    return get_property<double>(PERCENT_DONE_KEY, valid);
}

keeper::Error Item::get_error(bool *valid) const
{
    auto it = this->find(ERROR_KEY);

    // if no error was sent, things must be OK
    if (it == this->end())
    {
        return keeper::Error::OK;
    }

    return keeper::convert_from_dbus_variant(*it, valid);
}

QString Item::get_file_name(bool *valid) const
{
    return get_property<QString>(FILE_NAME_KEY, valid);
}

void Item::registerMetaType()
{
    qRegisterMetaType<Item>("Item");

    qDBusRegisterMetaType<Item>();
}

/////
// Items
/////


Items::Items() =default;

Items::~Items() =default;


Items::Items(Error error)
    : error_{error}
{
}

QStringList Items::get_uuids() const
{
    return this->keys();
}

void Items::registerMetaType()
{
    qRegisterMetaType<Items>("Items");

    qDBusRegisterMetaType<Items>();

    Item::registerMetaType();
}

} // namespace keeper

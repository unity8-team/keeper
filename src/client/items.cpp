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

// Item

constexpr const char TYPE_KEY[]         = "type";
constexpr const char DISPLAY_NAME_KEY[] = "display-name";
constexpr const char DIR_NAME_KEY[]     = "dir-name";
constexpr const char STATUS_KEY[]       = "action";
constexpr const char PERCENT_DONE_KEY[] = "percent-done";
constexpr const char ERROR_KEY[]        = "error";


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

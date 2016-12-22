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

bool has_all_predefined_properties(QStringList const & predefined_properties, Item const & values);

bool has_all_predefined_properties(QStringList const & predefined_properties, Item const & values)
{
    for (auto iter = predefined_properties.begin(); iter != predefined_properties.end(); ++iter)
    {
        if (!values.has_property((*iter)))
            return false;
    }
    return true;
}

constexpr const char TYPE_KEY[]         = "type";
constexpr const char DISPLAY_NAME_KEY[] = "display-name";
constexpr const char DIR_NAME_KEY[]     = "dir-name";
constexpr const char STATUS_KEY[]       = "action";
constexpr const char PERCENT_DONE_KEY[] = "percent-done";
constexpr const char ERROR_KEY[]        = "error";


Item::Item()
    : QVariantMap()
{
}

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

bool Item::is_valid()const
{
    // we need at least type and display name to consider this a keeper item
    return has_all_predefined_properties(QStringList{TYPE_KEY, DISPLAY_NAME_KEY}, *this);
}

bool Item::has_property(QString const & property) const
{
    auto iter = this->find(property);
    return iter != this->end();
}

QString Item::get_type(bool *valid) const
{
    if (!has_property(TYPE_KEY))
    {
        if (valid)
            *valid = false;
        return QString();
    }
    if (valid)
        *valid = true;
    return get_property_value(TYPE_KEY).toString();
}

QString Item::get_display_name(bool *valid) const
{
    if (!has_property(DISPLAY_NAME_KEY))
    {
        if (valid)
            *valid = false;
        return QString();
    }
    if (valid)
        *valid = true;
    return get_property_value(DISPLAY_NAME_KEY).toString();
}

QString Item::get_dir_name(bool *valid) const
{
    if (!has_property(DIR_NAME_KEY))
    {
        if (valid)
            *valid = false;
        return QString();
    }
    if (valid)
        *valid = true;
    return get_property_value(DIR_NAME_KEY).toString();
}

QString Item::get_status(bool *valid) const
{
    if (!has_property(STATUS_KEY))
    {
        if (valid)
            *valid = false;
        return QString();
    }
    if (valid)
        *valid = true;
    return get_property_value(STATUS_KEY).toString();
}

double Item::get_percent_done(bool *valid) const
{
    auto value = get_property_value(PERCENT_DONE_KEY);
    if (value.type() == QVariant::Double)
    {
        if (valid)
            *valid = true;
        return get_property_value(PERCENT_DONE_KEY).toDouble();
    }
    else
    {
        if (valid)
            *valid = false;
        return -1;
    }
}

keeper::Error Item::get_error(bool *valid) const
{
    // if it does not have explicitly defined the error, OK is considered
    if (!has_property(ERROR_KEY))
        return keeper::Error::OK;

    return keeper::convert_from_dbus_variant(get_property_value(ERROR_KEY), valid);
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
#if 0
    QStringList ret;
    for (auto iter = this->begin(); iter != this->end(); ++iter)
    {
        ret << iter.key();
    }

    return ret;
#endif
}

void Items::registerMetaType()
{
    qRegisterMetaType<Items>("Items");

    qDBusRegisterMetaType<Items>();

    Item::registerMetaType();
}

} // namespace keeper

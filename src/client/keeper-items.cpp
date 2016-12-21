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

// KeeperItem

bool has_all_predefined_properties(QStringList const & predefined_properties, KeeperItem const & values);

bool has_all_predefined_properties(QStringList const & predefined_properties, KeeperItem const & values)
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


KeeperItem::KeeperItem()
    : QVariantMap()
{
}

KeeperItem::KeeperItem(QVariantMap const & values)
    : QVariantMap(values)
{
}

KeeperItem::~KeeperItem() = default;

QVariant KeeperItem::get_property_value(QString const & property) const
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

bool KeeperItem::is_valid()const
{
    // we need at least type and display name to consider this a keeper item
    return has_all_predefined_properties(QStringList{TYPE_KEY, DISPLAY_NAME_KEY}, *this);
}

bool KeeperItem::has_property(QString const & property) const
{
    auto iter = this->find(property);
    return iter != this->end();
}

QString KeeperItem::get_type(bool *valid) const
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

QString KeeperItem::get_display_name(bool *valid) const
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

QString KeeperItem::get_dir_name(bool *valid) const
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

QString KeeperItem::get_status(bool *valid) const
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

double KeeperItem::get_percent_done(bool *valid) const
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

keeper::KeeperError KeeperItem::get_error(bool *valid) const
{
    // if it does not have explicitly defined the error, OK is considered
    if (!has_property(ERROR_KEY))
        return keeper::KeeperError::OK;

    auto value = get_property_value(ERROR_KEY);

    if (value.typeName() != QStringLiteral("QDBusArgument"))
    {
        qWarning() << Q_FUNC_INFO
                   << " Error converting dbus QVariant to KeeperError, expected type is [ QDBusArgument ] and current type is: ["
                   << value.typeName() << "]";
        if (valid)
            *valid = false;
        return KeeperError(keeper::KeeperError::ERROR_UNKNOWN);
    }
    auto dbus_arg = value.value<QDBusArgument>();

    if (dbus_arg.currentSignature() != "(i)")
    {
        qWarning() << Q_FUNC_INFO
                   << " Error converting dbus QVariant to KeeperError, expected signature is \"(i)\" and current signature is: \""
                   << dbus_arg.currentSignature() << "\"";
        if (valid)
            *valid = false;
        return KeeperError(keeper::KeeperError::ERROR_UNKNOWN);
    }
    KeeperError ret;
    dbus_arg >> ret;

    if (valid)
        *valid = true;

    return ret;
}

void KeeperItem::registerMetaType()
{
    qRegisterMetaType<KeeperItem>("KeeperItem");

    qDBusRegisterMetaType<KeeperItem>();
}

/////
// KeeperItemsMap
/////


KeeperItemsMap::KeeperItemsMap()
    : QMap<QString, KeeperItem>()
{
}

KeeperItemsMap::~KeeperItemsMap() = default;


KeeperItemsMap::KeeperItemsMap(KeeperError error)
    : QMap<QString, KeeperItem>()
      , error_(error)
{

}

QStringList KeeperItemsMap::get_uuids() const
{
    QStringList ret;
    for (auto iter = this->begin(); iter != this->end(); ++iter)
    {
        ret << iter.key();
    }

    return ret;
}

keeper::KeeperError KeeperItemsMap::get_error() const
{
    return error_;
}

void KeeperItemsMap::registerMetaType()
{
    qRegisterMetaType<KeeperItemsMap>("KeeperItemsMap");

    qDBusRegisterMetaType<KeeperItemsMap>();

    KeeperItem::registerMetaType();
}

QDBusArgument &operator<<(QDBusArgument &argument, const KeeperItemsMap &result)
{
    argument.beginStructure();
    argument.beginMap( QVariant::String, qMetaTypeId<keeper::KeeperItem>() );
    for (auto iter = result.begin(); iter != result.end(); ++iter)
    {
        argument.beginMapEntry();
        argument << iter.key() << (*iter);
        argument.endMapEntry();
    }
    argument.endMap();
    argument << result.error_;
    argument.endStructure();

    return argument;
}

const QDBusArgument &operator>>(const QDBusArgument &argument, KeeperItemsMap &result)
{
    argument.beginStructure();
    argument.beginMap();
    result.clear();

    while(!argument.atEnd())
    {
        QString key;
        KeeperItem value;
        argument.beginMapEntry();
        argument >> key >> value;
        argument.endMapEntry();
        result[key] = value;
    }
    argument.endMap();

    argument >> result.error_;
    argument.endStructure();

    return argument;
}

} // namespace keeper

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
 *   Charles Kerr <charles.kerr@canonical.com>
 */

#include "helper/metadata.h"
#include "service/keeper.h"
#include "service/keeper-user.h"

#include <QDebug>
#include <QDBusMessage>
#include <QDBusConnection>

KeeperUser::KeeperUser(Keeper* keeper)
  : QObject(keeper)
  , keeper_(*keeper)
{
}

KeeperUser::~KeeperUser() =default;

namespace
{
    QVariantMap strings_to_variants(const QMap<QString,QString>& strings)
    {
        QVariantMap variants;

        for (auto it=strings.begin(), end=strings.end(); it!=end; ++it)
            variants.insert(it.key(), QVariant::fromValue(it.value()));

        return variants;
    }

    QVariantDictMap choices_to_variant_dict_map(const QVector<Metadata>& choices)
    {
        QVariantDictMap ret;

        for (auto const& metadata : choices)
            ret.insert(metadata.uuid(), strings_to_variants(metadata.get_public_properties()));

        return ret;
    }
}

QVariantDictMap
KeeperUser::GetBackupChoices()
{
    return choices_to_variant_dict_map(keeper_.get_backup_choices());
}

void
KeeperUser::StartBackup (const QStringList& keys)
{
    // FIXME: writeme

    qDebug() << keys;
    keeper_.start_tasks(keys);
}

void
KeeperUser::Cancel()
{
    // FIXME: writeme

    qDebug() << "hello world";
}

QVariantDictMap
KeeperUser::GetRestoreChoices()
{
    return choices_to_variant_dict_map(keeper_.get_restore_choices());
}

void
KeeperUser::StartRestore (const QStringList& keys)
{
    // FIXME: writeme

    qDebug() << keys;
}

QVariantDictMap
KeeperUser::get_state() const
{
    return keeper_.get_state();
}

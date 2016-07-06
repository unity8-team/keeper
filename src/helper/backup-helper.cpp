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
 *     Xavi Garcia <xavi.garcia.mena@canonical.com>
 */

#include <helper/backup-helper.h>
#include "internal/backup-helper-impl.h"

BackupHelper::BackupHelper(QString const &appid, QObject * parent) :
      QObject(parent)
    , p_(new internal::BackupHelperImpl(appid, parent))
{
    connect(p_.data(), &internal::BackupHelperImpl::started, this, &BackupHelper::started);
    connect(p_.data(), &internal::BackupHelperImpl::finished, this, &BackupHelper::finished);
}

BackupHelper::~BackupHelper()
{
}

void BackupHelper::start()
{
    p_->start();
}

void BackupHelper::stop()
{
    p_->stop();
}

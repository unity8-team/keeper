/*
 * Copyright 2016 Canonical Ltd.
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

#include "tests/utils/xdg-user-dirs-sandbox.h"

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QString>
#include <QTemporaryDir>

XdgUserDirsSandbox::XdgUserDirsSandbox()
{
    init();
}

void
XdgUserDirsSandbox::init()
{
    // create the sandbox dir

    temporary_dir_.reset(new QTemporaryDir());
    const auto top = QDir(temporary_dir_->path());

    // create the user dirs

    const struct {
        const char * key;
        const char * dirname;
    } user_dirs[] = {
        { "XDG_DESKTOP_DIR", "Desktop" },
        { "XDG_DOWNLOAD_DIR", "Downloads" },
        { "XDG_TEMPLATES_DIR", "Templates" },
        { "XDG_PUBLICSHARE_DIR", "Public" },
        { "XDG_DOCUMENTS_DIR", "Documents" },
        { "XDG_MUSIC_DIR", "Music" },
        { "XDG_PICTURES_DIR", "Pictures" },
        { "XDG_VIDEOS_DIR", "Videos" }
    };
    for(const auto& user_dir : user_dirs) {
        top.mkdir(QString::fromUtf8(user_dir.dirname));
        auto env_value = QStringLiteral("%1%2%3").arg(top.path()).arg(QDir::separator()).arg(user_dir.dirname);
        qputenv(user_dir.key, env_value.toUtf8());
    }

    // create user-dirs.dirs

    const auto config_dirname = QStringLiteral(".config");
    top.mkdir(config_dirname);
    auto config_dir = QDir(top.absoluteFilePath(config_dirname));
    static constexpr char const xdg_config_key[]{"XDG_CONFIG_HOME"};
    qputenv(xdg_config_key, config_dir.path().toUtf8());

    const auto dirs_filename = QStringLiteral("user-dirs.dirs");
    QFile dirs_file(config_dir.absoluteFilePath(dirs_filename));
    dirs_file.open(QIODevice::Text | QIODevice::WriteOnly);
    QString contents;
    for(const auto& user_dir : user_dirs) {
        contents += QStringLiteral("%1=\"%2/%3\"\n")
                        .arg(user_dir.key)
                        .arg(top.path())
                        .arg(user_dir.dirname);
    }
    dirs_file.write(contents.toUtf8());
    dirs_file.close();

    qDebug() << xdg_config_key << "sandbox created in" << top.path();
}

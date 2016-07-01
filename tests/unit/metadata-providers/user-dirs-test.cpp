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

#include "service/backup-choices.h"

#include <gtest/gtest.h>

#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QSignalSpy>
#include <QString>
#include <QTemporaryDir>

#include <cstdio>
#include <memory>

inline void PrintTo(const QString& s, std::ostream* os)
{
    *os << "\"" << s.toStdString() << "\"";
}

inline void PrintTo(const std::set<QString>& s, std::ostream* os)
{
    *os << "{ ";
    for (const auto& str : s)
        *os << '"' << str.toStdString() << "\", "; 
    *os << " }";
}

class UserDirProviderTest : public ::testing::Test
{
protected:

    virtual void SetUp() override
    {
        init_xdg_sandbox();
    }

    virtual void TearDown() override
    {
        temporary_dir_.reset();
    }

    void init_xdg_sandbox()
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
        for(const auto& user_dir : user_dirs)
            top.mkdir(QString::fromUtf8(user_dir.dirname));

        // create user-dirs.dirs

        const auto config_dirname = QStringLiteral(".config");
        top.mkdir(config_dirname);
        auto config_dir = QDir(top.absoluteFilePath(config_dirname));
        qDebug() << "config_dir is " << qPrintable(config_dir.path());
        qputenv("XDG_CONFIG_HOME", config_dir.path().toUtf8());

        const auto dirs_filename = QStringLiteral("user-dirs.dirs");
        QFile dirs_file(config_dir.absoluteFilePath(dirs_filename));
        ASSERT_TRUE(dirs_file.open(QIODevice::Text | QIODevice::WriteOnly));
        QString contents;
        for(const auto& user_dir : user_dirs) {
            contents += QStringLiteral("%1=\"%2/%3\"\n")
                            .arg(user_dir.key)
                            .arg(top.path())
                            .arg(user_dir.dirname);
        }
        dirs_file.write(contents.toUtf8());
        dirs_file.close();
    }

private:

    std::shared_ptr<QTemporaryDir> temporary_dir_;
};


TEST_F(UserDirProviderTest, UserDirs)
{
    BackupChoices tmp;
    const auto choices = tmp.get_backups();

    // confirm that choices has the advertised public properties
    const auto type_str = QStringLiteral("type");
    const auto icon_str = QStringLiteral("icon");
    for(const auto& choice : choices)
    {
        ASSERT_FALSE(choice.key().isEmpty());
        ASSERT_FALSE(choice.display_name().isEmpty());
        ASSERT_TRUE(choice.has_property(type_str));
        EXPECT_TRUE(choice.has_property(icon_str));
    }

    // confirm that we have a system-data choice
    int i, n;
    for(i=0, n=choices.size(); i<n; ++i)
        if (choices[i].get_property(type_str) == QStringLiteral("system-data"))
            break;
    ASSERT_TRUE(i != n);
    auto system_data = choices[i];
    EXPECT_TRUE(system_data.has_property(icon_str));

    // confirm that we have user-dir choices
    std::set<QString> expected_user_dir_display_names = {
        QStringLiteral("Documents"),
        QStringLiteral("Movies"),
        QStringLiteral("Music"),
        QStringLiteral("Pictures")
    };
    std::set<QString> user_dir_display_names;
    for (const auto& choice : choices)
        if (choice.get_property(type_str) == QStringLiteral("folder"))
            user_dir_display_names.insert(choice.display_name());
    EXPECT_EQ(expected_user_dir_display_names, user_dir_display_names);
}

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
#include "tar/tar-creator.h"

#include <gtest/gtest.h>

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QString>

#include <cstdio>
#include <memory>

#if 0
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
#endif

class TarCreatorFixture: public ::testing::Test
{
protected:

    virtual void SetUp() override
    {
        user_dirs_sandbox_.reset(new XdgUserDirsSandbox());
    }

    virtual void TearDown() override
    {
        user_dirs_sandbox_.reset();
    }

private:

    std::shared_ptr<XdgUserDirsSandbox> user_dirs_sandbox_;
};


TEST_F(TarCreatorFixture, HelloWorld)
{
}

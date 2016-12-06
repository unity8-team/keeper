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

#include "tar/untar.h"

#include <QDebug>
#include <QProcess>
#include <QString>

#include <string>
#include <vector>

class Untar::Impl
{
public:

    explicit Impl(std::string const& path)
        : path_{path}
    {
        uncompress_.setStandardOutputProcess(&untar_);
        uncompress_.start("xz", QStringList{ "--decompress", "--stdout", "--force" });

        untar_.start("tar", QStringList{ "-xv", "-C", path_.c_str()});
        untar_.setProcessChannelMode(QProcess::ForwardedChannels);
    }

    ~Impl()
    {
        uncompress_.closeWriteChannel();
        uncompress_.waitForFinished();
        untar_.waitForFinished();
    }

    bool step(std::vector<char>& input)
    {
        bool success = true;

        auto n_left = input.size();
        auto buf = &input.front();
        while (n_left > 0)
        {
            auto const n_written_this_pass = uncompress_.write(buf, n_left);
            if (n_written_this_pass == -1) {
                qCritical() << Q_FUNC_INFO << strerror(errno);
                success = false;
                break;
            } else {
                n_left -= n_written_this_pass;
                buf += n_written_this_pass;
            }
        }

        return success;
    }

private:

    std::string const path_;
    QProcess uncompress_;
    QProcess untar_;
};

/**
***
**/

Untar::Untar(std::string const& path)
    : impl_{new Impl{path}}
{
}

Untar::~Untar() =default;

bool
Untar::step(std::vector<char>& input)
{
    return impl_->step(input);
}

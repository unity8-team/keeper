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
        finish();
    }

    bool step(char const * buf, size_t buflen)
    {
        bool success = true;

        auto n_left = buflen;
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

    bool finish ()
    {
        bool ok = true;

        uncompress_.closeWriteChannel();
        if (!finish(uncompress_, "xz"))
            ok = false;

        if (!finish(untar_, "untar"))
            ok = false;

        return ok;
    }

private:

    bool finish (QProcess& proc, QString const& name)
    {
        if (proc.state() != QProcess::NotRunning)
        {
            proc.waitForFinished();
        }

        bool ok;

        if (proc.state() != QProcess::NotRunning)
        {
            qCritical() << name << "did not finish";
            ok = false;
        }
        else if (proc.exitStatus() != QProcess::NormalExit)
        {
            qCritical() << name << "exited abnormally";
            ok = false;
        }
        else if (proc.exitCode() != 0)
        {
            qCritical() << name << "exited with error code" << proc.exitCode();
            ok = false;
        }
        else
        {
            qDebug() << name << "finished ok";
            ok = true;
        }

        return ok;
    }

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
Untar::step(char const * buf, size_t buflen)
{
    return impl_->step(buf, buflen);
}

bool
Untar::finish()
{
    return impl_->finish();
}

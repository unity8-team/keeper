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

#include "tests/utils/dummy-file.h"

#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QString>
#include <QTemporaryFile>

DummyFile::Info
DummyFile::create(const QDir& dir, qint64 filesize)
{
    // NB we want to exercise long filenames, but this cutoff length is arbitrary
    static constexpr int MAX_BASENAME_LEN {200};
    int filename_len = qrand() % MAX_BASENAME_LEN;
    QString basename;
    for (int i=0; i<filename_len; ++i)
        basename += ('a' + char(qrand() % ('z'-'a'));
    basename += QStringLiteral("-XXXXXX");
    auto template_name = dir.absoluteFilePath(basename);

    // fill the file with noise
    QTemporaryFile f(template_name);
    f.setAutoRemove(false);
    f.open();
    static constexpr qint64 max_step = 1024;
    char buf[max_step];
    qint64 left = filesize;
    while(left > 0)
    {
        int this_step = std::min(max_step, left);
        for(int i=0; i<this_step; ++i)
            buf[i] = 'a' + char(qrand() % ('z'-'a'));
        f.write(buf, this_step);
        left -= this_step;
    }
    f.close();

    // get a checksum
    f.open();
    QCryptographicHash hash(QCryptographicHash::Sha1);
    hash.addData(&f);
    const auto checksum = hash.result();
    f.close();

    DummyFile::Info info;
    info.info = QFileInfo(f.fileName());
    info.checksum = checksum;
    return info;
}

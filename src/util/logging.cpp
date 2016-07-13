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
 * Author: Pete Woods <pete.woods@canonical.com>
 */

#include "util/logging.h"

#include <QDir>

namespace util
{
    void
    loggingFunction (QtMsgType type, const QMessageLogContext &context,
                     const QString &msg)
    {
        const QByteArray localMsg = msg.toLocal8Bit();
        const std::string relative_file = QDir(CMAKE_SOURCE_DIR).relativeFilePath(context.file).toStdString();

        switch (type)
        {
            case QtMsgType::QtDebugMsg:
                fprintf (stderr, "[debug] %s (%s:%d)\n",
                         localMsg.constData(), relative_file.c_str(), context.line);
                break;
#if QT_VERSION >= QT_VERSION_CHECK(5, 5, 0)
            case QtMsgType::QtInfoMsg:
                fprintf (stderr, "[info] %s (%s:%d)\n",
                         localMsg.constData(), relative_file.c_str(), context.line);
                break;
#endif
            case QtMsgType::QtWarningMsg:
                fprintf (stderr, "[WARN] %s (%s:%d)\n",
                         localMsg.constData(), relative_file.c_str(), context.line);
                break;
            case QtMsgType::QtCriticalMsg:
                fprintf (stderr, "[CRITICAL]  %s (%s:%d)\n",
                         localMsg.constData(), relative_file.c_str(), context.line);
                break;
            case QtMsgType::QtFatalMsg:
                fprintf (stderr, "[FATAL] %s (%s:%d)\n",
                         localMsg.constData(), relative_file.c_str(), context.line);
                abort ();
        }
    }
}

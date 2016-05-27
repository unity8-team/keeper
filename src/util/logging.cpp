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

namespace util
{
    void
    loggingFunction (QtMsgType type, const QMessageLogContext &context,
                     const QString &msg)
    {
        QByteArray localMsg = msg.toLocal8Bit ();
        switch (type)
        {
            case QtMsgType::QtDebugMsg:
                fprintf (stderr, "Debug: %s (%s:%d, %s)\n",
                         localMsg.constData (), context.file, context.line,
                         context.function);
                break;
#if QT_VERSION >= QT_VERSION_CHECK(5, 5, 0)
            case QtMsgType::QtInfoMsg:
                fprintf (stderr, "Info: %s (%s:%d, %s)\n",
                         localMsg.constData (), context.file, context.line,
                         context.function);
                break;
#endif
            case QtMsgType::QtWarningMsg:
                fprintf (stderr, "Warning: %s (%s:%d, %s)\n",
                         localMsg.constData (), context.file, context.line,
                         context.function);
                break;
            case QtMsgType::QtCriticalMsg:
                fprintf (stderr, "Critical: %s (%s:%d, %s)\n",
                         localMsg.constData (), context.file, context.line,
                         context.function);
                break;
            case QtMsgType::QtFatalMsg:
                fprintf (stderr, "Fatal: %s (%s:%d, %s)\n",
                         localMsg.constData (), context.file, context.line,
                         context.function);
                abort ();
        }
    }
}

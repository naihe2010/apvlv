/*
 * This file is part of the apvlv package
 * Copyright (C) <2024> Alf
 *
 * Contact: Alf <naihe2010@126.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */
/* @CPPFILE ApvlvLog.cc
 *
 *  Author: Alf <naihe2010@126.com>
 */

#include <QFile>
#include <QFileInfo>
#include <QLoggingCategory>
#include <QTime>
#include <iostream>
#include <string>

#include "ApvlvLog.h"

namespace apvlv
{
ApvlvLog *ApvlvLog::mInstance = nullptr;

ApvlvLog::ApvlvLog (const QString &path)
{
  using FileFlag = QIODevice::OpenModeFlag;
  if (!path.isEmpty ())
    {
      mFile.setFileName (path);
      if (mFile.open (FileFlag::Text | FileFlag::WriteOnly | FileFlag::Append)
          == false)
        {
          cerr << "Open log file: " << path.toStdString ()
               << "error: " << mFile.errorString ().toStdString ()
               << std::endl;
          return;
        }

      mTextStream.setDevice (&mFile);
    }

  mInstance = this;

  QLoggingCategory::setFilterRules ("qt.*=false\n"
                                    "default.debug=true\n"
                                    "default.*=true");
  qInstallMessageHandler (ApvlvLog::logMessage);
}

void
ApvlvLog::writeMessage (const QString &msg)
{
#ifdef _DEBUG
  std::cout << msg.toStdString () << endl;
#endif

  auto endstr = "\n";
#ifdef WIN32
  endstr = "\r\n";
#endif
  if (mTextStream.status () == QTextStream::Ok)
    {
      mTextStream << msg << endstr;
    }
}

ApvlvLog::~ApvlvLog ()
{
  if (mFile.isOpen ())
    {
      mFile.close ();
    }
}

ApvlvLog *
ApvlvLog::instance ()
{
  return mInstance;
}

void
ApvlvLog::logMessage (QtMsgType type, const QMessageLogContext &context,
                      const QString &msg)
{
  auto now = QTime::currentTime ();
  auto nowstr = now.toString ("hh:mm:ss.zzz");
  QString log = nowstr + " ";

  if (context.file)
    {
      auto filename = QFileInfo (context.file).fileName ().toStdString ();
      log += QString::asprintf ("%s:%d ", filename.c_str (), context.line);
      log += QString::asprintf ("%s ", context.function);
    }
  log += msg;

  ApvlvLog::instance ()->writeMessage (log);
}
}

// Local Variables:
// mode: c++
// End:

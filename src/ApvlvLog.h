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
/* @CPPFILE ApvlvLog.h
 *
 *  Author: Alf <naihe2010@126.com>
 */
#ifndef _APVLV_LOG_H_
#define _APVLV_LOG_H_ 1

#include <QFile>
#include <QString>
#include <QTextStream>
#include <QtMessageHandler>
#include <memory>
#include <mutex>

namespace apvlv
{

class ApvlvLog
{
public:
  explicit ApvlvLog (const QString &path = "");
  ApvlvLog (const ApvlvLog &) = delete;
  ~ApvlvLog ();

  static ApvlvLog *instance ();
  static void logMessage (QtMsgType, const QMessageLogContext &,
                          const QString &);

private:
  void writeMessage (const QString &log);

  QFile mFile;
  QTextStream mTextStream;
  std::mutex mMutex;

  static ApvlvLog *mInstance;
};

};

#endif

/* Local Variables: */
/* mode: c++ */
/* End: */

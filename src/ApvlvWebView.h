/*
 * This file is part of the apvlv package
 *
 * Copyright (C) 2008 Alf.
 *
 * Contact: Alf <naihe2010@126.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2.0 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */
/* @CPPFILE ApvlvWebView.h
 *
 *  Author: Alf <naihe2010@126.com>
 */
/* @date Created: 2024/07/25 00:00:00 Alf */

#ifndef _APVLV_WEBVIEW_H_
#define _APVLV_WEBVIEW_H_

#include <QWebEngineProfile>
#include <QWebEngineUrlRequestJob>
#include <QWebEngineUrlSchemeHandler>
#include <QWebEngineView>
#include <memory>
#include <string>

namespace apvlv
{
using namespace std;

class ApvlvFile;
class ApvlvSchemeHandler : public QWebEngineUrlSchemeHandler
{
  Q_OBJECT
public:
  void
  setFile (ApvlvFile *file)
  {
    mFile = file;
  }
  void requestStarted (QWebEngineUrlRequestJob *job) override;

private:
  ApvlvFile *mFile;

signals:
  void webpageUpdated (const string &key);
};

class ApvlvWebview : public QWebEngineView
{
  Q_OBJECT
public:
  ApvlvWebview ();
  void
  setFile (ApvlvFile *file)
  {
    mSchemeHandler->setFile (file);
  }

signals:
  void webpageUpdated (const string &);

private slots:
  void
  webview_update (const string &msg)
  {
    emit webpageUpdated (msg);
  };

private:
  unique_ptr<QWebEnginePage> mPage;
  unique_ptr<ApvlvSchemeHandler> mSchemeHandler;
};

}

#endif

/* Local Variables: */
/* mode: c++ */
/* End: */

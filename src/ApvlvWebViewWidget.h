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
/* @CPPFILE ApvlvWebViewWidget.h
 *
 *  Author: Alf <naihe2010@126.com>
 */

#ifndef _APVLV_WEBVIEW_WIDGET_H_
#define _APVLV_WEBVIEW_WIDGET_H_

#include <QBuffer>
#include <QByteArray>
#include <QWebEngineFindTextResult>
#include <QWebEngineProfile>
#include <QWebEngineUrlRequestJob>
#include <QWebEngineUrlSchemeHandler>
#include <QWebEngineView>
#include <memory>
#include <string>

#include "ApvlvFileWidget.h"

namespace apvlv
{
class File;
class ApvlvSchemeHandler : public QWebEngineUrlSchemeHandler
{
  Q_OBJECT
public:
  void
  setFile (File *file)
  {
    mFile = file;
  }
  void requestStarted (QWebEngineUrlRequestJob *job) override;

private:
  File *mFile;
  QByteArray mArray;
  QBuffer mBuffer;

signals:
  void webpageUpdated (const string &key);
};

class WebView : public QWebEngineView
{
  Q_OBJECT
public:
  WebView ();
  void
  setFile (File *file)
  {
    mSchemeHandler->setFile (file);
  }

private:
  unique_ptr<QWebEnginePage> mPage;
  unique_ptr<ApvlvSchemeHandler> mSchemeHandler;

  bool isScrolledToTop ();
  bool isScrolledToBottom ();

  friend class WebViewWidget;
};

class WebViewWidget : public FileWidget
{
  Q_OBJECT
public:
  QWidget *createWidget () override;

  void
  setFile (File *file) override
  {
    mFile = file;
    if (mWidget == nullptr)
      mWidget = createWidget ();
    auto widget = dynamic_cast<WebView *> (mWidget);
    widget->setFile (mFile);
  }

  void showPage (int, double s) override;
  void showPage (int, const string &anchor) override;

  void scroll (int times, int w, int h) override;
  void scrollTo (double x, double y) override;

  void scrollUp (int times) override;
  void scrollDown (int times) override;
  void scrollLeft (int times) override;
  void scrollRight (int times) override;

  void setSearchStr (const string &str) override;
  void setSearchSelect (int select) override;

private:
  bool mIsScrollUp{ false };
  QWebEngineFindTextResult mSearchResult;

signals:
  void webpageUpdated (const string &msg);

private slots:
  void
  webview_update (const string &msg)
  {
    emit webpageUpdated (msg);
  };

  void webview_load_finished (bool suc);
  void webview_find_text_finished (const QWebEngineFindTextResult &result);
};

}

#endif

/* Local Variables: */
/* mode: c++ */
/* End: */

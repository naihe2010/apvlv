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
/* @CPPFILE ApvlvCore.h
 *
 *  Author: Alf <naihe2010@126.com>
 */
/* @date Created: 2008/09/30 00:00:00 Alf */

#ifndef _APVLV_CORE_H_
#define _APVLV_CORE_H_

#include <QBoxLayout>
#include <QFileSystemWatcher>
#include <QScrollArea>
#include <QScrollBar>
#include <QSplitter>
#include <iostream>
#include <map>

#include "ApvlvContent.h"
#include "ApvlvFile.h"
#include "ApvlvUtil.h"

using namespace std;

namespace apvlv
{
const int APVLV_DEFAULT_CONTENT_WIDTH = 200;
const int APVLV_WORD_WIDTH_DEFAULT = 40;
const int APVLV_LINE_HEIGHT_DEFAULT = 15;

class ApvlvCore;
class ApvlvStatus : public QFrame
{
  Q_OBJECT
public:
  ApvlvStatus ();

  ~ApvlvStatus () override = default;

  void setActive (bool act);

  void showMessages (const vector<string> &msgs);
};

class ApvlvView;
class ApvlvCore : public QFrame
{
  Q_OBJECT
public:
  explicit ApvlvCore (ApvlvView *);

  virtual ~ApvlvCore ();

  virtual bool reload ();

  virtual void inuse (bool use);

  virtual bool inuse ();

  virtual ApvlvCore *copy ();

  virtual ApvlvFile *file ();

  virtual void setDirIndex (const string &path);

  virtual bool loadfile (const string &file, bool check, bool show_content);

  virtual const char *filename ();

  virtual bool writefile (const char *);

  virtual int pagenumber ();

  virtual void showpage (int, double s);
  virtual void showpage (int, const string &anchor);
  virtual void refresh ();

  virtual double zoomvalue ();

  virtual void setActive (bool act);

  virtual double scrollrate ();

  virtual bool scrollto (double s);

  void scrollweb (int times, int w, int h);
  void scrollwebto (double xrate, double yrate);

  virtual void scrollup (int times);
  virtual void scrolldown (int times);
  virtual void scrollleft (int times);
  virtual void scrollright (int times);

  virtual void scrollupweb (int times);
  virtual void scrolldownweb (int times);
  virtual void scrollleftweb (int times);
  virtual void scrollrightweb (int times);

  bool webIsScrolledToTop ();
  bool webIsScrolledToBottom ();

  virtual bool usecache ();

  virtual void usecache (bool use);

  virtual void display ();

  virtual bool print (int ct);

  virtual bool totext (const char *name);

  virtual bool rotate (int ct);

  virtual void markposition (char s);

  virtual void setzoom (const char *z);

  virtual void jump (char s);

  virtual void nextpage (int times);

  virtual void prepage (int times);

  virtual void halfnextpage (int times);

  virtual void halfprepage (int times);

  virtual bool search (const char *str, bool reverse);

  virtual bool find (const char *str);

  virtual void gotolink (int ct);

  virtual void returnlink (int ct);

  virtual int getskip ();
  virtual void setskip (int ct);

  virtual void toggleContent ();

  virtual void toggleContent (bool enabled);

  virtual bool toggledControlContent (bool is_right);

  virtual bool isShowContent ();

  virtual bool isControlledContent ();

  virtual returnType process (int has, int times, uint keyval) = 0;

  ApvlvView *mView;

  static ApvlvCore *findByWidget (QWidget *widget);

protected:
  ApvlvFile *mFile{};

  ApvlvFileIndex mDirIndex{};

  bool mAutoScrollPage{};
  bool mAutoScrollDoc{};
  bool mContinuous{};

  bool mReady;

  bool mInuse;

  unique_ptr<QFileSystemWatcher> mWatcher;

  string mFilestr;

  uint mProCmd;

  int mSearchPagenum{};
  char mSearchCmd{};
  bool mSearchReverse{};
  uint mSearchSelect{};
  unique_ptr<ApvlvPoses> mSearchResults;
  string mSearchStr;

  enum
  {
    NORMAL,
    FITWIDTH,
    FITHEIGHT,
    CUSTOM
  } mZoommode;

  double mZoomrate{};

  bool mZoominit{};

  int mRotatevalue;

  bool mAdjInchg{};

  int mPagenum{};

  string mAnchor{};

  int mSkip{};

  double mPagex{}, mPagey{};

  DISPLAY_TYPE mDisplayType;

  // the main menubar
  QBoxLayout *mVbox;

  // the main panel
  QSplitter *mPaned;

  // content panel
  ApvlvContent *mContent;
  QScrollArea *mMainImageScrolView;

  // the webview
  QWebEngineView *mMainWebView;

  QScrollBar *mMainVaj, *mMainHaj;

  // the document scrolled window
  QFrame *mMainImageFrame;

  unique_ptr<QWebEngineProfile> mWebProfile;

  bool mWebScrollUp;

  // if active
  bool mActive{};

  // status bar
  ApvlvStatus *mStatus;

protected:
  void initImageUi ();

  void initWebUi ();

  void showImage ();

  void showWeb ();

signals:
  void indexGenerited (const ApvlvFileIndex &);

private slots:
  void webview_update (const string &key);
  void webview_load_finished (bool suc);
  void webview_context_menu_cb (const QPoint &);
};
}

#endif

/* Local Variables: */
/* mode: c++ */
/* End: */

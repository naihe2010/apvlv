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

#include "ApvlvContent.h"
#include "ApvlvFile.h"
#include "ApvlvUtil.h"

#include <gtk/gtk.h>

#include <iostream>
#include <map>

using namespace std;

namespace apvlv
{
const int APVLV_DEFAULT_CONTENT_WIDTH = 200;
const int APVLV_WORD_WIDTH_DEFAULT = 40;
const int APVLV_LINE_HEIGHT_DEFAULT = 15;

class ApvlvCore;
class ApvlvStatus
{
public:
  ApvlvStatus ();

  ~ApvlvStatus ();

  GtkWidget *widget ();

  void active (bool act);

  void show (const vector<string> &msgs);

protected:
  GtkWidget *mHbox;
};

class ApvlvView;
class ApvlvCore
{
public:
  explicit ApvlvCore (ApvlvView *);

  virtual ~ApvlvCore ();

  virtual bool reload ();

  virtual void inuse (bool use);

  virtual bool inuse ();

  virtual GtkWidget *widget ();

  virtual ApvlvCore *copy ();

  virtual ApvlvFile *file ();

  virtual void setDirIndex (const gchar *path);

  virtual bool loadfile (const char *file, bool check, bool show_content);

  virtual const char *filename ();

  virtual bool writefile (const char *);

  virtual gint pagenumber ();

  virtual void showpage (gint, gdouble s);
  virtual void showpage (gint, const string &anchor);
  virtual void refresh ();

  virtual gdouble zoomvalue ();

  virtual void setactive (bool act);

  virtual gdouble scrollrate ();

  virtual gboolean scrollto (double s);

  virtual void scrollup (int times);
  virtual void scrolldown (int times);
  virtual void scrollleft (int times);
  virtual void scrollright (int times);

  virtual bool usecache ();

  virtual void usecache (bool use);

  virtual void show ();

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

  virtual returnType process (int has, int times, guint keyval) = 0;

  ApvlvView *mView;

protected:
  ApvlvFile *mFile{};

  ApvlvFileIndex *mDirIndex{};

  bool mAutoScrollPage{};
  bool mAutoScrollDoc{};
  bool mContinuous{};

  bool mReady;

  bool mInuse;

  GFile *mGFile{};
  GFileMonitor *mGMonitor{};

  string mFilestr;

  guint mProCmd;

  double mScrollvalue{};

  int mSearchPagenum{};
  char mSearchCmd{};
  bool mSearchReverse{};
  guint mSearchSelect{};
  ApvlvPoses *mSearchResults;
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

  // the main menubar
  GtkWidget *mVbox;

  // the main panel
  GtkWidget *mPaned;

  // the content paned
  GtkWidget *mContentWidget;
  GtkAdjustment *mContentVaj;

  // the document scrolled window
  GtkWidget *mMainWidget;
  GtkAdjustment *mMainVaj, *mMainHaj;

  // if active
  bool mActive{};

  // content panel
  ApvlvContent *mContent;

  // status bar
  ApvlvStatus *mStatus;

private:
};
}

#endif

/* Local Variables: */
/* mode: c++ */
/* End: */

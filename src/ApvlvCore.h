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

#include "ApvlvFile.h"
#include "ApvlvUtil.h"

#include <gtk/gtk.h>

#include <iostream>
#include <map>

using namespace std;

namespace apvlv
{
    class ApvlvCore;
    class ApvlvCoreStatus {
     public:
      ApvlvCoreStatus ();

      virtual ~ ApvlvCoreStatus ();

      virtual GtkWidget *widget ();

      virtual void active (bool act);

      virtual void setsize (int w, int h);

      virtual void show (bool mContinuous);

     protected:
      GtkWidget *mHbox;
    };

    class ApvlvView;
    class ApvlvCore {
     public:
      explicit ApvlvCore (ApvlvView *);

      virtual ~ ApvlvCore ();

      virtual bool reload ();

      virtual void inuse (bool use);

      virtual bool inuse ();

      virtual int type ();

      virtual GtkWidget *widget ();

      virtual ApvlvCore *copy ();

      virtual ApvlvFile *file ();

      virtual bool loadfile (const char *file, bool check);

      virtual const char *filename ();

      virtual bool writefile (const char *);

      virtual gint pagenumber ();

      virtual void showpage (gint, gdouble s);
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

      virtual void gotolink (int ct);

      virtual void returnlink (int ct);

      virtual int getskip ();
      virtual void setskip (int ct);

      virtual void setsize (int wid, int hei);

      virtual returnType process (int has, int times, guint keyval);

      ApvlvView *mView;

     protected:
      ApvlvFile *mFile{};

      bool mAutoScrollPage{};
      bool mAutoScrollDoc{};
      bool mContinuous{};

      bool mReady;

      bool mInuse;

      int mType{};

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

      enum {
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

      int mSkip{};

      double mPagex{}, mPagey{};

      double mVrate{}, mHrate{};

      int mLines{}, mChars{};

      int mWidth{}, mHeight{};

      GtkAdjustment *mVaj, *mHaj;

      // the main widget
      GtkWidget *mVbox;

      // the document scrolled window
      GtkWidget *mScrollwin;

      // if active
      bool mActive{};

      // status bar
      ApvlvCoreStatus *mStatus{};

     private:
    };
}

#endif

/* Local Variables: */
/* mode: c++ */
/* End: */

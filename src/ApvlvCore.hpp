/*
 * This file is part of the apvlv package
 *
 * Copyright (C) 2008 Alf.
 *
 * Contact: Alf <naihe2010@gmail.com>
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
/* @PPCFILE ApvlvCore.hpp
 *
 *  Author: Alf <naihe2010@gmail.com>
 */
/* @date Created: 2008/09/30 00:00:00 Alf */

#ifndef _APVLV_CORE_H_
#define _APVLV_CORE_H_

#ifdef HAVE_CONFIG_H
# include "config.hpp"
#endif

#include "ApvlvUtil.hpp"

#include <gtk/gtk.h>

#include <iostream>
#include <map>

using namespace std;

namespace apvlv
{
  class ApvlvCore;
  class ApvlvCoreStatus
  {
  public:
    ApvlvCoreStatus ();

    virtual ~ ApvlvCoreStatus ();

    virtual GtkWidget *widget ();

    virtual void active (bool act);

    virtual void setsize (int w, int h);

    virtual void show ();

  protected:
      GtkWidget * mHbox;
  };

  class ApvlvCore
  {
  public:
    ApvlvCore ();

    virtual ~ ApvlvCore ();

    virtual void inuse (bool use);

    virtual bool inuse ();

    virtual int type ();

    virtual GtkWidget *widget ();

    virtual ApvlvCore *copy ();

    virtual bool loadfile (const char *file, bool check = true);

    virtual const char *filename ();

    virtual gint getrotate ();

    virtual gint pagenumber ();

    virtual gint pagesum ();

    virtual void showpage (gint, gdouble);
    virtual void refresh ();

    virtual gdouble zoomvalue ();

    virtual void setzoom (const char *s);
    virtual void setzoom (double d);

    virtual void zoomin ();
    virtual void zoomout ();

    virtual void setactive (bool act);

    virtual gdouble scrollrate ();

    virtual gboolean scrollto (double s);

    virtual void scrollup (int times);
    virtual void scrolldown (int times);
    virtual void scrollleft (int times);
    virtual void scrollright (int times);

    virtual void setsize (int wid, int hei);

    virtual returnType process (int times, guint keyval);

  protected:
      bool mReady;

    bool mInuse;

    int mType;

    string mFilestr;

    guint mProCmd;

    double mScrollvalue;

    double mSearchZoom;
    guint mSearchSelect;
    GList *mSearchResults;
    string mSearchstr;

    enum
    {
      NORMAL,
      FITWIDTH,
      FITHEIGHT,
      CUSTOM
    } mZoommode;

    double mZoomrate;

    bool mZoominit;

    int mRotatevalue;

    bool mAdjInchg;

    int mPagenum;

    double mPagex, mPagey;

    double mVrate, mHrate;

    int mLines, mChars;

    int mWidth, mHeight;

    GtkAdjustment *mVaj, *mHaj;

    // the main widget
    GtkWidget *mVbox;

    // the document scrolled window
    GtkWidget *mScrollwin;

    // if active
    bool mActive;

    // status bar
    ApvlvCoreStatus *mStatus;
  };
}

#endif

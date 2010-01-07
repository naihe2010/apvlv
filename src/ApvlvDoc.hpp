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
/* @PPCFILE ApvlvDoc.hpp
 *
 *  Author: Alf <naihe2010@gmail.com>
 */
/* @date Created: 2008/09/30 00:00:00 Alf */

#ifndef _APVLV_DOC_H_
#define _APVLV_DOC_H_

#ifdef HAVE_CONFIG_H
# include "config.hpp"
#endif

#include "ApvlvFile.hpp"
#include "ApvlvCore.hpp"
#include "ApvlvUtil.hpp"

#include <gtk/gtk.h>
#include <stdlib.h>

#include <iostream>
#include <vector>
#include <map>
#include <list>

using namespace std;

namespace apvlv
{
  struct PrintData
  {
    ApvlvFile *file;
    guint frmpn, endpn;
  };

  struct ApvlvDocPosition
  {
    int pagenum;
    double scrollrate;
  };

  typedef map < char, ApvlvDocPosition > ApvlvDocPositionMap;

  class ApvlvDoc;
  class ApvlvDocCache
  {
  public:
    ApvlvDocCache (ApvlvFile *);

    ~ApvlvDocCache ();

    void set (guint p, double zm, guint rot, bool delay = true);

    static void load (ApvlvDocCache *);

    PopplerPage *getpage ();

    guint getpagenum ();

    guchar *getdata (bool wait);

    GdkPixbuf *getbuf (bool wait);

    double getwidth ();

    double getheight ();

    ApvlvLinks *getlinks ();

  private:
      ApvlvFile * mFile;
    ApvlvLinks *mLinks;
    double mZoom;
    double mRotate;
    gint mPagenum;
    guchar *mData;
    gint mSize;
    GdkPixbuf *mBuf;
    gint mWidth;
    gint mHeight;
  };

  class ApvlvDocStatus:public ApvlvCoreStatus
  {
  public:
    ApvlvDocStatus (ApvlvDoc *);

    ~ApvlvDocStatus ();

    void active (bool act);

    void setsize (int, int);

    void show ();

  private:
      ApvlvDoc * mDoc;
#define AD_STATUS_SIZE   4
    GtkWidget *mStlab[AD_STATUS_SIZE];
  };

  class ApvlvDoc:public ApvlvCore
  {
  public:
    ApvlvDoc (int w, int h, const char *zm = "NORMAL", bool cache = false);

     ~ApvlvDoc ();

    void setactive (bool act);

    bool hascontent ();

    ApvlvDoc *copy ();

    bool usecache ();

    void usecache (bool use);

    bool loadfile (string & filename, bool check = true);

    bool loadfile (const char *src, bool check = true);

    bool print (int ct);

    bool totext (const char *name);

    bool rotate (int ct = 90);

    void markposition (const char s);

    void setzoom (const char *z);

    void jump (const char s);

    void showpage (int p, double s = 0.00);

    void nextpage (int times = 1);

    void prepage (int times = 1);

    void halfnextpage (int times = 1);

    void halfprepage (int times = 1);

    void scrollup (int times);
    void scrolldown (int times);
    void scrollleft (int times);
    void scrollright (int times);

    void search (const char *str, bool reverse = false);

    bool continuous ();

    returnType process (int times, guint keyval);

  private:
    void blank (int x, int y);

    void blankarea (int x1, int y1, int x2, int y2, guchar *, int width,
		    int height);

    void togglevisual (int type);

    int yank (int times);

    returnType subprocess (int ct, guint key);

    bool status_show ();

    int convertindex (int p);

    void markselection ();

    bool needsearch (const char *str, bool reverse = false);

    void gotolink (int ct);

    void returnlink (int ct);

    void refresh ();

    bool reload ();

    bool savelastposition ();

    bool loadlastposition ();

    static void apvlv_doc_on_mouse (GtkAdjustment *, ApvlvDoc *);

    static gboolean apvlv_doc_first_scroll_cb (gpointer);

    static gboolean apvlv_doc_first_copy_cb (gpointer);

    static void apvlv_doc_button_event (GtkEventBox * box,
					GdkEventButton * ev, ApvlvDoc *);

    static void apvlv_doc_copytoclipboard_cb (GtkMenuItem * item, ApvlvDoc *);

    static void begin_print (GtkPrintOperation * operation,
			     GtkPrintContext * context, PrintData * data);
    static void draw_page (GtkPrintOperation * operation,
			   GtkPrintContext * context,
			   gint page_nr, PrintData * data);
    static void end_print (GtkPrintOperation * operation,
			   GtkPrintContext * context, PrintData * data);

    enum
    { VISUAL_NONE, VISUAL_V, VISUAL_CTRL_V };
    gint mInVisual;
    gint mBlankx1, mBlanky1;
    gint mBlankx2, mBlanky2;
    gint mCurx, mCury;

    ApvlvDocPositionMap mPositions;
      vector < ApvlvDocPosition > mLinkPositions;

    ApvlvDocCache *mCurrentCache1, *mCurrentCache2;
    ApvlvDocCache *newcache (int pagenum);
    void deletecache (ApvlvDocCache * ac);

    bool mAutoScrollPage;
    bool mAutoScrollDoc;
    bool mContinuous;

    // image viewer
    GtkWidget *mImage1, *mImage2;
  };

}

#endif

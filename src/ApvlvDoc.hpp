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

#include "ApvlvCore.hpp"
#include "ApvlvUtil.hpp"

#include <gtk/gtk.h>
#include <glib/poppler.h>
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
    PopplerDocument *doc;
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
    ApvlvDocCache (ApvlvDoc *);

    ~ApvlvDocCache ();

    void set (guint p, bool delay = true);

    static void load (ApvlvDocCache *);

    PopplerPage *getpage ();

    guint getpagenum ();

    guchar *getdata (bool wait);

    GdkPixbuf *getbuf (bool wait);

    GList *getlinks ();

  private:
      ApvlvDoc * mDoc;
    PopplerPage *mPage;
    GList *mLinkMappings;
    gint mPagenum;
    guchar *mData;
    GdkPixbuf *mBuf;
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

    PopplerDocument *getdoc ();

    ApvlvDoc *copy ();

    void getpagesize (PopplerPage * p, double *x, double *y);

    bool getpagetext (PopplerPage * p, char **contents);

    gint pagesum ();

    bool usecache ();

    void usecache (bool use);

    bool loadfile (string & filename, bool check = true);

    bool loadfile (const char *src, bool check = true);

    bool print (int ct);

    bool totext (const char *name);

    bool rotate (int ct = 90);

    void markposition (const char s);

    void jump (const char s);

    void showpage (int p, double s = 0.00);

    void nextpage (int times = 1);

    void prepage (int times = 1);

    void halfnextpage (int times = 1);

    void halfprepage (int times = 1);

    void search (const char *str, bool reverse = false);

    returnType process (int times, guint keyval);

  private:
      returnType subprocess (int ct, guint key);

    bool status_show ();

    int convertindex (int p);

    void markselection ();

    bool needsearch (const char *str, bool reverse = false);

    GList *searchpage (int num, bool reverse = false);

    void gotolink (int ct);

    void returnlink (int ct);

    void refresh ();

    bool reload ();

    bool savelastposition ();

    bool loadlastposition ();

    static void apvlv_doc_on_mouse (GtkAdjustment *, ApvlvDoc *);

    static gboolean apvlv_doc_first_scroll_cb (gpointer);

    static gboolean apvlv_doc_first_copy_cb (gpointer);

    static void begin_print (GtkPrintOperation * operation,
			     GtkPrintContext * context, PrintData * data);
    static void draw_page (GtkPrintOperation * operation,
			   GtkPrintContext * context,
			   gint page_nr, PrintData * data);
    static void end_print (GtkPrintOperation * operation,
			   GtkPrintContext * context, PrintData * data);

    PopplerDocument *mDoc;

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

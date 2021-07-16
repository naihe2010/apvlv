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
/* @PPCPPFILE ApvlvDoc.h
 *
 *  Author: Alf <naihe2010@126.com>
 */
/* @date Created: 2008/09/30 00:00:00 Alf */

#ifndef _APVLV_DOC_H_
#define _APVLV_DOC_H_

#include "ApvlvFile.h"
#include "ApvlvCore.h"
#include "ApvlvUtil.h"

#include <gtk/gtk.h>
#include <webkit2/webkit2.h>

#include <iostream>
#include <vector>
#include <map>
#include <list>

using namespace std;

namespace apvlv
{
    struct PrintData {
        ApvlvFile *file;
        guint frmpn, endpn;
    };

    struct ApvlvDocPosition {
        int pagenum;
        double scrollrate;
    };

    typedef map<char, ApvlvDocPosition> ApvlvDocPositionMap;

    struct ApvlvWord {
        ApvlvPos pos;
    };

    struct ApvlvLine {
        ApvlvPos pos;
        vector<ApvlvWord> mWords;
    };

    typedef enum {
        DISPLAY_TYPE_IMAGE = 0,
        DISPLAY_TYPE_HTML = 1,
    } DISPLAY_TYPE;

    DISPLAY_TYPE get_display_type_by_filename (const char *filename);

    class ApvlvDoc;
    class ApvlvDocCache {
     public:
      explicit ApvlvDocCache (ApvlvFile *);

      ~ApvlvDocCache ();

      void set (guint p, double zm, guint rot, bool delay = true);

      static void load (ApvlvDocCache *);

      guint getpagenum () const;

      guchar *getdata (bool wait);

      GdkPixbuf *getbuf (bool wait);

      gint getwidth () const;

      gint getheight () const;

      ApvlvLinks *getlinks ();

      bool mInverted;

      ApvlvWord *getword (int x, int y);

      ApvlvLine *getline (double x, double y);

     private:
      ApvlvFile *mFile;
      ApvlvLinks *mLinks;
      double mZoom;
      guint mRotate;
      gint mPagenum;
      guchar *mData;
      gint mSize;
      GdkPixbuf *mBuf;
      gint mWidth;
      gint mHeight;

      vector<ApvlvLine> *mLines;

      void preparelines (gint x1, gint y1, gint x2, gint y2);
      ApvlvPos prepare_add (ApvlvPos &last, ApvlvPoses *results,
                            const char *word);
    };

    class ApvlvDocStatus : public ApvlvCoreStatus {
     public:
      explicit ApvlvDocStatus (ApvlvDoc *);

      ~ApvlvDocStatus () override;

      void active (bool act) override;

      void show (bool mContinuous) override;

     private:
      ApvlvDoc *mDoc;
#define AD_STATUS_SIZE   4
      GtkWidget *mStlab[AD_STATUS_SIZE];
    };

    class ApvlvDoc : public ApvlvCore {
     public:
      ApvlvDoc (ApvlvView *, DISPLAY_TYPE type, const char *zm = "NORMAL", bool cache = false);

      ~ApvlvDoc () override;

      void setactive (bool act) override;

      ApvlvDoc *copy () override;

      bool usecache () override;

      void usecache (bool use) override;

      bool loadfile (string &filename, bool check);

      bool loadfile (const char *src, bool check) override;

      int pagenumber () override;

      bool print (int ct) override;

      bool totext (const char *name) override;

      bool rotate (int ct) override;

      void markposition (char s) override;

      void setzoom (const char *z) override;

      void jump (char s) override;

      void showpage (int p, double s) override;

      void nextpage (int times) override;

      void prepage (int times) override;

      void halfnextpage (int times) override;

      void halfprepage (int times) override;

      void scrollup (int times) override;
      void scrolldown (int times) override;
      void scrollleft (int times) override;
      void scrollright (int times) override;

      void scrollupweb (int times);
      void scrolldownweb (int times);
      void scrollleftweb (int times);
      void scrollrightweb (int times);

      bool search (const char *str, bool reverse) override;

      returnType process (int hastimes, int times, guint keyval) override;

      void gotolink (int ct) override;

      void returnlink (int ct) override;

      void srtranslate (int &rtimes, double &sr, bool single2continuous);

      static void webview_resource_load_started_cb (WebKitWebView *web_view,
                                                    WebKitWebResource *resource,
                                                    WebKitURIRequest *request,
                                                    ApvlvDoc *doc);
      static void webview_load_changed_cb (WebKitWebView *web_view,
                                           WebKitLoadEvent event,
                                           ApvlvDoc *doc);
      static gboolean webview_context_menu_cb (WebKitWebView *web_view,
                                               WebKitContextMenu *context_menu,
                                               GdkEvent *event,
                                               WebKitHitTestResult *hit_test_result,
                                               ApvlvDoc *doc);

     private:
      void blank (int x, int y);

      static void blankarea (int x1, int y1, int x2, int y2, guchar *, int width,
                             int height);

      void blankaction (double x, double y);

      void togglevisual (int type);

      void scrollweb (int times, int w, int h);
      void scrollwebto (double xrate, double yrate);

      int yank (int times);

      void eventpos (double x, double y, double *rx, double *ry);

      returnType subprocess (int ct, guint key);

      int convertindex (int p);

      void markselection ();

      bool needsearch (const char *str, bool reverse = false);

      void refresh () override;

      bool reload () override;

      bool savelastposition (const char *filename);

      bool loadlastposition (const char *filename);

      static void apvlv_doc_on_mouse (GtkAdjustment *, ApvlvDoc *);

      static void apvlv_doc_button_event (GtkEventBox *box,
                                          GdkEventButton *button, ApvlvDoc *doc);

      static void apvlv_doc_motion_event (GtkWidget *, GdkEventMotion *,
                                          ApvlvDoc *);

      static void apvlv_doc_copytoclipboard_cb (GtkMenuItem *item, ApvlvDoc *);

      static void begin_print (GtkPrintOperation *operation,
                               GtkPrintContext *context, PrintData *data);
      static void draw_page (GtkPrintOperation *operation,
                             GtkPrintContext *context,
                             gint page_nr, PrintData *data);
      static void end_print (GtkPrintOperation *operation,
                             GtkPrintContext *context, PrintData *data);

      static void apvlv_doc_monitor_callback (GFileMonitor *, GFile *, GFile *, GFileMonitorEvent, ApvlvDoc *);

      enum {
          VISUAL_NONE, VISUAL_V, VISUAL_CTRL_V
      };
      gint mInVisual;
      gint mBlankx1, mBlanky1;
      gint mBlankx2, mBlanky2;
      gint mLastpress;
      gint mCurx, mCury;

      ApvlvDocPositionMap mPositions;
      vector<ApvlvDocPosition> mLinkPositions;

      ApvlvDocCache *mCurrentCache1, *mCurrentCache2, *mCurrentCache3;

      DISPLAY_TYPE mDisplayType;
      // image viewer
      GtkWidget *mImg1, *mImg2, *mImg3;
      GtkWidget *mWeb1;
    };

}

#endif

/* Local Variables: */
/* mode: c++ */
/* End: */

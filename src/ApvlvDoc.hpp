/*
* This file is part of the apvlv package
*
* Copyright (C) 2008 Alf.
*
* Contact: YuPengda <naihe2010@gmail.com>
*
* This library is free software; you can redistribute it and/or
* modify it under the terms of the GNU Lesser General Public License
* as published by the Free Software Foundation; either version 2.1 of
* the License, or (at your option) any later version.
*
* This library is distributed in the hope that it will be useful, but
* WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
* Lesser General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public
* License along with this library; if not, write to the Free Software
* Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
* 02110-1301 USA
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

#include "ApvlvUtil.hpp"


#ifdef HAVE_PTHREAD
#include <pthread.h>
#endif

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
#define HEADER_HEIGHT (10*72/25.4)
#define HEADER_GAP (3*72/25.4)

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

  class ApvlvDoc;
  class ApvlvDocCache
    {
  public:
    ApvlvDocCache (ApvlvDoc *);
    void set (guint p, bool delay = true);
    static void load (ApvlvDocCache *);
    static gboolean delayload (ApvlvDocCache *);
    ~ApvlvDocCache ();
    PopplerPage *getpage ();
    guint getpagenum ();
    guchar *getdata (bool wait);
    GdkPixbuf *getbuf (bool wait);

  private:
#ifdef HAVE_PTHREAD
    bool thread_running;
    gint timer;
    pthread_t tid;
    pthread_cond_t cond;
    pthread_mutex_t mutex;
#endif

    ApvlvDoc *doc;
    PopplerPage *page;
    int pagenum;
    guchar *data;
    GdkPixbuf *buf;
    };

  class ApvlvDocStatus
    {
  public:
    ApvlvDocStatus (ApvlvDoc *doc);
    ~ApvlvDocStatus ();
    GtkWidget *widget () { return vbox; }
    void active (bool act);
    void setsize (int w, int h);
    void show ();

  private:
#define AD_STATUS_SIZE   4
    ApvlvDoc  *doc;
    GtkWidget *vbox;
    GtkWidget *stlab[AD_STATUS_SIZE];
    };

  class ApvlvDoc
    {
  public:
    ApvlvDoc (const char *zm = "NORMAL");

    ~ApvlvDoc ();

    ApvlvDoc *copy ();

    const char *filename () { return doc? filestr.c_str (): NULL; }

    PopplerDocument *getdoc () { return doc; }

    void getpagesize (PopplerPage *p, double *x, double *y);

    bool getpagetext (PopplerPage *p, char **contents);

    int pagenumber () { return pagenum + 1; }

    int getrotate () { return rotatevalue; }

    int pagesum () { return doc? poppler_document_get_n_pages (doc): 0; }

    double scrollrate ();

    double zoomvalue () { return zoomrate; }

    GtkWidget *widget ();

    bool loadfile (string & filename, bool check = true) { return loadfile (filename.c_str (), check); }

    bool loadfile (const char *src, bool check = true);

    bool reload () { savelastposition (); return loadfile (filestr, false); }

    bool print (int ct);

    bool totext (const char *name);

    bool rotate (int ct = 90);

    void setsize (int wid, int hei);

    void sizesmaller (int s = 1);

    void sizebigger (int s = 1);

    void markposition (const char s);

    void jump (const char s);

    void showpage (int p, double s = 0.00);

    void nextpage (int times = 1) { showpage (pagenum + times); }
    void prepage (int times = 1) { showpage (pagenum - times); }
    void halfnextpage (int times = 1);
    void halfprepage (int times = 1);

    gboolean scrollto (double s);

    void scrollup (int times = 1);
    void scrolldown (int times = 1);
    void scrollleft (int times = 1);
    void scrollright (int times = 1);

    void setcache (const char *s) { return setcache (atoi (s)); }
    void setcache (int d);
    void setzoom (const char *s);
    void setzoom (double d);
    void zoomin () { zoomrate *= 1.1; refresh (); }
    void zoomout () { zoomrate /= 1.1; refresh (); }

    void search (const char *str);
    void backsearch (const char *str);

    void setactive (bool active);

    returnType process (int times, guint keyval, guint state);

  private:
    bool status_show ();

    int convertindex (int p);
    void markselection ();
    bool needsearch (const char *str);
    GList * searchpage (int num);
    void refresh ();
    bool savelastposition ();
    bool loadlastposition ();

    static gboolean apvlv_doc_first_scroll_cb (gpointer);

    static gboolean apvlv_doc_first_copy_cb (gpointer);

    static void begin_print (GtkPrintOperation *operation,
                             GtkPrintContext   *context,
                             PrintData         *data);
    static void draw_page (GtkPrintOperation *operation,
                           GtkPrintContext   *context,
                           gint               page_nr,
                           PrintData         *data);
    static void end_print (GtkPrintOperation *operation,
                           GtkPrintContext   *context,
                           PrintData         *data);
    string filestr;
    char *rawdata;
    size_t rawdatasize;
    PopplerDocument *doc;

    double scrollvalue;
    map <char, ApvlvDocPosition> positions;

    GList *results;
    string searchstr;

#ifdef HAVE_PTHREAD
    ApvlvDocCache *fetchcache (guint);
    ApvlvDocCache *mLastCache, *mNextCache;
#endif
    ApvlvDocCache *mCurrentCache;
    ApvlvDocCache *newcache (int pagenum);
    void deletecache (ApvlvDocCache *ac);

    int rotatevalue;
    int pagenum;
    double pagex, pagey;
    double vrate, hrate;

    enum
      {
        NORMAL,
        FITWIDTH,
        FITHEIGHT,
        CUSTOM
      } zoommode;
    double zoomrate;
    bool zoominit;
    int lines, chars;
    int width, height;
    GtkAdjustment *vaj, *haj;

    GtkWidget *vbox; // the main widget
    GtkWidget *scrollwin; // the document scrolled window
    GtkWidget *image; // the image

    ApvlvDocStatus *status;

    bool mActive;
    };

}

#endif

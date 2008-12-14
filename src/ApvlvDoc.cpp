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
/* @CPPFILE ApvlvDoc.cpp
*
*  Author: Alf <naihe2010@gmail.com>
*/
/* @date Created: 2008/09/30 00:00:00 Alf */

#include "ApvlvUtil.hpp"
#include "ApvlvView.hpp"
#include "ApvlvDoc.hpp"

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <sys/stat.h>

#ifdef HAVE_PTHREAD
#include <pthread.h>
#endif

#include <gtk/gtk.h>
#include <gtk/gtkprintoperation.h>
#include <glib/poppler.h>

#include <iostream>
#include <fstream>
#include <sstream>

#ifdef WIN32
#define snprintf _snprintf
#endif

namespace apvlv
{
#ifdef HAVE_PTHREAD
  static pthread_mutex_t rendermutex = PTHREAD_MUTEX_INITIALIZER;
#endif
  static GtkPrintSettings *settings = NULL;

  ApvlvDoc::ApvlvDoc (const char *zm)
    {
      mCurrentCache = NULL;
#ifdef HAVE_PTHREAD
      mLastCache = NULL;
      mNextCache = NULL;
#endif

      rotatevalue = 0;

      rawdata = NULL;
      doc = NULL;

      results = NULL;
      searchstr = "";

      vbox = gtk_vbox_new (FALSE, 0);

      scrollwin = gtk_scrolled_window_new (NULL, NULL);

      image = gtk_image_new ();
      gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (scrollwin),
                                             image);
      gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrollwin),
                                      GTK_POLICY_AUTOMATIC,
                                      GTK_POLICY_AUTOMATIC);

      vaj = gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW (scrollwin));
      haj = gtk_scrolled_window_get_hadjustment (GTK_SCROLLED_WINDOW (scrollwin));

      status = new ApvlvDocStatus (this);

      gtk_box_pack_start (GTK_BOX (vbox), scrollwin, FALSE, FALSE, 0);
      gtk_box_pack_end (GTK_BOX (vbox), status->widget (), FALSE, FALSE, 0);

      setzoom (zm);
    }

  ApvlvDoc::~ApvlvDoc ()
    {
    if (mCurrentCache)
      delete mCurrentCache;
#ifdef HAVE_PTHREAD
    if (mNextCache)
      delete mNextCache;
    if (mLastCache)
      delete mLastCache;
#endif

      if (filestr != helppdf)
        {
          savelastposition ();
        }
      positions.clear ();

      if (rawdata != NULL)
        delete []rawdata;

      delete status;

      gtk_widget_destroy (vbox);
    }

  void
    ApvlvDoc::setsize (int w, int h)
      {
        gtk_widget_set_usize (widget (), w, h);
        gtk_widget_set_usize (scrollwin, w, h - 20);
        status->setsize (w, 20);
        width = w;
        height = h;
      }

  ApvlvDoc *
    ApvlvDoc::copy ()
      {
        char rate[16];
        snprintf (rate, sizeof rate, "%f", zoomrate);
        ApvlvDoc *ndoc = new ApvlvDoc (rate);
        ndoc->loadfile (filestr, false);
        ndoc->showpage (pagenum, scrollrate ());
        return ndoc;
      }

  bool
    ApvlvDoc::savelastposition ()
      {
        gchar *path = absolutepath (sessionfile.c_str ());
        ofstream os (path, ios::app);
        g_free (path);

        if (os.is_open ())
          {
            os << ">";
            os << filename () << "\t";
            os << pagenum << "\t";
            os << scrollrate ();
            os << "\n";
            os.close ();
            return true;
          }
        return false;
      }

  bool
    ApvlvDoc::loadlastposition ()
      {
        bool ret = false;
        int pagen = 0;
        double scrollr = 0.00;

        char *path = absolutepath (sessionfile.c_str ());
        ifstream os (path, ios::in);
        g_free (path);

        if (os.is_open ())
          {
            string line;

            while ((getline (os, line)) != NULL)
              {
                const char *p = line.c_str ();

                if (*p == '>')
                  {
                    stringstream ss (++ p);

                    string files;
                    ss >> files;

                    if (files == filestr)
                      {
                        ss >> pagen >> scrollr;
                        ret = true;
                      }
                  }
              }
            os.close ();
          }

        scrollvalue = scrollr;
        showpage (pagen);

        // Warning
        // I can't think a better way to scroll correctly when
        // the page is not be displayed correctly
        g_timeout_add (50, apvlv_doc_first_scroll_cb, this);

        return ret;
      }

  bool
    ApvlvDoc::loadfile (const char *filename, bool check)
      {
        if (check)
          {
            if (strcmp (filename, filestr.c_str ()) == 0)
              {
                return false;
              }
          }

#ifdef WIN32
        gchar *wfilename = g_win32_locale_filename_from_utf8 (filename);
#else
        gchar *wfilename = (gchar *) filename;
#endif
        size_t filelen;
        struct stat sbuf;
        int rt = stat (wfilename, &sbuf);
        if (rt < 0)
          {
            err ("Can't stat the PDF file: %s.", wfilename);
            return false;
          }
        filelen = sbuf.st_size;

        if (rawdata != NULL
            && rawdatasize < filelen)
          {
            delete []rawdata;
            rawdata = NULL;
          }

        if (rawdata == NULL)
          {
            rawdata = new char[filelen];
            rawdatasize = filelen;
          }

        ifstream ifs (wfilename, ios::binary);
        if (ifs.is_open ())
        {
          ifs.read (rawdata, filelen);
          ifs.close ();
        }

#ifdef WIN32
        g_free (wfilename);
#endif

        doc = poppler_document_new_from_data (rawdata, filelen, NULL, NULL);

        if (doc != NULL)
          {
            zoominit = false;
            lines = 50;
            chars = 80;
            filestr = filename;

#ifdef HAVE_PTHREAD
            if (mLastCache != NULL)
              delete mLastCache;
            if (mNextCache != NULL)
              delete mNextCache;
            mLastCache = new ApvlvDocCache (this);
            mNextCache = new ApvlvDocCache (this);
#endif
            if (mCurrentCache != NULL)
              delete mCurrentCache;
            mCurrentCache = new ApvlvDocCache (this);

            loadlastposition ();

            status->show ();

            setactive (true);
          }

        return doc == NULL? false: true;
      }

  void
    ApvlvDoc::setzoom (const char *z)
      {
        if (strcmp (z, "normal") == 0)
          {
            zoommode = NORMAL;
          }
        if (strcmp (z, "fitwidth") == 0)
          {
            zoommode = FITWIDTH;
          }
        if (strcmp (z, "fitheight") == 0)
          {
            zoommode = FITHEIGHT;
          }
        else
          {
            double d = atof (z);
            if (d > 0)
              {
                zoommode = CUSTOM;
                zoomrate = d;
              }
          }

        refresh ();
      }

  void
    ApvlvDoc::setzoom (double d)
      {
        zoommode = CUSTOM;
        zoomrate = d;
        refresh ();
      }

  GtkWidget *
    ApvlvDoc::widget ()
      {
        return vbox;
      }

  int
    ApvlvDoc::convertindex (int p)
      {
        if (doc != NULL)
          {
            int c = poppler_document_get_n_pages (doc);
            if (p >= 0)
              {
                return p % c;
              }
            else if (p < 0)
              {
                return c + p;
              }
          }
        return -1;
      }

  void
    ApvlvDoc::markposition (const char s)
      {
        ApvlvDocPosition adp = { pagenum, scrollrate () };
        positions[s] = adp;
      }

  void
    ApvlvDoc::jump (const char s)
      {
        map <char, ApvlvDocPosition>::iterator it;
        for (it = positions.begin (); it != positions.end (); ++ it)
          {
            if ((*it).first == s)
              {
                ApvlvDocPosition adp = (*it).second;
                markposition ('\'');
                showpage (adp.pagenum, adp.scrollrate);
                break;
              }
          }
      }

  void
    ApvlvDoc::showpage (int p, double s)
      {
        int rp = convertindex (p);
        if (rp < 0)
          return;

#ifdef HAVE_PTHREAD
        ApvlvDocCache *ac = fetchcache (rp);
        if (ac != NULL)
          {
            GdkPixbuf *buf = ac->getbuf (true);
            gtk_image_set_from_pixbuf (GTK_IMAGE (image), buf);
            scrollto (s);

            pagenum = rp;

            mCurrentCache = ac;
            return;
          }
#endif
        PopplerPage *page = poppler_document_get_page (doc, rp);
        if (page != NULL)
          {
            getpagesize (page, &pagex, &pagey);

            if (zoominit == false)
              {
                switch (zoommode)
                  {
                  case NORMAL:
                    zoomrate = 1.2;
                    break;
                  case FITWIDTH:
                    zoomrate = ((double) (width - 26)) / pagex;
                    break;
                  case FITHEIGHT:
                    zoomrate = ((double) (height - 26)) / pagey;
                    break;
                  case CUSTOM:
                    break;
                  default:
                    break;
                  }

                zoominit = true;
              }

            pagenum = poppler_page_get_index (page);

            refresh ();

            scrollto (s);
          }
      }

  void
    ApvlvDoc::refresh ()
      {
        if (doc == NULL)
          return;

        mCurrentCache->set (pagenum, false);
        gtk_image_set_from_pixbuf (GTK_IMAGE (image), mCurrentCache->getbuf (true));

        status->show ();

#ifdef HAVE_PTHREAD
        if (mNextCache != NULL)
          {
            mNextCache->set (mNextCache->getpagenum ());
          }
        if (mLastCache != NULL)
          {
            mLastCache->set (mLastCache->getpagenum ());
          }
#endif
      }

#ifdef HAVE_PTHREAD
  ApvlvDocCache *
    ApvlvDoc::fetchcache (guint pn)
      {
        ApvlvDocCache *ac = NULL;

        if (mNextCache->getpagenum () == pn)
          {
            ac = mNextCache;
            mNextCache = mLastCache;
            mLastCache = mCurrentCache;
            mNextCache->set (convertindex (pn + 1));
          }

        else if (mLastCache->getpagenum () == pn)
          {
            ac = mLastCache;
            if ((int) mCurrentCache->getpagenum () == convertindex (pn + 1))
              {
                mLastCache = mNextCache;
                mLastCache->set (convertindex (pn - 1));
                mNextCache = mCurrentCache;
              }
            else
              {
                mLastCache = mCurrentCache;
                mLastCache->set (convertindex (pn - 1));
                mNextCache->set (convertindex (pn + 1));
              }
          }

        else
          {
            mNextCache->set (convertindex (pn + 1));
          }

        return ac;
      }
#endif

  void
    ApvlvDoc::halfnextpage (int times)
      {
        double sr = scrollrate ();
        int rtimes = times / 2;

        showpage (pagenum + rtimes, sr);
        if (times % 2 != 0)
          {
            scrolldown (lines / 2);
          }
      }

  void
    ApvlvDoc::halfprepage (int times)
      {
        double sr = scrollrate ();
        int rtimes = times / 2;

        showpage (pagenum - rtimes, sr);
        if (times % 2 != 0)
          {
            scrollup (lines / 2);
          }
      }

  double
    ApvlvDoc::scrollrate ()
      {
        double maxv = vaj->upper - vaj->lower - vaj->page_size;
        double val =  vaj->value / maxv;
        if (val > 1.0)
          {
            return 1.00;
          }
        else if (val > 0.0)
          {
            return val;
          }
        else
          {
            return 0.00;
          }
      }

  gboolean
    ApvlvDoc::scrollto (double s)
      {
        if (vaj->upper != vaj->lower)
          {
            double maxv = vaj->upper - vaj->lower - vaj->page_size;
            double val = maxv * s;
            gtk_adjustment_set_value (vaj, val);
            status->show ();
            return TRUE;
          }
        else
          {
            debug ("fatal a timer error, try again!");
            return FALSE;
          }
      }

  void
    ApvlvDoc::scrollup (int times)
      {
        if (doc == NULL)
          return;

        gdouble val = gtk_adjustment_get_value (vaj);
        vrate = (vaj->upper - vaj->lower) / lines;
        if (val - vrate * times > vaj->lower)
          {
            gtk_adjustment_set_value (vaj, val - vrate * times);
          }
        else if (val > vaj->lower)
          {
            gtk_adjustment_set_value (vaj, vaj->lower);
          }
        else if (pagenum > 0)
          {
            showpage (pagenum - 1, 1.00);
          }

        status->show ();
      }

  void
    ApvlvDoc::scrolldown (int times)
      {
        if (doc == NULL)
          return;

        gdouble val = gtk_adjustment_get_value (vaj);
        vrate = (vaj->upper - vaj->lower) / lines;
        if (val + vrate * times + vaj->page_size < vaj->upper)
          {
            gtk_adjustment_set_value (vaj, val + vrate * times);
          }
        else if (val + vaj->page_size < vaj->upper)
          {
            gtk_adjustment_set_value (vaj, vaj->upper - vaj->page_size);
          }
        else if (pagenum < poppler_document_get_n_pages (doc) - 1)
          {
            showpage (pagenum + 1, 0.00);
          }

        status->show ();
      }

  void
    ApvlvDoc::scrollleft (int times)
      {
        if (doc == NULL)
          return;

        hrate = (haj->upper - haj->lower) / chars;
        gdouble val = haj->value - hrate * times;
        if (val > vaj->lower)
          {
            gtk_adjustment_set_value (haj, val);
          }
        else
          {
            gtk_adjustment_set_value (haj, haj->lower);
          }
      }

  void
    ApvlvDoc::scrollright (int times)
      {
        if (doc == NULL)
          return;

        hrate = (haj->upper - haj->lower) / chars;
        gdouble val = haj->value + hrate * times;
        if (val + haj->page_size < haj->upper)
          {
            gtk_adjustment_set_value (haj, val);
          }
        else
          {
            gtk_adjustment_set_value (haj, haj->upper - haj->page_size);
          }
      }

  void
    ApvlvDoc::markselection ()
      {
        PopplerRectangle *rect = (PopplerRectangle *) results->data;

        gchar *txt = poppler_page_get_text (mCurrentCache->getpage (), POPPLER_SELECTION_GLYPH, rect);
        if (txt == NULL)
          {
            debug ("no search result");
            return;
          }

        // Caculate the correct position
        gint x1 = (gint) ((rect->x1) * zoomrate);
        gint y1 = (gint) ((pagey - rect->y2) * zoomrate);
        gint x2 = (gint) ((rect->x2) * zoomrate);
        gint y2 = (gint) ((pagey - rect->y1) * zoomrate);

        // make the selection at the page center
        gdouble val = ((y1 + y2) - vaj->page_size) / 2;
        if (val + vaj->page_size > vaj->upper)
          {
            gtk_adjustment_set_value (vaj, vaj->upper);
          }
        else if (val > 0)
          {
            gtk_adjustment_set_value (vaj, val);
          }
        else
          {
            gtk_adjustment_set_value (vaj, vaj->lower);
          }
        val = ((x1 + x2) - haj->page_size) / 2;
        if (val + haj->page_size > haj->upper)
          {
            gtk_adjustment_set_value (haj, haj->upper);
          }
        else if (val > 0)
          {
            gtk_adjustment_set_value (haj, val);
          }
        else
          {
            gtk_adjustment_set_value (haj, haj->lower);
          }

        guchar *pagedata = mCurrentCache->getdata (true);
        GdkPixbuf *pixbuf = mCurrentCache->getbuf (true);
        // change the back color of the selection
        for (gint y = y1; y < y2; y ++)
          {
            for (gint x = x1; x < x2; x ++)
              {
                gint p = (gint) (y * 3 * pagex * zoomrate + (x * 3));
                pagedata[p + 0] = 0xFF - pagedata[p + 0];
                pagedata[p + 1] = 0xFF - pagedata[p + 0];
                pagedata[p + 2] = 0xFF - pagedata[p + 0];
              }
          }

        gtk_image_set_from_pixbuf (GTK_IMAGE (image), pixbuf);

        g_free (rect);
        results = g_list_remove (results, rect);
      }

  GList *
    ApvlvDoc::searchpage (int num)
      {
        if (doc == NULL)
          return NULL;

        PopplerPage *page = poppler_document_get_page (doc, num);

        GList *ret = poppler_page_find_text (page, searchstr.c_str ());

        return ret;
      }

  bool
    ApvlvDoc::needsearch (const char *str)
      {
        if (doc == NULL)
          return false;

        if (strlen (str) > 0)
          {
            g_list_free (results);
            results = NULL;

            searchstr = str;

            return true;
          }
        else
          {
            if (results != NULL)
              {
                markselection ();
                return false;
              }
            else
              {
                return true;
              }
          }
      }

  void
    ApvlvDoc::search (const char *str)
      {
        if (needsearch (str))
          {
            int num = poppler_document_get_n_pages (doc);
            int i = strlen (str) > 0? pagenum - 1: pagenum;
            while (i ++ < num - 1)
              {
                results = searchpage (i);
                if (results != NULL)
                  {
                    showpage (i);
                    markselection ();
                    break;
                  }
                i ++;
              }
          }
      }

  void
    ApvlvDoc::backsearch (const char *str)
      {
        if (needsearch (str))
          {
            poppler_document_get_n_pages (doc);
            int i = strlen (str) > 0? pagenum + 1: pagenum;
            while (i -- > 0)
              {
                results = g_list_reverse (searchpage (i));
                if (results != NULL)
                  {
                    showpage (i);
                    markselection ();
                    break;
                  }
              }
          }
      }

  void
    ApvlvDoc::getpagesize (PopplerPage *p, double *x, double *y)
      {
        if (rotatevalue == 90
            || rotatevalue == 270)
          {
            poppler_page_get_size (p, y, x);
          }
        else
          {
            poppler_page_get_size (p, x, y);
          }
      }

  bool
    ApvlvDoc::getpagetext (PopplerPage *page, char **text)
      {
        double x, y;
        getpagesize (page, &x, &y);
        PopplerRectangle rect = { 0, 0, x, y };
        *text = poppler_page_get_text (page, POPPLER_SELECTION_WORD, &rect);
        if (*text != NULL)
          {
            return true;
          }
        return false;
      }

  bool
    ApvlvDoc::totext (const char *file)
      {
        if (doc == NULL)
          return false;

        PopplerPage *page = mCurrentCache->getpage ();
        char *txt;
        bool ret = getpagetext (page, &txt);
        if (ret == true)
          {
            g_file_set_contents (file, txt, -1, NULL);
            return true;
          }
        return false;
      }

  void
    ApvlvDoc::setactive (bool act)
      {
        status->active (act);
        mActive = act;
      }

  bool
    ApvlvDoc::rotate (int ct)
      {
        if (ct == 1) ct = 90;

        if (ct % 90 != 0)
          {
            warn ("Not a 90 times value, ignore.");
            return false;
          }

        rotatevalue += ct;
        while (rotatevalue < 0)
          {
            rotatevalue += 360;
          }
        refresh ();
        return true;
      }

  bool
    ApvlvDoc::print (int ct)
      {
#ifdef WIN32
        return false;
#else
        bool ret = false;
        GtkPrintOperation *print = gtk_print_operation_new ();

        gtk_print_operation_set_allow_async (print, TRUE);
        gtk_print_operation_set_show_progress (print, TRUE);

        PrintData *data = new PrintData;
        data->doc = doc;
        data->frmpn = pagenum;
        data->endpn = pagenum;

        g_signal_connect (G_OBJECT (print), "begin-print", G_CALLBACK (begin_print), data);
        g_signal_connect (G_OBJECT (print), "draw-page", G_CALLBACK (draw_page), data);
        g_signal_connect (G_OBJECT (print), "end-print", G_CALLBACK (end_print), data);
        if (settings != NULL)
          {
            gtk_print_operation_set_print_settings (print, settings);
          }
        int r = gtk_print_operation_run (print, GTK_PRINT_OPERATION_ACTION_PRINT_DIALOG, GTK_WINDOW (gView->widget ()), NULL);
        if (r == GTK_PRINT_OPERATION_RESULT_APPLY)
          {
            if (settings != NULL)
              {
                g_object_unref (settings);
              }
            settings = gtk_print_operation_get_print_settings (print);
            ret = true;
          }
        g_object_unref (print);
        return ret;
#endif
      }

  gboolean
    ApvlvDoc::apvlv_doc_first_scroll_cb (gpointer data)
      {
        ApvlvDoc *doc = (ApvlvDoc *) data;
        return doc->scrollto (doc->scrollvalue) == TRUE? FALSE: TRUE;
      }

  gboolean
    ApvlvDoc::apvlv_doc_first_copy_cb (gpointer data)
      {
        ApvlvDoc *doc = (ApvlvDoc *) data;
        doc->loadfile (doc->filestr, false);
        return FALSE;
      }

#ifndef WIN32
  void
    ApvlvDoc::begin_print (GtkPrintOperation *operation,
                           GtkPrintContext   *context,
                           PrintData         *data)
      {
        gtk_print_operation_set_n_pages (operation, data->endpn - data->frmpn + 1);
      }

  void
    ApvlvDoc::draw_page (GtkPrintOperation *operation,
                         GtkPrintContext   *context,
                         gint               page_nr,
                         PrintData         *data)
      {
        cairo_t *cr = gtk_print_context_get_cairo_context (context);
        PopplerPage *page = poppler_document_get_page (data->doc, data->frmpn + page_nr);
        poppler_page_render_for_printing (page, cr);

        PangoLayout *layout = gtk_print_context_create_pango_layout (context);
        pango_cairo_show_layout (cr, layout);
        g_object_unref (layout);
      }

  void
    ApvlvDoc::end_print (GtkPrintOperation *operation,
                         GtkPrintContext   *context,
                         PrintData         *data)
      {
        delete data;
      }
#endif

  ApvlvDocCache::ApvlvDocCache (ApvlvDoc *dc)
    {
      doc = dc;
      pagenum = -1;
      data = NULL;
      buf = NULL;
#ifdef HAVE_PTHREAD
      timer = -1;
      thread_running = false;
      pthread_cond_init (&cond, NULL);
      pthread_mutex_init (&mutex, NULL);
#endif
    }

  void
    ApvlvDocCache::set (guint p, bool delay)
      {
#ifdef HAVE_PTHREAD
        if (thread_running)
          {
            pthread_cancel (tid);
          }
#endif
        pagenum = p;
        if (data != NULL)
          {
            delete []data;
            data = NULL;
          }
        if (buf != NULL)
          {
            g_object_unref (buf);
            buf = NULL;
          }
#ifdef HAVE_PTHREAD
        pthread_cond_init (&cond, NULL);

        if (delay == true)
          {
            timer = g_timeout_add (50, (gboolean (*) (void *)) delayload, this);
          }
        else
          {
            pthread_create (&tid, NULL, (void *(*) (void *)) load, this);
          }
#else
        load (this);
#endif
      }

#ifdef HAVE_PTHREAD
  gboolean
    ApvlvDocCache::delayload (ApvlvDocCache *ac)
      {
        pthread_create (&ac->tid, NULL, (void *(*) (void *)) load, ac);
        ac->timer = -1;
        return FALSE;
      }
#endif

  void
    ApvlvDocCache::load (ApvlvDocCache *ac)
      {
        int c = poppler_document_get_n_pages (ac->doc->getdoc ());
        if (ac->pagenum < 0
            || ac->pagenum >= c)
          {
            debug ("no this page: %d", ac->pagenum);
            return;
          }

#ifdef HAVE_PTHREAD
        ac->thread_running = true;
#endif

        PopplerPage *tpage = poppler_document_get_page (ac->doc->getdoc (), ac->pagenum);

        double tpagex, tpagey;
        ac->doc->getpagesize (tpage, &tpagex, &tpagey);

        int ix = (int) (tpagex * ac->doc->zoomvalue ()), iy = (int) (tpagey * ac->doc->zoomvalue ());

        guchar *dat = new guchar[ix * iy * 3];

        GdkPixbuf *bu = gdk_pixbuf_new_from_data (dat, GDK_COLORSPACE_RGB,
                                    FALSE,
                                    8,
                                    ix, iy,
                                    3 * ix,
                                    NULL, NULL);

#ifdef HAVE_PTHREAD
        pthread_mutex_lock (&rendermutex);
#endif
        poppler_page_render_to_pixbuf (tpage, 0, 0, ix, iy, ac->doc->zoomvalue (), ac->doc->getrotate (), bu);
#ifdef HAVE_PTHREAD
        pthread_mutex_unlock (&rendermutex);
#endif

        ac->page = tpage;
        ac->data = dat;
        ac->buf = bu;

#ifdef HAVE_PTHREAD
        pthread_cond_signal (&ac->cond);

        ac->thread_running = false;
#endif
      }

  ApvlvDocCache::~ApvlvDocCache ()
    {
#ifdef HAVE_PTHREAD
      if (timer > 0)
        {
          g_source_remove (timer);
        }
      if (thread_running)
        {
          pthread_cancel (tid);
        }
#endif
      if (data != NULL)
        delete []data;
      if (buf != NULL)
        g_object_unref (buf);
    }

  PopplerPage *
    ApvlvDocCache::getpage ()
      {
        return page;
      }

  guint
    ApvlvDocCache::getpagenum ()
      {
        return pagenum;
      }

  /*
   * get the cache data
   * @param: wait, if not wait, not wait the buffer be prepared
   * @return: the buffer
   * */
  guchar *ApvlvDocCache::getdata (bool wait)
    {
#ifndef HAVE_PTHREAD
      return data;
#else
      if (!wait)
        {
          return data;
        }

      guchar *dat = data;
      if (dat == NULL)
        {
          pthread_mutex_lock (&mutex);
          pthread_cond_wait (&cond, &mutex);
          dat = data;
          pthread_mutex_unlock (&mutex);
        }
      return dat;
#endif
    }

  /*
   * get the cache GdkPixbuf
   * @param: wait, if not wait, not wait the pixbuf be prepared
   * @return: the buffer
   * */
  GdkPixbuf *ApvlvDocCache::getbuf (bool wait)
    {
#ifndef HAVE_PTHREAD
      return buf;
#else
      if (!wait)
        {
          return buf;
        }

      GdkPixbuf *bu = buf;
      if (bu == NULL)
        {
          pthread_mutex_lock (&mutex);
          pthread_cond_wait (&cond, &mutex);
          bu = buf;
          pthread_mutex_unlock (&mutex);
        }
      return bu;
#endif
    }

  ApvlvDocStatus::ApvlvDocStatus (ApvlvDoc *dc)
    {
      doc = dc;
      vbox = gtk_hbox_new (FALSE, 0);
      for (int i=0; i<AD_STATUS_SIZE; ++i)
        {
          stlab[i] = gtk_label_new ("");
          gtk_box_pack_start (GTK_BOX (vbox), stlab[i], FALSE, FALSE, 0);
        }
    }

  ApvlvDocStatus::~ApvlvDocStatus ()
    {
    }

  void
    ApvlvDocStatus::active (bool act)
      {
        GdkColor c;

        if (act)
          {
            c.red = 300;
            c.green = 300;
            c.blue = 300;
          }
        else
          {
            c.red = 30000;
            c.green = 30000;
            c.blue = 30000;
          }

        for (unsigned int i=0; i<AD_STATUS_SIZE; ++i)
          {
            gtk_widget_modify_fg (stlab[i], GTK_STATE_NORMAL, &c);
          }
      }

  void
    ApvlvDocStatus::setsize (int w, int h)
      {
        int sw[AD_STATUS_SIZE];
        sw[0] = w >> 1;
        sw[1] = sw[0] >> 1;
        sw[2] = sw[1] >> 1;
        sw[3] = sw[1] >> 1;
        for (unsigned int i=0; i<AD_STATUS_SIZE; ++i)
          {
            gtk_widget_set_usize (stlab[i], sw[i], h);
          }
      }

  void
    ApvlvDocStatus::show ()
      {
        if (doc->filename ())
          {
            char temp[AD_STATUS_SIZE][256];
            gchar *bn;
            bn = g_path_get_basename (doc->filename ());
            snprintf (temp[0], sizeof temp[0], "%s", bn);
            snprintf (temp[1], sizeof temp[1], "%d/%d", doc->pagenumber (), doc->pagesum ());
            snprintf (temp[2], sizeof temp[2], "%d%%", (int) (doc->zoomvalue () * 100));
            snprintf (temp[3], sizeof temp[3], "%d%%", (int) (doc->scrollrate () * 100));
            for (unsigned int i=0; i<AD_STATUS_SIZE; ++i)
              {
                gtk_label_set_text (GTK_LABEL (stlab[i]), temp[i]);
              }
            g_free (bn);
          }
      }
}

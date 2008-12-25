/*
* This file is part of the apvlv package
*
* Copyright (C) 2008 Alf.
*
* Contact: Alf <naihe2010@gmail.com>
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

      mRotatevalue = 0;

      mRawdata = NULL;
      mDoc = NULL;

      mResults = NULL;
      mSearchstr = "";

      mVbox = gtk_vbox_new (FALSE, 0);

      mScrollwin = gtk_scrolled_window_new (NULL, NULL);

      mImage = gtk_image_new ();
      gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (mScrollwin),
                                             mImage);
      gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (mScrollwin),
                                      GTK_POLICY_AUTOMATIC,
                                      GTK_POLICY_AUTOMATIC);

      mVaj = gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW (mScrollwin));
      mHaj = gtk_scrolled_window_get_hadjustment (GTK_SCROLLED_WINDOW (mScrollwin));

      mStatus = new ApvlvDocStatus (this);

      gtk_box_pack_start (GTK_BOX (mVbox), mScrollwin, FALSE, FALSE, 0);
      gtk_box_pack_end (GTK_BOX (mVbox), mStatus->widget (), FALSE, FALSE, 0);

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

      if (mFilestr != helppdf)
        {
          savelastposition ();
        }
      mPositions.clear ();

      if (mRawdata != NULL)
        delete []mRawdata;

      delete mStatus;
    }

  void
    ApvlvDoc::setsize (int w, int h)
      {
        gtk_widget_set_usize (widget (), w, h);
        gtk_widget_set_usize (mScrollwin, w, h - 20);
        mStatus->setsize (w, 20);
        mWidth = w;
        mHeight = h;
      }

  ApvlvDoc *
    ApvlvDoc::copy ()
      {
        char rate[16];
        snprintf (rate, sizeof rate, "%f", mZoomrate);
        ApvlvDoc *ndoc = new ApvlvDoc (rate);
        ndoc->loadfile (mFilestr, false);
        ndoc->showpage (mPagenum, scrollrate ());
        return ndoc;
      }

  const char *
    ApvlvDoc::filename () 
      { 
        return mDoc? mFilestr.c_str (): NULL; 
      }

  PopplerDocument *
    ApvlvDoc::getdoc () 
      { 
        return mDoc; 
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
            os << mPagenum << "\t";
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

                    if (files == mFilestr)
                      {
                        ss >> pagen >> scrollr;
                        ret = true;
                      }
                  }
              }
            os.close ();
          }

        mScrollvalue = scrollr;
        showpage (pagen);

        // Warning
        // I can't think a better way to scroll correctly when
        // the page is not be displayed correctly
        g_timeout_add (50, apvlv_doc_first_scroll_cb, this);

        return ret;
      }

  void 
    ApvlvDoc::zoomin () 
      { 
        mZoomrate *= 1.1; 
        refresh (); 
      }

  void 
    ApvlvDoc::zoomout () 
      { 
        mZoomrate /= 1.1; 
        refresh (); 
      }

  bool 
    ApvlvDoc::reload () 
      { 
        savelastposition (); 
        return loadfile (mFilestr, false); 
      }

  bool 
    ApvlvDoc::loadfile (string & filename, bool check) 
      { 
        return loadfile (filename.c_str (), check); 
      }

  bool
    ApvlvDoc::loadfile (const char *filename, bool check)
      {
        if (check)
          {
            if (strcmp (filename, mFilestr.c_str ()) == 0)
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

        if (mRawdata != NULL
            && mRawdatasize < filelen)
          {
            delete []mRawdata;
            mRawdata = NULL;
          }

        if (mRawdata == NULL)
          {
            mRawdata = new char[filelen];
            mRawdatasize = filelen;
          }

        ifstream ifs (wfilename, ios::binary);
        if (ifs.is_open ())
        {
          ifs.read (mRawdata, filelen);
          ifs.close ();
        }

#ifdef WIN32
        g_free (wfilename);
#endif

        mDoc = poppler_document_new_from_data (mRawdata, filelen, NULL, NULL);

        if (mDoc != NULL)
          {
            mZoominit = false;
            mLines = 50;
            mChars = 80;
            mFilestr = filename;

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

            mStatus->show ();

            setactive (true);
          }

        return mDoc == NULL? false: true;
      }

  void
    ApvlvDoc::setzoom (const char *z)
      {
        if (strcmp (z, "normal") == 0)
          {
            mZoommode = NORMAL;
          }
        if (strcmp (z, "fitwidth") == 0)
          {
            mZoommode = FITWIDTH;
          }
        if (strcmp (z, "fitheight") == 0)
          {
            mZoommode = FITHEIGHT;
          }
        else
          {
            double d = atof (z);
            if (d > 0)
              {
                mZoommode = CUSTOM;
                mZoomrate = d;
              }
          }

        refresh ();
      }

  void
    ApvlvDoc::setzoom (double d)
      {
        mZoommode = CUSTOM;
        mZoomrate = d;
        refresh ();
      }

  int 
    ApvlvDoc::pagenumber ()
      { 
        return mPagenum + 1; 
      }

  int 
    ApvlvDoc::getrotate () 
      { 
        return mRotatevalue; 
      }

  int 
    ApvlvDoc::pagesum () 
      { 
        return mDoc? poppler_document_get_n_pages (mDoc): 0; 
      }

  double 
    ApvlvDoc::zoomvalue () 
      { 
        return mZoomrate; 
      }

  GtkWidget *
    ApvlvDoc::widget ()
      {
        return mVbox;
      }

  int
    ApvlvDoc::convertindex (int p)
      {
        if (mDoc != NULL)
          {
            int c = poppler_document_get_n_pages (mDoc);
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
        ApvlvDocPosition adp = { mPagenum, scrollrate () };
        mPositions[s] = adp;
      }

  void
    ApvlvDoc::jump (const char s)
      {
        ApvlvDocPositionMap::iterator it;
        for (it = mPositions.begin (); it != mPositions.end (); ++ it)
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
            gtk_image_set_from_pixbuf (GTK_IMAGE (mImage), buf);
            scrollto (s);

            mPagenum = rp;

            mCurrentCache = ac;
            return;
          }
#endif
        PopplerPage *mPage = poppler_document_get_page (mDoc, rp);
        if (mPage != NULL)
          {
            getpagesize (mPage, &mPagex, &mPagey);

            if (mZoominit == false)
              {
                switch (mZoommode)
                  {
                  case NORMAL:
                    mZoomrate = 1.2;
                    break;
                  case FITWIDTH:
                    mZoomrate = ((double) (mWidth - 26)) / mPagex;
                    break;
                  case FITHEIGHT:
                    mZoomrate = ((double) (mHeight - 26)) / mPagey;
                    break;
                  case CUSTOM:
                    break;
                  default:
                    break;
                  }

                mZoominit = true;
              }

            mPagenum = poppler_page_get_index (mPage);

            refresh ();

            scrollto (s);
          }
      }

  void 
    ApvlvDoc::nextpage (int times)
      { 
        showpage (mPagenum + times); 
      }

  void 
    ApvlvDoc::prepage (int times) 
      { 
        showpage (mPagenum - times); 
      }

  void
    ApvlvDoc::refresh ()
      {
        if (mDoc == NULL)
          return;

        mCurrentCache->set (mPagenum, false);
        gtk_image_set_from_pixbuf (GTK_IMAGE (mImage), mCurrentCache->getbuf (true));

        mStatus->show ();

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

        if (times % 2 != 0)
          {
            if (sr > 0.5)
              {
                sr = 0;
                rtimes += 1;
              }
            else
              {
                sr = 1;
              }
          }

        showpage (mPagenum + rtimes, sr);
      }

  void
    ApvlvDoc::halfprepage (int times)
      {
        double sr = scrollrate ();
        int rtimes = times / 2;

        if (times % 2 != 0)
          {
            if (sr < 0.5)
              {
                sr = 1;
                rtimes += 1;
              }
            else
              {
                sr = 0;
              }
          }

        showpage (mPagenum - rtimes, sr);
      }

  double
    ApvlvDoc::scrollrate ()
      {
        double maxv = mVaj->upper - mVaj->lower - mVaj->page_size;
        double val =  mVaj->value / maxv;
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
        if (mVaj->upper != mVaj->lower)
          {
            double maxv = mVaj->upper - mVaj->lower - mVaj->page_size;
            double val = maxv * s;
            gtk_adjustment_set_value (mVaj, val);
            mStatus->show ();
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
        if (mDoc == NULL)
          return;

        gdouble val = gtk_adjustment_get_value (mVaj);
        mVrate = (mVaj->upper - mVaj->lower) / mLines;
        if (val - mVrate * times > mVaj->lower)
          {
            gtk_adjustment_set_value (mVaj, val - mVrate * times);
          }
        else if (val > mVaj->lower)
          {
            gtk_adjustment_set_value (mVaj, mVaj->lower);
          }
        else
          {
            showpage (mPagenum - 1, 1.00);
          }

        mStatus->show ();
      }

  void
    ApvlvDoc::scrolldown (int times)
      {
        if (mDoc == NULL)
          return;

        gdouble val = gtk_adjustment_get_value (mVaj);
        mVrate = (mVaj->upper - mVaj->lower) / mLines;
        if (val + mVrate * times + mVaj->page_size < mVaj->upper)
          {
            gtk_adjustment_set_value (mVaj, val + mVrate * times);
          }
        else if (val + mVaj->page_size < mVaj->upper)
          {
            gtk_adjustment_set_value (mVaj, mVaj->upper - mVaj->page_size);
          }
        else
          {
            showpage (mPagenum + 1, 0.00);
          }

        mStatus->show ();
      }

  void
    ApvlvDoc::scrollleft (int times)
      {
        if (mDoc == NULL)
          return;

        mHrate = (mHaj->upper - mHaj->lower) / mChars;
        gdouble val = mHaj->value - mHrate * times;
        if (val > mVaj->lower)
          {
            gtk_adjustment_set_value (mHaj, val);
          }
        else
          {
            gtk_adjustment_set_value (mHaj, mHaj->lower);
          }
      }

  void
    ApvlvDoc::scrollright (int times)
      {
        if (mDoc == NULL)
          return;

        mHrate = (mHaj->upper - mHaj->lower) / mChars;
        gdouble val = mHaj->value + mHrate * times;
        if (val + mHaj->page_size < mHaj->upper)
          {
            gtk_adjustment_set_value (mHaj, val);
          }
        else
          {
            gtk_adjustment_set_value (mHaj, mHaj->upper - mHaj->page_size);
          }
      }

  void
    ApvlvDoc::markselection ()
      {
        PopplerRectangle *rect = (PopplerRectangle *) mResults->data;

        gchar *txt = poppler_page_get_text (mCurrentCache->getpage (), POPPLER_SELECTION_GLYPH, rect);
        if (txt == NULL)
          {
            debug ("no search result");
            return;
          }

        // Caculate the correct position
        gint x1 = (gint) ((rect->x1) * mZoomrate);
        gint y1 = (gint) ((mPagey - rect->y2) * mZoomrate);
        gint x2 = (gint) ((rect->x2) * mZoomrate);
        gint y2 = (gint) ((mPagey - rect->y1) * mZoomrate);

        // make the selection at the page center
        gdouble val = ((y1 + y2) - mVaj->page_size) / 2;
        if (val + mVaj->page_size > mVaj->upper)
          {
            gtk_adjustment_set_value (mVaj, mVaj->upper);
          }
        else if (val > 0)
          {
            gtk_adjustment_set_value (mVaj, val);
          }
        else
          {
            gtk_adjustment_set_value (mVaj, mVaj->lower);
          }
        val = ((x1 + x2) - mHaj->page_size) / 2;
        if (val + mHaj->page_size > mHaj->upper)
          {
            gtk_adjustment_set_value (mHaj, mHaj->upper);
          }
        else if (val > 0)
          {
            gtk_adjustment_set_value (mHaj, val);
          }
        else
          {
            gtk_adjustment_set_value (mHaj, mHaj->lower);
          }

        guchar *pagedata = mCurrentCache->getdata (true);
        GdkPixbuf *pixbuf = mCurrentCache->getbuf (true);
        // change the back color of the selection
        for (gint y = y1; y < y2; y ++)
          {
            for (gint x = x1; x < x2; x ++)
              {
                gint p = (gint) (y * 3 * mPagex * mZoomrate + (x * 3));
                pagedata[p + 0] = 0xFF - pagedata[p + 0];
                pagedata[p + 1] = 0xFF - pagedata[p + 0];
                pagedata[p + 2] = 0xFF - pagedata[p + 0];
              }
          }

        gtk_image_set_from_pixbuf (GTK_IMAGE (mImage), pixbuf);

        g_free (rect);
        mResults = g_list_remove (mResults, rect);
      }

  GList *
    ApvlvDoc::searchpage (int num)
      {
        if (mDoc == NULL)
          return NULL;

        PopplerPage *page = poppler_document_get_page (mDoc, num);

        GList *ret = poppler_page_find_text (page, mSearchstr.c_str ());

        return ret;
      }

  bool
    ApvlvDoc::needsearch (const char *str)
      {
        if (mDoc == NULL)
          return false;

        if (strlen (str) > 0)
          {
            g_list_free (mResults);
            mResults = NULL;

            mSearchstr = str;

            return true;
          }
        else
          {
            if (mResults != NULL)
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
            int num = poppler_document_get_n_pages (mDoc);
            int i = strlen (str) > 0? mPagenum - 1: mPagenum;
            while (i ++ < num - 1)
              {
                mResults = searchpage (i);
                if (mResults != NULL)
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
            poppler_document_get_n_pages (mDoc);
            int i = strlen (str) > 0? mPagenum + 1: mPagenum;
            while (i -- > 0)
              {
                mResults = g_list_reverse (searchpage (i));
                if (mResults != NULL)
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
        if (mRotatevalue == 90
            || mRotatevalue == 270)
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
        if (mDoc == NULL)
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
        mStatus->active (act);
        mActive = act;
      }

  bool
    ApvlvDoc::rotate (int ct)
      {
        // just hack
        if (ct == 1) ct = 90;

        if (ct % 90 != 0)
          {
            warn ("Not a 90 times value, ignore.");
            return false;
          }

        mRotatevalue += ct;
        while (mRotatevalue < 0)
          {
            mRotatevalue += 360;
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
        data->doc = mDoc;
        data->frmpn = mPagenum;
        data->endpn = mPagenum;

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
        return doc->scrollto (doc->mScrollvalue) == TRUE? FALSE: TRUE;
      }

  gboolean
    ApvlvDoc::apvlv_doc_first_copy_cb (gpointer data)
      {
        ApvlvDoc *doc = (ApvlvDoc *) data;
        doc->loadfile (doc->mFilestr, false);
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
      mDoc = dc;
      mPagenum = -1;
      mData = NULL;
      mBuf = NULL;
#ifdef HAVE_PTHREAD
      mTimer = -1;
      mThreadRunning = false;
      pthread_cond_init (&mCond, NULL);
      pthread_mutex_init (&mMutex, NULL);
#endif
    }

  void
    ApvlvDocCache::set (guint p, bool delay)
      {
#ifdef HAVE_PTHREAD
        if (mThreadRunning)
          {
            pthread_cancel (mTid);
          }
#endif
        mPagenum = p;
        if (mData != NULL)
          {
            delete []mData;
            mData = NULL;
          }
        if (mBuf != NULL)
          {
            g_object_unref (mBuf);
            mBuf = NULL;
          }
#ifdef HAVE_PTHREAD
        pthread_cond_init (&mCond, NULL);

        if (delay == true)
          {
            mTimer = g_timeout_add (50, (gboolean (*) (void *)) delayload, this);
          }
        else
          {
            pthread_create (&mTid, NULL, (void *(*) (void *)) load, this);
          }
#else
        load (this);
#endif
      }

#ifdef HAVE_PTHREAD
  gboolean
    ApvlvDocCache::delayload (ApvlvDocCache *ac)
      {
        pthread_create (&ac->mTid, NULL, (void *(*) (void *)) load, ac);
        ac->mTimer = -1;
        return FALSE;
      }
#endif

  void
    ApvlvDocCache::load (ApvlvDocCache *ac)
      {
        int c = poppler_document_get_n_pages (ac->mDoc->getdoc ());
        if (ac->mPagenum < 0
            || ac->mPagenum >= c)
          {
            debug ("no this page: %d", ac->mPagenum);
            return;
          }

#ifdef HAVE_PTHREAD
        ac->mThreadRunning = true;
#endif

        PopplerPage *tpage = poppler_document_get_page (ac->mDoc->getdoc (), ac->mPagenum);

        double tpagex, tpagey;
        ac->mDoc->getpagesize (tpage, &tpagex, &tpagey);

        int ix = (int) (tpagex * ac->mDoc->zoomvalue ()), iy = (int) (tpagey * ac->mDoc->zoomvalue ());

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
        poppler_page_render_to_pixbuf (tpage, 0, 0, ix, iy, ac->mDoc->zoomvalue (), ac->mDoc->getrotate (), bu);
#ifdef HAVE_PTHREAD
        pthread_mutex_unlock (&rendermutex);
#endif

        ac->mPage = tpage;
        ac->mData = dat;
        ac->mBuf = bu;

#ifdef HAVE_PTHREAD
        pthread_cond_signal (&ac->mCond);

        ac->mThreadRunning = false;
#endif
      }

  ApvlvDocCache::~ApvlvDocCache ()
    {
#ifdef HAVE_PTHREAD
      if (mTimer > 0)
        {
          g_source_remove (mTimer);
        }
      if (mThreadRunning)
        {
          pthread_cancel (mTid);
        }
#endif
      if (mData != NULL)
        delete []mData;
      if (mBuf != NULL)
        g_object_unref (mBuf);
    }

  PopplerPage *
    ApvlvDocCache::getpage ()
      {
        return mPage;
      }

  guint
    ApvlvDocCache::getpagenum ()
      {
        return mPagenum;
      }

  /*
   * get the cache data
   * @param: wait, if not wait, not wait the buffer be prepared
   * @return: the buffer
   * */
  guchar *ApvlvDocCache::getdata (bool wait)
    {
#ifndef HAVE_PTHREAD
      return mData;
#else
      if (!wait)
        {
          return mData;
        }

      guchar *dat = mData;
      if (dat == NULL)
        {
          pthread_mutex_lock (&mMutex);
          pthread_cond_wait (&mCond, &mMutex);
          dat = mData;
          pthread_mutex_unlock (&mMutex);
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
      return mBuf;
#else
      if (!wait)
        {
          return mBuf;
        }

      GdkPixbuf *bu = mBuf;
      if (bu == NULL)
        {
          pthread_mutex_lock (&mMutex);
          pthread_cond_wait (&mCond, &mMutex);
          bu = mBuf;
          pthread_mutex_unlock (&mMutex);
        }
      return bu;
#endif
    }

  ApvlvDocStatus::ApvlvDocStatus (ApvlvDoc *dc)
    {
      mDoc = dc;
      mVbox = gtk_hbox_new (FALSE, 0);
      for (int i=0; i<AD_STATUS_SIZE; ++i)
        {
          mStlab[i] = gtk_label_new ("");
          gtk_box_pack_start (GTK_BOX (mVbox), mStlab[i], FALSE, FALSE, 0);
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
            gtk_widget_modify_fg (mStlab[i], GTK_STATE_NORMAL, &c);
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
            gtk_widget_set_usize (mStlab[i], sw[i], h);
          }
      }

  void
    ApvlvDocStatus::show ()
      {
        if (mDoc->filename ())
          {
            char temp[AD_STATUS_SIZE][256];
            gchar *bn;
            bn = g_path_get_basename (mDoc->filename ());
            snprintf (temp[0], sizeof temp[0], "%s", bn);
            snprintf (temp[1], sizeof temp[1], "%d/%d", mDoc->pagenumber (), mDoc->pagesum ());
            snprintf (temp[2], sizeof temp[2], "%d%%", (int) (mDoc->zoomvalue () * 100));
            snprintf (temp[3], sizeof temp[3], "%d%%", (int) (mDoc->scrollrate () * 100));
            for (unsigned int i=0; i<AD_STATUS_SIZE; ++i)
              {
                gtk_label_set_text (GTK_LABEL (mStlab[i]), temp[i]);
              }
            g_free (bn);
          }
      }
}

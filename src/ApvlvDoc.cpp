/****************************************************************************
 * Copyright (c) 1998-2005,2006 Free Software Foundation, Inc.              
 *                                                                          
 * Permission is hereby granted, free of charge, to any person obtaining a  
 * copy of this software and associated documentation files (the            
 * "Software"), to deal in the Software without restriction, including      
 * without limitation the rights to use, copy, modify, merge, publish,      
 * distribute, distribute with modifications, sublicense, and/or sell       
 * copies of the Software, and to permit persons to whom the Software is    
 * furnished to do so, subject to the following conditions:                 
 *                                                                          
 * The above copyright notice and this permission notice shall be included  
 * in all copies or substantial portions of the Software.                   
 *                                                                          
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS  
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF               
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.   
 * IN NO EVENT SHALL THE ABOVE COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,   
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR    
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR    
 * THE USE OR OTHER DEALINGS IN THE SOFTWARE.                               
 *                                                                          
 * Except as contained in this notice, the name(s) of the above copyright   
 * holders shall not be used in advertising or otherwise to promote the     
 * sale, use or other dealings in this Software without prior written       
 * authorization.                                                           
****************************************************************************/

/****************************************************************************
 *  Author:    YuPengda
 *  AuthorRef: Alf <naihe2010@gmail.com>
 *  Blog:      http://naihe2010.cublog.cn
****************************************************************************/
#include "ApvlvUtil.hpp"
#include "ApvlvDoc.hpp"

#include <stdlib.h>
#include <string.h>

#ifdef HAVE_PTHREAD
#include <pthread.h>
#endif

#include <gtk/gtk.h>
#include <glib/poppler.h>

#include <iostream>
#include <fstream>
#include <sstream>

namespace apvlv
{
  ApvlvDoc::ApvlvDoc (const char *zm)
    {
      mCurrentCache = NULL;
      mCacheBrother = 2;
#ifdef HAVE_PTHREAD
      pthread_mutex_init (&mutex, NULL);
#endif

      doc = NULL;

      page = NULL;

      results = NULL;
      searchstr = "";

      scrollwin = gtk_scrolled_window_new (NULL, NULL);

      image = gtk_image_new ();
      gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (scrollwin),
                                             image);
      gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrollwin),
                                      GTK_POLICY_AUTOMATIC,
                                      GTK_POLICY_AUTOMATIC);

      vaj = gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW (scrollwin));
      haj = gtk_scrolled_window_get_hadjustment (GTK_SCROLLED_WINDOW (scrollwin));

      setzoom (zm);
    }

  ApvlvDoc::~ApvlvDoc ()
    {
      if (filestr != helppdf)
        {
          savelastposition ();
        }
      positions.clear ();
      gtk_widget_destroy (scrollwin);
    }

  void
    ApvlvDoc::setsize (int w, int h)
      {
        width = w;
        height = h;
        gtk_widget_set_usize (widget (), width, height);
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
        char *path = absolutepath ("~/.apvlvinfo");

        ofstream os (path, ios::app);

        if (os.is_open ())
          {
            os << ">";
            os << filename () << "\t";
            os << pagenum << "\t";
            os << scrollrate ();
            os << "\n";
            os.close ();
          }
      }

  bool
    ApvlvDoc::loadlastposition ()
      {
        char *path = absolutepath ("~/.apvlvinfo");

        ifstream os (path, ios::in);

        if (os.is_open ())
          {
            string line;
            int pagen = 0;
            double scrollr = 0.00;

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
                      }
                  }
              }

            scrollvalue = scrollr;
            showpage (pagen);

            // Warning
            // I can't think a better way to scroll correctly when 
            // the page is not be displayed correctly
            gtk_timeout_add (50, apvlv_doc_first_scroll_cb, this);

            os.close ();
          }
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

        gchar *uri = g_filename_to_uri (filename, NULL, NULL);
        if (uri == NULL)
          {
            cerr << "Can't convert" << filename << "to a valid uri";
            return false;
          }

        doc = poppler_document_new_from_file (uri, NULL, NULL);
        g_free (uri);

        if (doc != NULL)
          {
            clearcache ();
            zoominit = false;
            lines = 50;
            chars = 80;
            filestr = filename;
            loadlastposition ();
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
        return scrollwin;
      }

  PopplerPage *
    ApvlvDoc::getpage (int p)
      {
        int rp = convertindex (p);
        if (rp < 0)
          return NULL;

        return poppler_document_get_page (doc, rp);
      }

  int
    ApvlvDoc::convertindex (int p)
      {
        if (doc != NULL)
          {
            int c = poppler_document_get_n_pages (doc);
            if (0 <= p && p < c)
              {
                return p;
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
        pthread_mutex_lock (&mutex);
        bool hasCache = false;
        if (mCurrentCache != NULL)
          {
            int nd = rp - pagenum;
            if (0 <= nd && nd <= mCacheBrother)
              {
                for (int i=0; i<nd; ++i) 
                  mCurrentCache = mCurrentCache->next;
                hasCache = true;
              }
            else if (nd < 0 && nd > (0 - mCacheBrother))
              {
                for (int i=0; i<0-nd; ++i) 
                  mCurrentCache = mCurrentCache->prev;
                hasCache = true;
              }
          }

        if (hasCache)
          {
            pagenum = rp;
            gtk_image_set_from_pixbuf (GTK_IMAGE (image), mCurrentCache->buf);
            pthread_mutex_unlock (&mutex);      /* Dangerous !!! */
            scrollto (s);
            pthread_create (&tid, NULL, (void *(*) (void *)) prepare_func, this);
            return;
          }
        pthread_mutex_unlock (&mutex);
#endif

        page = getpage (p);
        if (page != NULL)
          {
            poppler_page_get_size (page, &pagex, &pagey);

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

#ifdef HAVE_PTHREAD
  void *
    ApvlvDoc::prepare_func (ApvlvDoc *adoc)
      {
        if (adoc->doc == NULL)
          return NULL;

        ApvlvDocCache *ac, *ac2;
        int i;

        pthread_mutex_lock (&adoc->mutex);
        for (ac = adoc->mCurrentCache, i=0; i<adoc->mCacheBrother; ++i)
          {
            ac2 = ac->prev;
            if (ac2 == NULL)
              {
                ac2 = adoc->newcache (adoc->pagenum - i - 1);
                if (ac2 == NULL) break;
                adoc->insertcache (NULL, ac, ac2);
              }
            ac = ac2;
          }

        while ((ac2 = ac->prev) != NULL)
          {
            adoc->removecache (ac2);
            adoc->deletecache (ac2);
          }

        for (ac = adoc->mCurrentCache, i=0; i<adoc->mCacheBrother; ++i)
          {
            ac2 = ac->next;
            if (ac2 == NULL)
              {
                ac2 = adoc->newcache (adoc->pagenum + i + 1);
                if (ac2 == NULL) break;
                adoc->insertcache (ac, NULL, ac2);
              }
            ac = ac2;
          }

        while ((ac2 = ac->next) != NULL)
          {
            adoc->removecache (ac2);
            adoc->deletecache (ac2);
          }
        pthread_mutex_unlock (&adoc->mutex);
      }
#endif

  ApvlvDocCache *
    ApvlvDoc::newcache (int pn)
      {
        PopplerPage *tpage;
        double tpagex, tpagey;

        tpage = getpage (pn);
        if (tpage == NULL)
          {
            debug ("no this page");
            return NULL;
          }
        debug ("get page: %d:(%p)", pn, tpage);

        poppler_page_get_size (tpage, &tpagex, &tpagey);

        int ix = (int) (tpagex * zoomrate), iy = (int) (tpagey * zoomrate);

        ApvlvDocCache *ac = new ApvlvDocCache;
        ac->prev = ac->next = NULL;
        ac->pagenum = pn;
        ac->page = tpage;
        ac->data = (guchar *) new char[ix * iy * 3];
        ac->buf = gdk_pixbuf_new_from_data (ac->data, GDK_COLORSPACE_RGB,
                                    FALSE,
                                    8,
                                    ix, iy,
                                    3 * ix,
                                    NULL, NULL);

        poppler_page_render_to_pixbuf (tpage, 0, 0, ix, iy, zoomrate, 0, ac->buf);

        return ac;
      }

  void
    ApvlvDoc::deletecache (ApvlvDocCache *ac)
      {
        debug ("delete page: %d:(%p)", ac->pagenum, ac->page);
        delete ac->data;
        g_object_unref (ac->buf);
      }

  void
    ApvlvDoc::insertcache (ApvlvDocCache *prev, ApvlvDocCache *next, ApvlvDocCache *n)
      {
        if (prev != NULL)
          {
            n->next = prev->next;
            prev->next = n;
            n->prev = prev;
          }
        else if (next != NULL)
          {
            n->prev = next->prev;
            next->prev = n;
            n->next = next;
          }
      }

  void
    ApvlvDoc::removecache (ApvlvDocCache *c)
      {
        if (c->prev != NULL)
          c->prev->next = c->next;

        if (c->next != NULL)
          c->next->prev = c->prev;
      }

  void
    ApvlvDoc::clearcache ()
      {
        ApvlvDocCache *ac, *ac2;
        if (mCurrentCache == NULL)
          return;

        for (ac = mCurrentCache->prev; ac != NULL; ac = ac2)
          {
            ac2 = ac->prev;
            deletecache (ac);
          }
        for (ac = mCurrentCache->next; ac != NULL; ac = ac2)
          {
            ac2 = ac->next;
            deletecache (ac);
          }

        deletecache (mCurrentCache);
        mCurrentCache = NULL;
      }

  void 
    ApvlvDoc::refresh ()
      {
        if (doc == NULL)
          return;

        clearcache ();
        mCurrentCache = newcache (pagenum);
        gtk_image_set_from_pixbuf (GTK_IMAGE (image), mCurrentCache->buf);
#ifdef HAVE_PTHREAD
        pthread_create (&tid, NULL, (void *(*) (void *)) prepare_func, this);
#endif
      }

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

  void
    ApvlvDoc::scrollto (double s)
      {
        double maxv = vaj->upper - vaj->lower - vaj->page_size;
        double val = maxv * s;
        gtk_adjustment_set_value (vaj, val);
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

        guchar *pagedata = mCurrentCache->data;
        GdkPixbuf *pixbuf = mCurrentCache->buf;
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
            int num = poppler_document_get_n_pages (doc);
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

  gboolean 
    ApvlvDoc::apvlv_doc_first_scroll_cb (gpointer data)
      {
        ApvlvDoc *doc = (ApvlvDoc *) data;
        doc->scrollto (doc->scrollvalue);
        return FALSE;
      }

  gboolean 
    ApvlvDoc::apvlv_doc_first_copy_cb (gpointer data)
      {
        ApvlvDoc *doc = (ApvlvDoc *) data;
        doc->loadfile (doc->filestr, false);
        return FALSE;
      }
}

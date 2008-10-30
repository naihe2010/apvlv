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

#include <gtk/gtk.h>
#include <glib/poppler.h>

#include <iostream>
#include <fstream>
#include <sstream>

namespace apvlv
{
  ApvlvDoc::ApvlvDoc (const char *zm)
    {
      doc = NULL;

      pagedata = NULL;
      pixbuf = NULL;
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
        if (doc != NULL)
          {
            int c = poppler_document_get_n_pages (doc);
            if (0 <= p && p < c)
              {
                return poppler_document_get_page (doc, p);
              }
            else if (p < 0)
              {
                return poppler_document_get_page (doc, c + p);
              }
          }

        return NULL;
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

            refresh ();

            scrollto (s);

            pagenum = poppler_page_get_index (page);
          }
      }

  void 
    ApvlvDoc::refresh ()
      {
        if (doc == NULL)
          return;

        int ix = (int) (pagex * zoomrate), iy = (int) (pagey * zoomrate);

        if (pagedata != NULL)
          {
            delete []pagedata;
          }
        pagedata = (guchar *) new char[ix * iy * 3];

        if (pixbuf)
          {
            g_object_unref (G_OBJECT (pixbuf));
            pixbuf = NULL;
          }
        pixbuf =
          gdk_pixbuf_new_from_data (pagedata, GDK_COLORSPACE_RGB,
                                    FALSE,
                                    8,
                                    ix, iy,
                                    3 * ix,
                                    NULL, NULL);

        poppler_page_render_to_pixbuf (page, 0, 0, ix, iy, zoomrate, 0, pixbuf);

        gtk_image_set_from_pixbuf (GTK_IMAGE (image), pixbuf);
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

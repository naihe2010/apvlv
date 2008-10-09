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
#include "ApvlvDoc.hpp"

#include <gtk/gtk.h>
#include <glib/poppler.h>

#include <iostream>

namespace apvlv
{
  ApvlvDoc::ApvlvDoc (const char *zm, GtkWidget *v, GtkWidget *h)
    {
      zoominit = false;
      doc = NULL;
      pagedata = NULL;
      pixbuf = NULL;
      page = NULL;
      results = NULL;
      searchstr = "";

      v? vbox = v: vbox = gtk_vbox_new (TRUE, 0);
      h? hbox = h: hbox = gtk_hbox_new (TRUE, 0);

      gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 0);

      scrollwin = gtk_scrolled_window_new (NULL, NULL);
      gtk_box_pack_start (GTK_BOX (hbox), scrollwin, TRUE, TRUE, 0);

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
      gtk_widget_destroy (vbox);
    }

  void 
    ApvlvDoc::vseperate ()
    {
      ApvlvDoc *ndoc = new ApvlvDoc ("NORMAL", vbox, NULL);
      vchildren.insert (vchildren.end (), ndoc);
    }

  void 
    ApvlvDoc::hseperate ()
    {
      ApvlvDoc *ndoc = new ApvlvDoc ("NORMAL", NULL, hbox);
      hchildren.insert (vchildren.end (), ndoc);
    }

  bool 
    ApvlvDoc::loadfile (const char *filename)
  {
    gchar *uri = g_filename_to_uri (filename, NULL, NULL);
    if (uri == NULL)
      {
	cerr << "Can't convert" << filename << "to a valid uri";
        return FALSE;
      }

    doc = poppler_document_new_from_file (uri, NULL, NULL);
    g_free (uri);

    return doc == NULL? FALSE: TRUE;
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

  PopplerPage *
    ApvlvDoc::getpage (int p)
      {
        if (doc != NULL)
          {
            int c = poppler_document_get_n_pages (doc);
            if (0 <= p && p < c)
              return poppler_document_get_page (doc, p);
          }

        return NULL;
      }

  void 
    ApvlvDoc::showpage (int p)
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

            pagenum = p;

            refresh ();
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

        vrate = (vaj->upper - vaj->lower) / 50;
        hrate = (haj->upper - haj->lower) / 78;
      }

  void
    ApvlvDoc::scrollup (int times)
      {
        if (doc == NULL)
          return;

        gdouble val = gtk_adjustment_get_value (vaj);
        val -= vrate * times;
        if (val > vaj->lower)
          {
            gtk_adjustment_set_value (vaj, val);
          }
        else
          {
            if (pagenum == 0) 
              {
                gtk_adjustment_set_value (vaj, vaj->lower);
              }
            else
              {
                prepage ();
                gtk_adjustment_set_value (vaj, vaj->upper);
              }
          }
      }

  void
    ApvlvDoc::scrolldown (int times)
      {
        if (doc == NULL)
          return;

        gdouble val = gtk_adjustment_get_value (vaj);
        val += vrate * times;
        if (val + vaj->page_size < vaj->upper)
          {
            gtk_adjustment_set_value (vaj, val);
          }
        else 
          {
            if (pagenum == poppler_document_get_n_pages (doc) - 1)
              {
                gtk_adjustment_set_value (vaj, vaj->upper);
              }
            else
              {
                nextpage ();
                gtk_adjustment_set_value (vaj, vaj->lower);
              }
          }
      }

  void
    ApvlvDoc::scrollleft (int times)
      {
        if (doc == NULL)
          return;

        gdouble val = gtk_adjustment_get_value (haj);
        val -= hrate * times;
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

        gdouble val = gtk_adjustment_get_value (haj);
        val += hrate * times;
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
}

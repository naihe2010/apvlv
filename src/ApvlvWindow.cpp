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
#include "ApvlvView.hpp"
#include "ApvlvParams.hpp"
#include "ApvlvWindow.hpp"

#include <gtk/gtk.h>

namespace apvlv
{
  ApvlvWindow::ApvlvWindow (ApvlvDoc *doc)
    {
      type = AW_DOC;
      if (doc == NULL)
        {
          m_Doc = new ApvlvDoc (gParams->settingvalue ("zoom"));
        }
      else
        {
          m_Doc = doc;
        }
      m_prev = m_next = m_parent = m_child = NULL;
    }

  ApvlvWindow::~ApvlvWindow ()
    {
      cout << __LINE__ << ":" << __func__ << ": " << "delete a window: " << this << endl;
      if (type == AW_DOC && gView->hasloaded (m_Doc->filename ()) != m_Doc)
        {
          delete m_Doc;
        }

      if (m_parent != NULL)
        {
          if (m_parent->m_child == this)
            {
              m_parent->m_child = m_next;
            }

          if (m_prev != NULL)
            {
              m_prev->m_next = m_next;
            }
          if (m_next != NULL)
            {
              m_next->m_prev = m_prev;
            }
        }

      if (type != AW_DOC)
        {
          ApvlvWindow *nwin;
          while ((nwin = m_child) != NULL)
            {
              delete nwin;
            }
        }
    }

  ApvlvWindow *
    ApvlvWindow::getneighbor (const char *s)
      {
        if (strcmp (s, "C-w") == 0)
          {
            return getnext (1);
          }
        else if (strcmp (s, "k") == 0)
          {
            return getv (1, false);
          }
        else if (strcmp (s, "j") == 0)
          {
            return getv (1, true);
          }
        else if (strcmp (s, "h") == 0)
          {
            return geth (1, false);
          }
        else if (strcmp (s, "l") == 0)
          {
            return geth (1, true);
          }
      }

  inline ApvlvWindow *
    ApvlvWindow::getv (int num, bool next)
      {
        ApvlvWindow *w, *nw, *nfw;

        for (w = this; w != NULL; w = w->m_parent)
          {
            if (w->m_parent == NULL)
              return NULL;
            if (next == false)
              nw = w->m_prev;
            else
              nw = w->m_next;
            if (w->m_parent->type == AW_SP && nw != NULL)
              break;
          }

        for (;;)
          {
            if (nw->type == AW_DOC)
              {
                nfw = nw;
                break;
              }
            nfw = nw->m_child;
            if (nw->type == AW_SP)
              {
                while (nfw->m_next != NULL)
                  nfw = nfw->m_next;
              }
            if (nw->type == AW_VSP && !next)
              {
              while (nw->m_next != NULL)
                nw = nw->m_next;
              }
            nfw = nw;
          }
end:
        if (nfw != NULL)
          return nfw;
      }

  inline ApvlvWindow *
    ApvlvWindow::geth (int num, bool next)
      {
        ApvlvWindow *w, *nw, *nfw;

        for (w = this; w != NULL; w = w->m_parent)
          {
            if (w->m_parent == NULL)
              return NULL;
            if (next == false)
              nw = w->m_prev;
            else
              nw = w->m_next;
            if (w->m_parent->type == AW_SP && nw != NULL)
              break;
          }

        for (;;)
          {
            if (nw->type == AW_DOC)
              {
                nfw = nw;
                break;
              }
            nfw = nw->m_child;
            if (nw->type == AW_SP)
              {
                while (nfw->m_next != NULL)
                  nfw = nfw->m_next;
              }
            if (nw->type == AW_VSP && !next)
              {
              while (nw->m_next != NULL)
                nw = nw->m_next;
              }
            nfw = nw;
          }
end:
        if (nfw != NULL)
          return nfw;
      }

  inline ApvlvWindow *
    ApvlvWindow::getnext (int num)
      {
        ApvlvWindow *n = getv (num, true);
        if (n == NULL)
          {
            n = geth (num, true);
            if (n == NULL)
              {
                n = geth (num, false);
                if (n == NULL)
                  n = getv (num, false);
              }
          }
        return n;
      }

  ApvlvWindow *
    ApvlvWindow::birth (ApvlvDoc *doc)
      {
        if (doc == NULL)
          {
            doc = m_Doc->copy ();
          }
        ApvlvWindow *nwindow = new ApvlvWindow (doc);
        
        nwindow->m_parent = this;

        if (m_child == NULL)
          {
            m_child = nwindow;
          }
        else
          {
            ApvlvWindow *nw;
            for (nw = m_child; nw->m_next != NULL; nw = nw->m_next);
            nw->m_next = nwindow;
            nwindow->m_prev = nw;
          }

        return nwindow;
      }

  ApvlvWindow *
    ApvlvWindow::separate (bool vsp)
      {
        int ttype = vsp == false? AW_SP: AW_VSP;

        if (m_parent != NULL && m_parent->type == ttype)
          {
            ApvlvWindow *nwin = m_parent->birth ();
            cout << __LINE__ << ":" << __func__ << ": create a new window: " << nwin << endl;
            gtk_box_pack_start (GTK_BOX (m_parent->m_box), nwin->widget (), TRUE, TRUE, 0);
            gtk_widget_show_all (m_parent->m_box);
            return this;
          }

        GtkWidget *old = widget ();
        GtkWidget *parent = gtk_widget_get_parent (old);
        g_object_ref (G_OBJECT (old));
        gtk_container_remove (GTK_CONTAINER (parent), old);

        ApvlvWindow *nwindow = birth (m_Doc);
        ApvlvWindow *nwindow2 = birth ();
        cout << __LINE__ << ":" << __func__ << ": create 2 new window: " \
          << nwindow << "&" << nwindow2 << endl;

        type = windowType (ttype);

        m_box = type == AW_SP? gtk_vbox_new (TRUE, 2): gtk_hbox_new (TRUE, 2);
        gtk_container_add (GTK_CONTAINER (parent), m_box);

        gtk_box_pack_start (GTK_BOX (m_box), nwindow->widget (), TRUE, TRUE, 0);
        gtk_box_pack_end (GTK_BOX (m_box), nwindow2->widget (), TRUE, TRUE, 0);
        gtk_widget_show_all (parent);

        return nwindow;
      }

  void
    ApvlvWindow::setsize (int width, int height)
      {
        m_width = width;
        m_height = height;
        if (m_Doc)
          {
            m_Doc->setsize (m_width, m_height);
          }
      }

  ApvlvDoc *
    ApvlvWindow::loadDoc (const char *filename)
      {
        if (m_Doc->filename () == NULL || gView->hasloaded (m_Doc->filename ()) != m_Doc)
          {
            m_Doc->setsize (m_width, m_height);
            bool ret = m_Doc->loadfile (filename);
            return ret? m_Doc: NULL;
          }

        ApvlvDoc *ndoc = new ApvlvDoc (gParams->settingvalue ("zoom"));
        ndoc->setsize (m_width, m_height);
        bool ret = ndoc->loadfile (filename);
        if (ret)
          {
            GtkWidget *parent = gtk_widget_get_parent (widget ());
            g_object_ref (G_OBJECT (widget ()));
            gtk_container_remove (GTK_CONTAINER (parent), widget ());

            m_Doc = ndoc;
            gtk_container_add (GTK_CONTAINER (parent), ndoc->widget ());
            gtk_widget_show_all (parent);
            return ndoc;
          }
        else
          {
            delete ndoc;
            return NULL;
          }
      }

  void
    ApvlvWindow::setDoc (ApvlvDoc *doc)
      {
        GtkWidget *parent = gtk_widget_get_parent (widget ());
        g_object_ref (G_OBJECT (widget ()));
        gtk_container_remove (GTK_CONTAINER (parent), widget ());

        doc->setsize (m_width, m_height);
        m_Doc = doc;

        gtk_container_add (GTK_CONTAINER (parent), doc->widget ());
        gtk_widget_show_all (parent);
      }

  void 
    ApvlvWindow::smaller (int times)
      {
      }

  void 
    ApvlvWindow::bigger (int times)
      {
      }
}

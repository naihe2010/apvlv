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
#include "ApvlvUtil.hpp"
#include "ApvlvParams.hpp"
#include "ApvlvWindow.hpp"

#include <gtk/gtk.h>

#include <string.h>

namespace apvlv
{
  int ApvlvWindow::times = 0;
  ApvlvWindow *ApvlvWindow::m_curWindow = NULL;

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
      ApvlvWindow *nwin, *cwin;

      if (type == AW_SP || type == AW_VSP)
        {
          while (nwin = m_child)
            {
              delete nwin;
            }
        }

      if (m_parent != NULL)
        {
          if (m_prev != NULL)
            {
              m_prev->m_next = m_next;
            }
          else      // if it has not a prev window, it must be the oldest brother of the parent
            {
              m_parent->m_child = m_next;
            }

          if (m_next != NULL)
            {
              m_next->m_prev = m_prev;
              setcurrentWindow (m_next);
            }

          if (m_parent->m_child && m_parent->m_child->m_next == NULL)
            {
              m_parent->unbirth ();
              setcurrentWindow (m_parent);
              return;
            }
        }

      if (type == AW_DOC
          && m_Doc != gView->hasloaded (m_Doc->filename ())
      )
        {
          delete m_Doc;
        }
      else
        {
          g_object_ref (widget ());
          GtkWidget *parent = gtk_widget_get_parent (widget ());
          gtk_container_remove (GTK_CONTAINER (parent), widget ());
        }
    }

  returnType
    ApvlvWindow::process (int ct, guint key)
      {
        ApvlvWindow *nwin;

        switch (key)
          {
          case CTRL ('w'):
          case 'k':
          case 'j':
          case 'h':
          case 'l':
            nwin = getneighbor (ct, key);
            if (nwin != NULL)
              {
                debug ("here");
                setcurrentWindow (nwin);
              }
            break;

          case '-':
            smaller (ct);
            break;

          case '+':
            bigger (ct);
            break;

          default:
            break;
          }
      }

  ApvlvWindow *
    ApvlvWindow::getneighbor (int ct, guint key)
      {
        switch (key)
          {
          case CTRL ('w'):
            return getnext (ct);
          case 'k':
            return getkj (1, false);
          case 'j':
            return getkj (1, true);
          case 'h':
            return gethl (1, false);
          case 'l':
            return gethl (1, true);
          default:
            break;
          }

        return NULL;
      }

  inline ApvlvWindow *
    ApvlvWindow::getkj (int num, bool down)
      {
        ApvlvWindow *bw, *w, *nw, *fw;
        int id;

        if (m_parent == NULL)
          return NULL;

        for (bw = NULL, w = this; w != NULL; bw = w, w = w->m_parent)
          {
            if (w->m_parent == NULL)
              {
                fw = nw;
                goto end;
              }

            if (down)
              nw = w->m_next;
            else
              nw = w->m_prev;

            if (w->m_parent->type == AW_SP && nw != NULL) break;
          }

        for (;;)
          {
            if (nw->type == AW_DOC)
              {
                fw = nw;
                break;
              }

            fw = nw->m_child;
            if (nw->type == AW_VSP)
              {
                while (fw->m_next != NULL
                       && bw
                       && bw != bw->m_parent->m_child)
                  {
                    fw = fw->m_next;
                    bw = bw->m_prev;
                  }
              }

            if (nw->type == AW_SP && !down)
              {
                while (fw->m_next != NULL)
                  fw = fw->m_next;
              }
            nw = fw;
          }

end:
        return fw;
      }

  inline ApvlvWindow *
    ApvlvWindow::gethl (int num, bool right)
      {
        ApvlvWindow *bw, *w, *nw, *fw;
        int id;

        if (m_parent == NULL)
          return NULL;

        for (bw = NULL, w = this; w != NULL; bw = w, w = w->m_parent)
          {
            if (w->m_parent == NULL)
              {
                fw = nw;
                goto end;
              }

            if (right)
              nw = w->m_next;
            else
              nw = w->m_prev;
            if (w->m_parent->type == AW_VSP && nw != NULL) break;
          }

        for (;;)
          {
            if (nw->type == AW_DOC)
              {
                fw = nw;
                break;
              }

            fw = nw->m_child;
            if (nw->type == AW_SP)
              {
                while (fw->m_next != NULL
                       && bw &&
                       bw != bw->m_parent->m_child)
                  {
                    fw = fw->m_next;
                    bw = bw->m_prev;
                  }
              }

            if (nw->type == AW_VSP && !right)
              {
                while (fw->m_next != NULL)
                  fw = fw->m_next;
              }
            nw = fw;
          }

end:
        return fw;
      }

  inline ApvlvWindow *
    ApvlvWindow::getnext (int num)
      {
        ApvlvWindow *n = getkj (num, true);
        if (n == NULL)
          {
            n = gethl (num, true);
            if (n == NULL)
              {
                n = gethl (num, false);
                if (n == NULL)
                  n = getkj (num, false);
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

        return nwindow;
      }

  ApvlvWindow *
    ApvlvWindow::unbirth ()
      {
        GtkWidget *wid;

        ApvlvWindow *oldchild = m_child;
        g_object_ref (oldchild->widget ());

        type = m_child->type;
        if (type == AW_SP || type == AW_VSP)
          {
            m_child = oldchild->m_child;
            m_child->m_parent = this;
            oldchild->m_child = NULL;
            oldchild->m_parent = NULL;
            wid = m_child->widget ();
          }
        else if (type == AW_DOC)
          {
            ApvlvDoc *odoc = oldchild->getDoc ();
            if (odoc == gView->hasloaded (odoc->filename ()))
              {
                m_Doc = odoc;
              }
            else
              {
                m_Doc = oldchild->getDoc ()->copy ();
              }
            wid = m_Doc->widget ();
          }

        delete oldchild;

        GtkWidget *parent = gtk_widget_get_parent (m_paned);
        gtk_container_remove (GTK_CONTAINER (parent), m_paned);
        gtk_container_add (GTK_CONTAINER (parent), wid);
        gtk_widget_show_all (parent);

        return this;
      }

  ApvlvWindow *
    ApvlvWindow::insertafter (ApvlvWindow *awin)
      {
        awin->m_prev = this;
        awin->m_next = m_next;
        if (m_next != NULL)
          {
            m_next->m_prev = awin;
          }
        m_next = awin;
      }

  ApvlvWindow *
    ApvlvWindow::insertbefore (ApvlvWindow *bwin)
      {
        if (m_prev != NULL)
          {
            m_prev->m_next = bwin;
            bwin->m_prev = m_prev;
          }
        bwin->m_next = this;
        m_prev = bwin;
        if (m_parent && m_parent->m_child == this)
          {
            m_parent->m_child = bwin;
          }
      }

  ApvlvWindow *
    ApvlvWindow::separate (bool vsp)
      {
        ApvlvWindow *nwindow, *nwindow2;
        int ttype = vsp == false? AW_SP: AW_VSP;
        GtkWidget *pan;

        GtkWidget *old = widget ();
        GtkWidget *parent = gtk_widget_get_parent (old);
        g_object_ref (G_OBJECT (old));
        gtk_container_remove (GTK_CONTAINER (parent), old);

        nwindow = birth (m_Doc);
        nwindow2 = birth ();
        m_child->insertafter (nwindow2);

        debug ("separate window: %p to 2 new windows: %p & %p", this, nwindow, nwindow2);

        type = windowType (ttype);

        m_paned = type == AW_SP? gtk_vpaned_new (): gtk_hpaned_new ();
        gtk_container_add (GTK_CONTAINER (parent), m_paned);

        gtk_paned_pack1 (GTK_PANED (m_paned), nwindow->widget (), TRUE, TRUE);
        gtk_paned_pack2 (GTK_PANED (m_paned), nwindow2->widget (), TRUE, TRUE);

        if (ttype == AW_SP)
          {
            nwindow->setsize (m_width, m_height / 2);
            nwindow2->setsize (m_width, m_height / 2);
          }
        else if (ttype == AW_VSP)
          {
            nwindow->setsize (m_width / 2, m_height);
            nwindow2->setsize (m_width / 2, m_height);
          }

        gtk_widget_show_all (m_paned);

        setcurrentWindow (nwindow);

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
        else
          {
            gtk_widget_set_usize (m_paned, m_width, m_height);
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
        if (m_parent == NULL) return;

        int val = gtk_paned_get_position (GTK_PANED (m_parent->m_paned));
        int len = 20 * times;
        m_parent->m_child == this? val -= len: val += len;
        gtk_paned_set_position (GTK_PANED (m_parent->m_paned), val);
      }

  void 
    ApvlvWindow::bigger (int times)
      {
        if (m_parent == NULL) return;

        int val = gtk_paned_get_position (GTK_PANED (m_parent->m_paned));
        int len = 20 * times;
        m_parent->m_child == this? val += len: val -= len;
        gtk_paned_set_position (GTK_PANED (m_parent->m_paned), val);
      }
}

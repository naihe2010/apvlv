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
/* @CPPFILE ApvlvWindow.cpp
*
*  Author: Alf <naihe2010@gmail.com>
*/
/* @date Created: 2008/09/30 00:00:00 Alf */

#include "ApvlvView.hpp"
#include "ApvlvUtil.hpp"
#include "ApvlvParams.hpp"
#include "ApvlvWindow.hpp"

#include <gtk/gtk.h>

#include <string.h>

namespace apvlv
{
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
      m_son = m_daughter = m_parent = NULL;
    }

  ApvlvWindow::~ApvlvWindow ()
    {
      if (type == AW_DOC)
        {
          if (m_Doc != NULL)
            {
              const char *fdoc = m_Doc->filename ();
              if (fdoc != NULL && m_Doc != gView->hasloaded (fdoc))
                {
                  delete m_Doc;
                }
            }
        }
      else if (type == AW_SP || type == AW_VSP)
        {
          delete m_son;
          delete m_daughter;
          gtk_widget_destroy (m_Paned);
        }
      else
        {
          debug ("type error: %d", type);
        }
    }

  GtkWidget *
    ApvlvWindow::widget ()
      {
        if (type == AW_DOC)
          {
            return m_Doc->widget ();
          }
        else if (type == AW_SP || type == AW_VSP)
          {
            return m_Paned;
          }
        else
          {
            err ("type error: %d", type);
            return NULL;
          }
      }

  void
    ApvlvWindow::setcurrentWindow (ApvlvWindow *pre, ApvlvWindow *win)
      {
        if (pre) asst (pre->type == AW_DOC);
        asst (win->type == AW_DOC);
        if (pre != NULL && pre->type == AW_DOC)
          {
            pre->m_Doc->setactive (false);
          }

        if (win->type == AW_DOC)
          {
            win->m_Doc->setactive (true);
          }

        m_curWindow = win;
      }

  void
    ApvlvWindow::delcurrentWindow ()
      {
        asst (currentWindow ()->istop () == false);
        ApvlvWindow *pwin = currentWindow ()->m_parent;
        ApvlvWindow *win = currentWindow () == pwin->m_son? pwin->m_daughter: pwin->m_son;
        ApvlvWindow *cwin = pwin->unbirth (win);
        setcurrentWindow (NULL, cwin);
      }

  ApvlvWindow *
    ApvlvWindow::currentWindow ()
      {
        return m_curWindow;
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
                setcurrentWindow (this, nwin);
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
        return MATCH;
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
        ApvlvWindow *cw, *w, *nw, *fw;
        bool right = false;

        asst (this && type == AW_DOC);
        for (cw = fw = NULL, w = this; w != NULL; cw = w, w = w->m_parent)
          {
            if (w->type == AW_SP)
              {
                if (cw == w->m_daughter && down == true
                    || cw == w->m_son && down == false)
                  {
                    continue;
                  }
                else
                  {
                    fw = down? w->m_daughter: w->m_son;
                    break;
                  }
              }
            else if (w->type == AW_VSP)
              {
                if (cw != NULL && cw == w->m_daughter)
                  {
                    right = true;
                  }
                else
                  {
                    right = false;
                  }
              }
          }

        for ( nw = w = fw; w != NULL; )
          {
            if (w->type == AW_DOC)
              {
                nw = w;
                break;
              }
            else if (w->type == AW_SP)
              {
                w = down? w->m_son: w->m_daughter;
              }
            else if (w->type == AW_VSP)
              {
                w = right? w->m_daughter: w->m_son;
              }
            else
              {
                debug ("error type: %d", w->type);
                return NULL;
              }
          }

        return nw;
      }

  inline ApvlvWindow *
    ApvlvWindow::gethl (int num, bool right)
      {
        ApvlvWindow *cw, *w, *nw, *fw;
        bool down = false;

        asst (this && type == AW_DOC);
        for (cw = fw = NULL, w = this; w != NULL; cw = w, w = w->m_parent)
          {
            if (w->type == AW_VSP)
              {
                if (cw == w->m_daughter && right == true
                    || cw == w->m_son && right == false)
                  {
                    continue;
                  }
                else
                  {
                    fw = right? w->m_daughter: w->m_son;
                    break;
                  }
              }
            else if (w->type == AW_SP)
              {
                if (cw != NULL && cw == w->m_daughter)
                  {
                    down = true;
                  }
                else
                  {
                    down = false;
                  }
              }
          }

        for ( nw = w = fw; w != NULL; )
          {
            if (w->type == AW_DOC)
              {
                nw = w;
                break;
              }
            else if (w->type == AW_VSP)
              {
                w = right? w->m_son: w->m_daughter;
              }
            else if (w->type == AW_SP)
              {
                w = down? w->m_daughter: w->m_son;
              }
            else
              {
                debug ("error type: %d", w->type);
                return NULL;
              }
          }

        return nw;
      }

  ApvlvWindow *
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

  // birth a new AW_DOC window, and the new window beyond the input doc
  // this made a AW_DOC window to AW_SP|AW_VSP
  ApvlvWindow *
    ApvlvWindow::birth (bool vsp, ApvlvDoc *doc)
      {
        asst (type == AW_DOC);

        if (doc == m_Doc)
          {
            debug ("can't birth with orign doc, copy it");
            doc = NULL;
          }

        if (doc == NULL)
          {
            doc = m_Doc->copy ();
          }

        ApvlvWindow *nwindow = new ApvlvWindow (m_Doc);
        nwindow->m_parent = this;
        m_son = nwindow;

        ApvlvWindow *nwindow2 = new ApvlvWindow (doc);
        nwindow2->m_parent = this;
        m_daughter = nwindow2;

        m_Paned = vsp == false? gtk_vpaned_new (): gtk_hpaned_new ();
        g_signal_connect (G_OBJECT (m_Paned), "button-release-event", G_CALLBACK (apvlv_window_paned_resized_cb), this);

        replace_widget (widget (), m_Paned, WR_REF);

        gtk_paned_pack1 (GTK_PANED (m_Paned), nwindow->widget (), TRUE, TRUE);
        gtk_paned_pack2 (GTK_PANED (m_Paned), nwindow2->widget (), TRUE, TRUE);

        type = vsp == false? AW_SP: AW_VSP;
        if (type == AW_SP)
          {
            nwindow->setsize (m_width, m_height / 2);
            nwindow2->setsize (m_width, m_height / 2);
          }
        else if (type == AW_VSP)
          {
            nwindow->setsize (m_width / 2, m_height);
            nwindow2->setsize (m_width / 2, m_height);
          }

        gtk_widget_show_all (m_Paned);

        setcurrentWindow (nwindow2, nwindow);
        return nwindow;
      }

  ApvlvWindow *
    ApvlvWindow::unbirth (ApvlvWindow *child)
      {
        asst (type == AW_SP || type == AW_VSP);

        ApvlvWindow *owin = child == child->m_parent->m_son?
          child->m_parent->m_daughter:
          child->m_parent->m_son;

        delete owin;

        if (child->type == AW_DOC)
          {
            ApvlvDoc *doc = child->getDoc (true);
            replace_widget (widget (), doc->widget (), WR_REF);
            m_Doc = doc;
          }
        else if (child->type == AW_SP || child->type == AW_VSP)
          {
            replace_widget (widget (), child->widget (), WR_REF_CHILDREN);
            m_Paned = child->widget ();
          }

        gtk_widget_show_all (widget ());
        type = AW_DOC;
        delete child;

        return this;
      }

  bool
    ApvlvWindow::istop ()
      {
        return m_parent == NULL? true: false;
      }

  void
    ApvlvWindow::setsize (int width, int height)
      {
        m_width = width;
        m_height = height;

        if (type == AW_DOC)
          {
            m_Doc->setsize (m_width, m_height);
          }
        else if (type == AW_SP
                 || type == AW_VSP)
          {
            g_timeout_add (50, apvlv_window_resize_children_cb, this);
          }
      }

  ApvlvDoc *
    ApvlvWindow::loadDoc (const char *filename)
      {
        asst (type == AW_DOC);
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
            replace_widget (widget (), ndoc->widget (), WR_REF);
            m_Doc = ndoc;
            gtk_widget_show_all (widget ());
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
        asst (type == AW_DOC);
        replace_widget (widget (), doc->widget (), WR_REF);
        m_Doc = doc;
      }

  ApvlvDoc *
    ApvlvWindow::getDoc (bool remove)
      {
        asst (type == AW_DOC);
        ApvlvDoc *rdoc = m_Doc;

        if (remove)
          {
            remove_widget (widget (), WR_REF);
            m_Doc = NULL;
          }

        return rdoc;
      }

  void
    ApvlvWindow::smaller (int times)
      {
        if (m_parent == NULL) return;

        int val = gtk_paned_get_position (GTK_PANED (m_parent->m_Paned));
        int len = 20 * times;
        m_parent->m_son == this? val -= len: val += len;
        gtk_paned_set_position (GTK_PANED (m_parent->m_Paned), val);

        m_parent->resize_children ();
      }

  void
    ApvlvWindow::bigger (int times)
      {
        if (m_parent == NULL) return;

        int val = gtk_paned_get_position (GTK_PANED (m_parent->m_Paned));
        int len = 20 * times;
        m_parent->m_son == this? val += len: val -= len;
        gtk_paned_set_position (GTK_PANED (m_parent->m_Paned), val);

        m_parent->resize_children ();
      }

  gboolean
    ApvlvWindow::apvlv_window_paned_resized_cb (GtkWidget   *wid,
                                                GdkEventButton *but,
                                                ApvlvWindow *win)
      {
        win->resize_children ();
        return FALSE;
      }

  gboolean
    ApvlvWindow::resize_children ()
      {
        int mw1 = m_width, mw2 = m_width, mh1 = m_height, mh2 = m_height;
        int mi = GTK_PANED (m_Paned)->min_position;
        int ma = GTK_PANED (m_Paned)->max_position;
        int mv = gtk_paned_get_position (GTK_PANED (m_Paned));

        int ms = ma - mi;
        if (ms != 0)
          {
            if (type == AW_SP)
              {
                mh1 = (m_height * (mv - mi)) / ms;
                mh2 = m_height - mh1;
              }
            else if (type == AW_VSP)
              {
                mw1 = (m_width * (mv - mi)) / ms;
                mw2 = m_width - mw1;
              }

            m_son->setsize (mw1, mh1);
            m_daughter->setsize (mw2, mh2);

            return TRUE;
          }
        else
          {
            return FALSE;
          }
      }

  gboolean
    ApvlvWindow::apvlv_window_resize_children_cb (gpointer data)
      {
        ApvlvWindow *win = (ApvlvWindow *) data;
        return win->resize_children () == TRUE? FALSE: FALSE;
      }
}

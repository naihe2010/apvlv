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
          mDoc = new ApvlvDoc (gParams->value ("zoom"));
        }
      else
        {
          mDoc = doc;
        }
      m_son = m_daughter = m_parent = NULL;
    }

  ApvlvWindow::~ApvlvWindow ()
    {
      if (type == AW_DOC)
        {
          if (mDoc != NULL)
            {
              const char *fdoc = mDoc->filename ();
              if (fdoc != NULL && mDoc != gView->hasloaded (fdoc))
                {
                  delete mDoc;
                }
            }
        }
      else if (type == AW_SP || type == AW_VSP)
        {
          delete m_son;
          delete m_daughter;
          gtk_widget_destroy (mPaned);
        }
    }

  GtkWidget *
    ApvlvWindow::widget ()
      {
        if (type == AW_DOC)
          {
            return mDoc->widget ();
          }
        else if (type == AW_SP || type == AW_VSP)
          {
            return mPaned;
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
            pre->mDoc->setactive (false);
          }

        if (win->type == AW_DOC)
          {
            win->mDoc->setactive (true);
          }

        m_curWindow = win;
      }

  void
    ApvlvWindow::delcurrentWindow ()
      {
        asst (currentWindow ()->istop () == false);
        ApvlvWindow *crwin = currentWindow ();
        ApvlvWindow *pwin = crwin->m_parent;
        ApvlvWindow *child = crwin == pwin->m_son? pwin->m_daughter: pwin->m_son;
        ApvlvWindow *cwin = pwin->unbirth (crwin, child);
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
                if ((cw == w->m_daughter && down == true)
                    || (cw == w->m_son && down == false))
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
                if ((cw == w->m_daughter && right == true)
                    || (cw == w->m_son && right == false))
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

        if (doc == mDoc)
          {
            debug ("can't birth with orign doc, copy it");
            doc = NULL;
          }

        if (doc == NULL)
          {
            doc = mDoc->copy ();
          }

        ApvlvWindow *nwindow = new ApvlvWindow (mDoc);
        nwindow->m_parent = this;
        m_son = nwindow;

        ApvlvWindow *nwindow2 = new ApvlvWindow (doc);
        nwindow2->m_parent = this;
        m_daughter = nwindow2;

        mPaned = vsp == false? gtk_vpaned_new (): gtk_hpaned_new ();
        g_signal_connect (G_OBJECT (mPaned), "button-release-event", G_CALLBACK (apvlv_window_paned_resized_cb), this);

        replace_widget (widget (), mPaned, WR_REF);

        gtk_paned_pack1 (GTK_PANED (mPaned), nwindow->widget (), TRUE, TRUE);
        gtk_paned_pack2 (GTK_PANED (mPaned), nwindow2->widget (), TRUE, TRUE);

        type = vsp == false? AW_SP: AW_VSP;
        if (type == AW_SP)
          {
            nwindow->setsize (mWidth, mHeight / 2);
            nwindow2->setsize (mWidth, mHeight / 2);
          }
        else if (type == AW_VSP)
          {
            nwindow->setsize (mWidth / 2, mHeight);
            nwindow2->setsize (mWidth / 2, mHeight);
          }

        gtk_widget_show_all (mPaned);

        setcurrentWindow (nwindow2, nwindow);
        return nwindow;
      }

  // unbirth a child
  // @param 1, be delete
  // @param 2, be unbirth, that is up to the parent
  // return the new child
  ApvlvWindow *
    ApvlvWindow::unbirth (ApvlvWindow *dead, ApvlvWindow *child)
      {
        asst (type == AW_SP || type == AW_VSP);

        if (child->type == AW_DOC)
          {
            ApvlvDoc *doc = child->getDoc (true);
            replace_widget (widget (), doc->widget (), WR_REF);
            mDoc = doc;
            type = AW_DOC;
          }
        else if (child->type == AW_SP || child->type == AW_VSP)
          {
            g_object_ref (G_OBJECT (child->mPaned));
            gtk_container_remove (GTK_CONTAINER (mPaned), child->mPaned);
            replace_widget (mPaned, child->mPaned, WR_REMOVE);
            type = child->type;
            mPaned = child->mPaned;
            m_son = child->m_son;
            m_son->m_parent = this;
            m_daughter = child->m_daughter;
            m_daughter->m_parent = this;
            child->type = AW_NONE;
          }

        gtk_widget_show_all (widget ());

        delete dead;
        delete child;

        ApvlvWindow *win;
        for (win = this; win->type != AW_DOC; win = win->m_son);

        return win;
      }

  bool
    ApvlvWindow::istop ()
      {
        return m_parent == NULL? true: false;
      }

  void
    ApvlvWindow::setsize (int width, int height)
      {
        mWidth = width;
        mHeight = height;

        if (type == AW_DOC)
          {
            mDoc->setsize (mWidth, mHeight);
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
        if (mDoc->filename () == NULL || gView->hasloaded (mDoc->filename ()) != mDoc)
          {
            mDoc->setsize (mWidth, mHeight);
            bool ret = mDoc->loadfile (filename);
            return ret? mDoc: NULL;
          }

        bool bcache = false;
        const char *scache = gParams->value ("cache");
        if (strcmp (scache, "yes") == 0)
          {
            bcache = true;
          }
        ApvlvDoc *ndoc = new ApvlvDoc (gParams->value ("zoom"), bcache);
        ndoc->setsize (mWidth, mHeight);
        bool ret = ndoc->loadfile (filename);
        if (ret)
          {
            replace_widget (widget (), ndoc->widget (), WR_REF);
            mDoc = ndoc;
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
        mDoc = doc;
      }

  ApvlvDoc *
    ApvlvWindow::getDoc (bool remove)
      {
        asst (type == AW_DOC);
        ApvlvDoc *rdoc = mDoc;

        if (remove)
          {
            remove_widget (widget (), WR_REF);
            mDoc = NULL;
          }

        return rdoc;
      }

  void
    ApvlvWindow::smaller (int times)
      {
        if (m_parent == NULL) return;

        int val = gtk_paned_get_position (GTK_PANED (m_parent->mPaned));
        int len = 20 * times;
        m_parent->m_son == this? val -= len: val += len;
        gtk_paned_set_position (GTK_PANED (m_parent->mPaned), val);

        m_parent->resize_children ();
      }

  void
    ApvlvWindow::bigger (int times)
      {
        if (m_parent == NULL) return;

        int val = gtk_paned_get_position (GTK_PANED (m_parent->mPaned));
        int len = 20 * times;
        m_parent->m_son == this? val += len: val -= len;
        gtk_paned_set_position (GTK_PANED (m_parent->mPaned), val);

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
        int mw1 = mWidth, mw2 = mWidth, mh1 = mHeight, mh2 = mHeight;
        int mi = GTK_PANED (mPaned)->min_position;
        int ma = GTK_PANED (mPaned)->max_position;
        int mv = gtk_paned_get_position (GTK_PANED (mPaned));

        int ms = ma - mi;
        if (ms != 0)
          {
            if (type == AW_SP)
              {
                mh1 = (mHeight * (mv - mi)) / ms;
                mh2 = mHeight - mh1;
              }
            else if (type == AW_VSP)
              {
                mw1 = (mWidth * (mv - mi)) / ms;
                mw2 = mWidth - mw1;
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

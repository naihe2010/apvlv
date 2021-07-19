/*
 * This file is part of the apvlv package
 *
 * Copyright (C) 2008 Alf.
 *
 * Contact: Alf <naihe2010@126.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2.0 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */
/* @CPPFILE ApvlvWindow.cc
 *
 *  Author: Alf <naihe2010@126.com>
 */
/* @date Created: 2008/09/30 00:00:00 Alf */

#include "ApvlvView.h"
#include "ApvlvUtil.h"
#include "ApvlvParams.h"
#include "ApvlvWindow.h"

#include <gtk/gtk.h>

namespace apvlv
{
    ApvlvWindow::ApvlvWindow (ApvlvCore *doc, ApvlvView *view)
    {
      mIsClose = false;

      mPaned = nullptr;

      mActiveWindow = this;

      mType = AW_CORE;
      if (doc == nullptr)
        {
          mCore = new ApvlvDoc (view, DISPLAY_TYPE_IMAGE, gParams->values ("zoom"));
        }
      else
        {
          mCore = doc;
        }
      m_son = m_daughter = m_parent = nullptr;

      mView = view;
    }

    ApvlvWindow::~ApvlvWindow ()
    {
      if (mIsClose)
        {
          return;
        }

      debug ("delete window: %p", this);

      mIsClose = true;

      if (m_parent != nullptr)
        {
          if (m_parent->m_son == this)
            {
              m_parent->m_son = nullptr;
            }
          else if (m_parent->m_daughter == this)
            {
              m_parent->m_daughter = nullptr;
            }
        }

      if (mType == AW_CORE)
        {
          mCore->inuse (false);
        }

      else if (mType == AW_SP || mType == AW_VSP)
        {
          if (m_son != nullptr)
            {
              delete m_son;
              m_son = nullptr;
            }
          if (m_daughter != nullptr)
            {
              delete m_daughter;
              m_daughter = nullptr;
            }

          g_object_unref (mPaned);
        }
    }

    GtkWidget *ApvlvWindow::widget ()
    {
      if (mType == AW_CORE)
        {
          return mCore->widget ();
        }
      else if (mType == AW_SP || mType == AW_VSP)
        {
          return mPaned;
        }
      else
        {
          debug ("type error: %d", mType);
          return nullptr;
        }
    }

    void ApvlvWindow::setcurrentWindow (ApvlvWindow *pre, ApvlvWindow *win)
    {
      if (pre != nullptr && pre->mType == ApvlvWindow::AW_CORE)
        {
          pre->getCore ()->setactive (false);
        }

      if (win->mType == ApvlvWindow::AW_CORE)
        {
          win->getCore ()->setactive (true);
        }

      ApvlvWindow *parent = win;
      ApvlvWindow *grand = parent->m_parent;
      while (grand != nullptr)
        {
          parent = grand;
          grand = parent->m_parent;
        }
      parent->mActiveWindow = win;
    }

    void ApvlvWindow::delcurrentWindow ()
    {
      ApvlvWindow *pwin = mActiveWindow->m_parent;
      ApvlvWindow *child =
          mActiveWindow == pwin->m_son ? pwin->m_daughter : pwin->m_son;
      ApvlvWindow *cwin = pwin->unbirth (mActiveWindow, child);
      setcurrentWindow (nullptr, cwin);
    }

    returnType ApvlvWindow::process (int ct, guint key)
    {
      ApvlvWindow *nwin;
      debug ("input [%d]", key);

      switch (key)
        {
          case CTRL ('w'):
          case 'k':
          case 'j':
          case 'h':
          case 'l':
            nwin = getneighbor (ct, key);
          if (nwin != nullptr)
            {
              ApvlvWindow::setcurrentWindow (this, nwin);
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

    ApvlvWindow *ApvlvWindow::getneighbor (int ct, guint key)
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

      return nullptr;
    }

    inline ApvlvWindow *ApvlvWindow::getkj (__attribute__((unused)) int num, bool down)
    {
      ApvlvWindow *cw, *w, *nw, *fw;
      bool right = false;

      asst (mType == AW_CORE);
      for (cw = fw = nullptr, w = this; w != nullptr; cw = w, w = w->m_parent)
        {
          if (w->mType == AW_SP)
            {
              if ((cw == w->m_daughter && down)
                  || (cw == w->m_son && !down))
                {
                  continue;
                }
              else
                {
                  fw = down ? w->m_daughter : w->m_son;
                  break;
                }
            }
          else if (w->mType == AW_VSP)
            {
              if (cw != nullptr && cw == w->m_daughter)
                {
                  right = true;
                }
              else
                {
                  right = false;
                }
            }
        }

      for (nw = w = fw; w != nullptr;)
        {
          if (w->mType == AW_CORE)
            {
              nw = w;
              break;
            }
          else if (w->mType == AW_SP)
            {
              w = down ? w->m_son : w->m_daughter;
            }
          else if (w->mType == AW_VSP)
            {
              w = right ? w->m_daughter : w->m_son;
            }
          else
            {
              debug ("error type: %d", w->mType);
              return nullptr;
            }
        }

      return nw;
    }

    inline ApvlvWindow *ApvlvWindow::gethl (__attribute__((unused)) int num, bool right)
    {
      ApvlvWindow *cw, *w, *nw, *fw;
      bool down = false;

      if (mCore->toggledControlContent (right))
        {
          return this;
        }

      asst (mType == AW_CORE);
      for (cw = fw = nullptr, w = this; w != nullptr; cw = w, w = w->m_parent)
        {
          if (w->mType == AW_VSP)
            {
              if ((cw == w->m_daughter && right)
                  || (cw == w->m_son && !right))
                {
                  continue;
                }
              else
                {
                  fw = right ? w->m_daughter : w->m_son;
                  break;
                }
            }
          else if (w->mType == AW_SP)
            {
              if (cw != nullptr && cw == w->m_daughter)
                {
                  down = true;
                }
              else
                {
                  down = false;
                }
            }
        }

      for (nw = w = fw; w != nullptr;)
        {
          if (w->mType == AW_CORE)
            {
              nw = w;
              break;
            }
          else if (w->mType == AW_VSP)
            {
              w = right ? w->m_son : w->m_daughter;
            }
          else if (w->mType == AW_SP)
            {
              w = down ? w->m_daughter : w->m_son;
            }
          else
            {
              debug ("error type: %d", w->mType);
              return nullptr;
            }
        }

      return nw;
    }

    ApvlvWindow *ApvlvWindow::getnext (int num)
    {
      ApvlvWindow *n = getkj (num, true);
      if (n == nullptr)
        {
          n = gethl (num, true);
          if (n == nullptr)
            {
              n = gethl (num, false);
              if (n == nullptr)
                n = getkj (num, false);
            }
        }
      return n;
    }

    // birth a new AW_CORE window, and the new window beyond the input doc
    // this made a AW_CORE window to AW_SP|AW_VSP
    ApvlvWindow *ApvlvWindow::birth (bool vsp, ApvlvCore *doc)
    {
      asst (mType == AW_CORE);

      if (doc == mCore)
        {
          debug ("can't birth with orign doc, copy it");
          doc = nullptr;
        }

      if (doc == nullptr)
        {
          doc = mCore->copy ();
          mCore->mView->regloaded (doc);
        }

      if (doc == nullptr)
        {
          mCore->mView->errormessage ("can't split");
          return this;
        }

      auto *nwindow = new ApvlvWindow (doc, mView);
      nwindow->m_parent = this;
      m_son = nwindow;

      auto *nwindow2 = new ApvlvWindow (mCore, mView);
      nwindow2->m_parent = this;
      m_daughter = nwindow2;

#if GTK_CHECK_VERSION (3, 0, 0)
      mPaned = gtk_paned_new (!vsp ? GTK_ORIENTATION_VERTICAL : GTK_ORIENTATION_HORIZONTAL);
#else
      mPaned = vsp == false ? gtk_vpaned_new () : gtk_hpaned_new ();
#endif
      g_object_ref (mPaned);

      if (m_parent)
        {
          void (*panedcb) (GtkPaned *, GtkWidget *, gboolean, gboolean);
          GtkWidget *parent = m_parent->mPaned;
          if (gtk_paned_get_child1 (GTK_PANED (parent)) == widget ())
            {
              panedcb = gtk_paned_pack1;
            }
          else
            {
              panedcb = gtk_paned_pack2;
            }

          gtk_container_remove (GTK_CONTAINER (parent), widget ());
          panedcb (GTK_PANED (parent), mPaned, TRUE, TRUE);
        }
      else
        {
          replace_widget (widget (), mPaned);
        }

      mType = vsp ? AW_VSP : AW_SP;

      gtk_paned_pack1 (GTK_PANED (mPaned), nwindow->widget (), TRUE, TRUE);
      gtk_paned_pack2 (GTK_PANED (mPaned), nwindow2->widget (), TRUE, TRUE);

      gtk_widget_show_all (mPaned);

      setcurrentWindow (nwindow2, nwindow);

      return nwindow;
    }

    // unbirth a child
    // @param 1, be delete
    // @param 2, be unbirth, that is up to the parent
    // return the new child
    ApvlvWindow *ApvlvWindow::unbirth (ApvlvWindow *dead, ApvlvWindow *child)
    {
      asst (mType == AW_SP || mType == AW_VSP);

      if (m_parent)
        {
          void (*panedcb) (GtkPaned *, GtkWidget *);
          GtkWidget *parent = m_parent->mPaned;
          if (gtk_paned_get_child1 (GTK_PANED (parent)) == mPaned)
            {
              panedcb = gtk_paned_add1;
            }
          else
            {
              panedcb = gtk_paned_add2;
            }

          gtk_container_remove (GTK_CONTAINER (mPaned), child->widget ());
          gtk_container_remove (GTK_CONTAINER (parent), mPaned);
          panedcb (GTK_PANED (parent), child->widget ());
        }
      else
        {
          gtk_container_remove (GTK_CONTAINER (mPaned), child->widget ());
          replace_widget (mPaned, child->widget ());
        }

      if (child->mType == AW_CORE)
        {
          ApvlvCore *doc = child->getCore ();
          mType = AW_CORE;
          mCore = doc;
        }
      else if (child->mType == AW_SP || child->mType == AW_VSP)
        {
          mType = child->mType;
          mPaned = child->mPaned;
          m_son = child->m_son;
          m_son->m_parent = this;
          m_daughter = child->m_daughter;
          m_daughter->m_parent = this;
          child->mType = AW_NONE;
        }

      gtk_widget_show_all (widget ());

      delete dead;
      delete child;

      ApvlvWindow *win;
      for (win = this; win->mType != AW_CORE; win = win->m_son);

      return win;
    }

    bool ApvlvWindow::istop () const
    {
      return m_parent == nullptr;
    }

    void ApvlvWindow::setCore (ApvlvCore *doc)
    {
      debug ("widget (): %p, doc->widget (): %p", widget (), doc->widget ());
      if (mType == AW_CORE)
        {
          mCore->inuse (false);
        }
      replace_widget (widget (), doc->widget ());
      doc->inuse (true);
      mType = AW_CORE;
      mCore = doc;
    }

    ApvlvCore *ApvlvWindow::getCore ()
    {
      return mCore;
    }

    ApvlvWindow *ApvlvWindow::activeWindow ()
    {
      return mActiveWindow;
    }

    void ApvlvWindow::smaller (int times)
    {
      if (m_parent == nullptr)
        return;

      int val = gtk_paned_get_position (GTK_PANED (m_parent->mPaned));
      int len = 20 * times;
      m_parent->m_son == this ? val -= len : val += len;
      gtk_paned_set_position (GTK_PANED (m_parent->mPaned), val);
    }

    void ApvlvWindow::bigger (int times)
    {
      if (m_parent == nullptr)
        return;

      int val = gtk_paned_get_position (GTK_PANED (m_parent->mPaned));
      int len = 20 * times;
      m_parent->m_son == this ? val += len : val -= len;
      gtk_paned_set_position (GTK_PANED (m_parent->mPaned), val);
    }
}

// Local Variables:
// mode: c++
// End:

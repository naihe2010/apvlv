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

#include "ApvlvWindow.h"
#include "ApvlvParams.h"
#include "ApvlvUtil.h"
#include "ApvlvView.h"

#include <gtk/gtk.h>

namespace apvlv
{
ApvlvWindow::ApvlvWindow (ApvlvCore *doc, ApvlvView *view)
{
  mPaned = nullptr;

  mActiveWindow = this;

  mType = AW_CORE;
  if (doc == nullptr)
    {
      mCore = new ApvlvDoc (view, gParams->values ("zoom"));
    }
  else
    {
      mCore = doc;
    }
  m_child_1 = m_child_2 = m_parent = nullptr;

  mView = view;
}

ApvlvWindow::~ApvlvWindow ()
{
  if (mType == AW_CORE)
    {
      debug ("release doc: %p\n", mCore);
      mCore->inuse (false);
    }
  else
    {
      g_object_unref (mPaned);
    }
}

GtkWidget *
ApvlvWindow::widget ()
{
  if (mType == AW_CORE)
    {
      return mCore->widget ();
    }
  else
    {
      return mPaned;
    }
}

void
ApvlvWindow::setcurrentWindow (ApvlvWindow *win)
{
  if (mType == AW_CORE)
    {
      getCore ()->setactive (false);
    }

  if (win->mType == AW_CORE)
    {
      win->getCore ()->setactive (true);
    }

  getAncestor ()->mActiveWindow = win;
}

returnType
ApvlvWindow::process (int ct, guint key)
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
          getCore ()->setactive (false);
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

  return nullptr;
}

inline ApvlvWindow *
ApvlvWindow::getkj (__attribute__ ((unused)) int num, bool down)
{
  ApvlvWindow *cw, *w, *nw, *fw;
  bool right = false;

  g_return_val_if_fail (mType == AW_CORE, nullptr);

  for (cw = fw = nullptr, w = this; w != nullptr; cw = w, w = w->m_parent)
    {
      if (w->mType == AW_SP)
        {
          if ((cw == w->m_child_2 && down) || (cw == w->m_child_1 && !down))
            {
              continue;
            }
          else
            {
              fw = down ? w->m_child_2 : w->m_child_1;
              break;
            }
        }
      else if (w->mType == AW_VSP)
        {
          if (cw != nullptr && cw == w->m_child_2)
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
          w = down ? w->m_child_1 : w->m_child_2;
        }
      else if (w->mType == AW_VSP)
        {
          w = right ? w->m_child_2 : w->m_child_1;
        }
      else
        {
          debug ("error type: %d", w->mType);
          return nullptr;
        }
    }

  return nw;
}

inline ApvlvWindow *
ApvlvWindow::gethl (__attribute__ ((unused)) int num, bool right)
{
  ApvlvWindow *cw, *w, *nw, *fw;
  bool down = false;

  g_return_val_if_fail (mType == AW_CORE, nullptr);

  if (mCore->toggledControlContent (right))
    {
      return this;
    }

  for (cw = fw = nullptr, w = this; w != nullptr; cw = w, w = w->m_parent)
    {
      if (w->mType == AW_VSP)
        {
          if ((cw == w->m_child_2 && right) || (cw == w->m_child_1 && !right))
            {
              continue;
            }
          else
            {
              fw = right ? w->m_child_2 : w->m_child_1;
              break;
            }
        }
      else if (w->mType == AW_SP)
        {
          if (cw != nullptr && cw == w->m_child_2)
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
          w = right ? w->m_child_1 : w->m_child_2;
        }
      else if (w->mType == AW_SP)
        {
          w = down ? w->m_child_2 : w->m_child_1;
        }
      else
        {
          debug ("error type: %d", w->mType);
          return nullptr;
        }
    }

  return nw;
}

ApvlvWindow *
ApvlvWindow::getnext (int num)
{
  g_return_val_if_fail (mType == AW_CORE, nullptr);

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
ApvlvWindow *
ApvlvWindow::birth (WindowType type, ApvlvCore *doc)
{
  g_return_val_if_fail (mType == AW_CORE, nullptr);

  if (doc == mCore)
    {
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
      return nullptr;
    }

  auto current_doc = mCore;
  debug ("steal current doc: %p\n", current_doc);
  auto orig_parent = gtk_widget_get_parent (current_doc->widget ());
  gtk_container_remove (GTK_CONTAINER (orig_parent), current_doc->widget ());

  m_child_1 = new ApvlvWindow (doc, mView);
  m_child_1->m_parent = this;

  m_child_2 = new ApvlvWindow (mCore, mView);
  m_child_2->m_parent = this;

  debug ("%p birth -> %p doc:%p <-> %p doc:%p\n", this, m_child_1, doc,
         m_child_2, mCore);

  mPaned = gtk_paned_new (type == AW_SP ? GTK_ORIENTATION_VERTICAL
                                        : GTK_ORIENTATION_HORIZONTAL);
  g_object_ref (mPaned);

  if (m_parent == nullptr)
    {
      gtk_box_pack_start (GTK_BOX (orig_parent), mPaned, TRUE, TRUE, 0);
    }
  else
    {
      if (m_parent->m_child_1 == this)
        {
          gtk_paned_pack1 (GTK_PANED (orig_parent), mPaned, TRUE, TRUE);
        }
      else
        {
          gtk_paned_pack2 (GTK_PANED (orig_parent), mPaned, TRUE, TRUE);
        }
    }

  gtk_paned_pack1 (GTK_PANED (mPaned), m_child_1->widget (), TRUE, TRUE);
  gtk_paned_pack2 (GTK_PANED (mPaned), m_child_2->widget (), TRUE, TRUE);

  setcurrentWindow (m_child_1);

  gtk_widget_show_all (mPaned);

  mType = type;

  return m_child_1;
}

ApvlvWindow *
ApvlvWindow::unbirth ()
{
  g_return_val_if_fail (mType == AW_CORE, nullptr);
  g_return_val_if_fail (m_parent != nullptr, nullptr);

  auto other = m_parent->m_child_1;
  if (other == this)
    other = m_parent->m_child_2;

  other->m_parent = m_parent->m_parent;

  gtk_container_remove (GTK_CONTAINER (m_parent->mPaned), other->widget ());

  auto orig_parent = gtk_widget_get_parent (m_parent->mPaned);
  gtk_container_remove (GTK_CONTAINER (orig_parent), m_parent->mPaned);

  if (m_parent->m_parent == nullptr)
    {
      gtk_box_pack_start (GTK_BOX (orig_parent), other->widget (), TRUE, TRUE,
                          0);
    }
  else
    {
      if (m_parent->m_parent->m_child_1 == m_parent)
        {
          gtk_paned_pack1 (GTK_PANED (orig_parent), other->widget (), TRUE,
                           TRUE);
          m_parent->m_parent->m_child_1 = other;
        }
      else
        {
          gtk_paned_pack2 (GTK_PANED (orig_parent), other->widget (), TRUE,
                           TRUE);
          m_parent->m_parent->m_child_2 = other;
        }
    }

  auto win = other;
  while (win->mType != AW_CORE)
    win = win->m_child_1;
  setcurrentWindow (win);
  debug ("now current core window: %p\n", win);

  delete m_parent;
  delete this;
  debug ("%p unbirth %p -> %p\n", this, m_parent, other);

  win = other->getAncestor ();
  gtk_widget_show_all (win->widget ());
  return win;
}

void
ApvlvWindow::setCore (ApvlvCore *doc)
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

ApvlvCore *
ApvlvWindow::getCore ()
{
  return mCore;
}

ApvlvWindow *
ApvlvWindow::activeCoreWindow ()
{
  return mActiveWindow;
}

void
ApvlvWindow::smaller (int times)
{
  if (m_parent == nullptr)
    return;

  int val = gtk_paned_get_position (GTK_PANED (m_parent->mPaned));
  int len = 20 * times;
  m_parent->m_child_1 == this ? val -= len : val += len;
  gtk_paned_set_position (GTK_PANED (m_parent->mPaned), val);
}

void
ApvlvWindow::bigger (int times)
{
  if (m_parent == nullptr)
    return;

  int val = gtk_paned_get_position (GTK_PANED (m_parent->mPaned));
  int len = 20 * times;
  m_parent->m_child_1 == this ? val += len : val -= len;
  gtk_paned_set_position (GTK_PANED (m_parent->mPaned), val);
}

ApvlvWindow *
ApvlvWindow::getAncestor ()
{
  ApvlvWindow *win = this;
  while (win->m_parent != nullptr)
    win = win->m_parent;
  return win;
}

bool
ApvlvWindow::isAncestor () const
{
  return m_parent == nullptr;
}
}

// Local Variables:
// mode: c++
// End:

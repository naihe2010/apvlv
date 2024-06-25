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

#include <QSplitter>
#include <stack>

#include "ApvlvParams.h"
#include "ApvlvUtil.h"
#include "ApvlvView.h"
#include "ApvlvWindow.h"

namespace apvlv
{
ApvlvWindowContext::ApvlvWindowContext (ApvlvView *view, ApvlvWindow *root,
                                        ApvlvWindow *active, int count)
{
  mView = view;
  mRootWindow = root;
  mActiveWindow = active;
  mWindowCount = count;
}

void
ApvlvWindowContext::registerWindow (ApvlvWindow *win)
{
  qDebug ("context: %p register win: %p", this, win);
  if (mRootWindow == nullptr)
    {
      mRootWindow = win;
      addWidget (win->widget ());
    }
  setActiveWindow (win);
  mWindowCount++;
}

void
ApvlvWindowContext::unregisterWindow (ApvlvWindow *win)
{
  qDebug ("context: %p unregister win: %p", this, win);
  mWindowCount--;
  if (mActiveWindow == win)
    {
      auto awin = mRootWindow;
      while (awin && awin->mType != ApvlvWindow::AW_CORE)
        {
          awin = awin->m_child_1;
        }
      if (awin)
        setActiveWindow (awin);
    }
  if (mRootWindow == win)
    {
      mRootWindow->widget ()->setParent (nullptr);
      mRootWindow = nullptr;
    }
}

ApvlvWindow::ApvlvWindow (ApvlvWindowContext *context, ApvlvCore *core)
{
  mPaned = nullptr;

  mType = AW_CORE;
  if (core == nullptr)
    {
      auto ndoc = new ApvlvDoc (context->getView (), gParams->values ("zoom"));
      mCore = ndoc;
    }
  else
    {
      mCore = core;
    }
  m_child_1 = m_child_2 = m_parent = nullptr;

  mContext = context;
  mContext->registerWindow (this);
}

ApvlvWindow::~ApvlvWindow ()
{
  mContext->unregisterWindow (this);
  mContext = nullptr;

  if (mType == AW_CORE)
    {
      qDebug ("release doc: %p", mCore);
      if (mCore)
        mCore->inuse (false);
    }
}

QWidget *
ApvlvWindow::widget ()
{
  if (mType == AW_CORE)
    {
      return mCore;
    }
  else
    {
      return mPaned;
    }
}

ReturnType
ApvlvWindow::process (int ct, uint key)
{
  ApvlvWindow *nwin;
  qDebug ("input [%d]", key);

  switch (key)
    {
    case CTRL ('w'):
    case 'k':
    case 'j':
    case 'h':
    case 'l':
      nwin = getNeighbor (ct, key);
      if (nwin != nullptr)
        {
          nwin->setActive (true);
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
ApvlvWindowContext::findWindowByWidget (QWidget *widget)
{
  auto doc = ApvlvCore::findByWidget (widget);
  if (doc == nullptr)
    return nullptr;

  stack<ApvlvWindow *> winstack;
  winstack.push (mRootWindow);
  while (!winstack.empty ())
    {
      auto win = winstack.top ();
      winstack.pop ();
      if (win->mType == ApvlvWindow::AW_CORE)
        {
          if (win->widget () == doc)
            return win;
        }
      else
        {
          winstack.push (win->m_child_1);
          winstack.push (win->m_child_2);
        }
    }

  return nullptr;
}

ApvlvWindow *
ApvlvWindow::getNeighbor (int count, uint key)
{
  ApvlvWindow *last_win = nullptr;
  ApvlvWindow *win = this;

  for (int c = 0; c < count; ++c)
    {
      switch (key)
        {
        case CTRL ('w'):
          win = win->getNext ();
          break;
        case 'k':
          win = win->getTop ();
          break;
        case 'j':
          win = win->getBottom ();
          break;
        case 'h':
          win = win->getLeft ();
          break;
        case 'l':
          win = win->getRight ();
          break;
        default:
          break;
        }

      if (win != nullptr)
        last_win = win;
      else
        break;
    }

  return last_win;
}

ApvlvWindow *
ApvlvWindow::getLeft ()
{
  if (mCore->toggledControlContent (false))
    {
      return this;
    }

  ApvlvWindow *twin = nullptr;
  ApvlvWindow *fwin = this;

  while (fwin->m_parent)
    {
      if (fwin->m_parent->mType == AW_SP && fwin->m_parent->m_child_2 == fwin)
        {
          twin = fwin->m_parent->m_child_1;
          break;
        }
      fwin = fwin->m_parent;
    }
  if (twin == nullptr)
    return nullptr;

  while (twin->mType != AW_CORE)
    {
      twin = twin->m_child_2;
    }
  return twin;
}

ApvlvWindow *
ApvlvWindow::getRight ()
{
  if (mCore->toggledControlContent (true))
    {
      return this;
    }

  ApvlvWindow *twin = nullptr;
  ApvlvWindow *fwin = this;

  while (fwin->m_parent)
    {
      if (fwin->m_parent->mType == AW_SP && fwin->m_parent->m_child_1 == fwin)
        {
          twin = fwin->m_parent->m_child_2;
          break;
        }
      fwin = fwin->m_parent;
    }
  if (twin == nullptr)
    return nullptr;

  while (twin->mType != AW_CORE)
    {
      twin = twin->m_child_1;
    }
  return twin;
}

ApvlvWindow *
ApvlvWindow::getTop ()
{
  ApvlvWindow *twin = nullptr;
  ApvlvWindow *fwin = this;

  while (fwin->m_parent)
    {
      if (fwin->m_parent->mType == AW_VSP && fwin->m_parent->m_child_2 == fwin)
        {
          twin = fwin->m_parent->m_child_1;
          break;
        }
      fwin = fwin->m_parent;
    }
  if (twin == nullptr)
    return nullptr;

  while (twin->mType != AW_CORE)
    {
      twin = twin->m_child_2;
    }
  return twin;
}

ApvlvWindow *
ApvlvWindow::getBottom ()
{
  ApvlvWindow *twin = nullptr;
  ApvlvWindow *fwin = this;

  while (fwin->m_parent)
    {
      if (fwin->m_parent->mType == AW_VSP && fwin->m_parent->m_child_1 == fwin)
        {
          twin = fwin->m_parent->m_child_2;
          break;
        }
      fwin = fwin->m_parent;
    }
  if (twin == nullptr)
    return nullptr;

  while (twin->mType != AW_CORE)
    {
      twin = twin->m_child_1;
    }
  return twin;
}

void
ApvlvWindow::splitWidget (WindowType type, QWidget *one, QWidget *other)
{
  mType = type;

  mPaned = new QSplitter ();
  mPaned->setOrientation (type == AW_SP ? Qt::Horizontal : Qt::Vertical);
  mPaned->setHandleWidth (10);
  mPaned->setStretchFactor (0, 1);
  mPaned->setStretchFactor (1, 1);

  auto parent_widget = mCore->parentWidget ();
  Q_ASSERT (parent_widget->inherits ("QSplitter"));
  auto spliter = dynamic_cast<QSplitter *> (parent_widget);
  auto index = spliter->indexOf (mCore);
  spliter->insertWidget (index, mPaned);
  mCore->setParent (nullptr);

  mPaned->addWidget (one);
  mPaned->addWidget (other);
}

ApvlvWindow *
ApvlvWindow::getNext ()
{
  auto *n = getRight ();
  if (n)
    return n;

  n = getBottom ();
  if (n)
    return n;

  n = getLeft ();
  if (n)
    return n;

  n = getTop ();
  return n;
}

// birth a new AW_CORE window, and the new window beyond the input doc
// this made a AW_CORE window to AW_SP|AW_VSP
bool
ApvlvWindow::birth (WindowType type, ApvlvCore *doc)
{
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
      return false;
    }

  splitWidget (type, mCore, doc);

  m_child_1 = new ApvlvWindow (mContext, mCore);
  m_child_1->m_parent = this;

  m_child_2 = new ApvlvWindow (mContext, doc);
  m_child_2->m_parent = this;

  qDebug ("%p birth -> %p doc:%p <-> %p doc:%p", this, m_child_1, mCore,
          m_child_2, doc);
  mCore = nullptr;

  return true;
}

void
ApvlvWindow::unbirth ()
{
  if (mType != AW_CORE)
    return;
  if (m_parent == nullptr)
    return;

  auto parent_widget = m_parent->mPaned;
  auto parent_parent_widget = parent_widget->parentWidget ();
  auto other_window = m_parent->m_child_1 == this ? m_parent->m_child_2
                                                  : m_parent->m_child_1;
  auto other = other_window->widget ();

  Q_ASSERT (parent_parent_widget->inherits ("QSplitter"));
  auto splitter = dynamic_cast<QSplitter *> (parent_parent_widget);
  auto index = splitter->indexOf (parent_widget);
  other->setParent (nullptr);
  splitter->insertWidget (index, other);
  parent_widget->setParent (nullptr);

  m_parent->mType = other_window->mType;
  m_parent->mCore = other_window->mCore;
  m_parent->mPaned = other_window->mPaned;
  m_parent->m_child_1 = other_window->m_child_1;
  m_parent->m_child_2 = other_window->m_child_2;

  auto awin = m_parent;
  while (awin->mType != AW_CORE)
    {
      awin = awin->m_child_1;
    }
  mContext->setActiveWindow (awin);

  other_window->mCore = nullptr;
  other_window->mPaned = nullptr;
  other_window->m_parent = nullptr;
  other_window->m_child_1 = nullptr;
  other_window->m_child_2 = nullptr;
  delete other_window;

  m_parent = nullptr;
  deleteLater ();

  qDebug ("%p unbirth %p -> %p", this, other, m_parent);
}

void
ApvlvWindow::setActive (bool act)
{
  if (mType != AW_CORE)
    return;

  if (act)
    mContext->setActiveWindow (this);

  mCore->setActive (act);
}

void
ApvlvWindow::setCore (ApvlvCore *doc)
{
  qDebug ("widget (): %p, doc->widget (): %p", widget (), doc);
  if (mType == AW_CORE)
    {
      mCore->inuse (false);
    }
  auto parent = widget ()->parentWidget ();
  qDebug ("window parent: %s", parent->metaObject ()->className ());
  Q_ASSERT (parent->inherits ("QSplitter"));
  auto splitter = dynamic_cast<QSplitter *> (parent);
  splitter->addWidget (doc);
  mCore->setParent (nullptr);
  doc->inuse (true);
  mType = AW_CORE;
  mCore = doc;
}

ApvlvCore *
ApvlvWindow::getCore ()
{
  return mCore;
}

bool
ApvlvWindow::isRoot ()
{
  return mContext->getRootWindow () == this;
}

void
ApvlvWindow::smaller (int times)
{
  if (m_parent == nullptr)
    return;

  int val = m_parent->mPaned->width ();
  int len = 20 * times;
  m_parent->m_child_1 == this ? val -= len : val += len;
  m_parent->mPaned->setFixedWidth (val);
}

void
ApvlvWindow::bigger (int times)
{
  if (m_parent == nullptr)
    return;

  int val
      = 0; // need impl gtk_paned_get_position (GTK_PANED (m_parent->mPaned));
  int len = 20 * times;
  m_parent->m_child_1 == this ? val += len : val -= len;
  // gtk_paned_set_position (GTK_PANED (m_parent->mPaned), val);
}

}

// Local Variables:
// mode: c++
// End:

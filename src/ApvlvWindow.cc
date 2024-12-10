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

#include <QSplitter>
#include <QStackedWidget>
#include <stack>

#include "ApvlvParams.h"
#include "ApvlvView.h"
#include "ApvlvWindow.h"

namespace apvlv
{
ApvlvWindow::ApvlvWindow ()
{
  qDebug () << "win: " << this << ", init";
  setLayout (&mLayout);
  mLayout.addWidget (&mPaned);
}

ApvlvWindow::~ApvlvWindow ()
{
  qDebug () << "win: " << this << ", released";
  if (mType == WindowType::FRAME)
    {
      auto frame = stealFrame ();
      if (frame)
        frame->inuse (false);
    }
}

CmdReturn
ApvlvWindow::process (int ct, uint key)
{
  ApvlvWindow *nwin;
  qDebug () << "input [" << key << "]";

  switch (key)
    {
    case ctrlValue ('w'):
    case 'k':
    case 'j':
    case 'h':
    case 'l':
      nwin = getNeighbor (ct, key);
      if (nwin != nullptr && nwin != this)
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
  return CmdReturn::MATCH;
}

ApvlvWindow *
ApvlvWindow::findWindowByWidget (QWidget *widget)
{
  auto doc = ApvlvFrame::findByWidget (widget);
  if (doc == nullptr)
    return nullptr;

  std::stack<ApvlvWindow *> winstack;
  winstack.push (this);
  while (!winstack.empty ())
    {
      auto win = winstack.top ();
      winstack.pop ();
      if (win->mType == ApvlvWindow::WindowType::FRAME)
        {
          if (win->getFrame () == doc)
            return win;
        }
      else
        {
          winstack.push (win->firstWindow ());
          winstack.push (win->secondWindow ());
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
        case ctrlValue ('w'):
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
  if (mType == WindowType::FRAME && getFrame ()->toggledControlDirectory (false))
    {
      return this;
    }

  ApvlvWindow *fwin = this;

  while (fwin->parentWindow ())
    {
      fwin = fwin->parentWindow ();
      if (fwin->mType == WindowType::SP && fwin->firstWindow () != this)
        {
          if (fwin->secondWindow () == this)
            return fwin->firstWindow ();

          while (fwin->mType != WindowType::FRAME)
            {
              fwin = fwin->secondWindow ();
            }
          return fwin;
        }
    }

  return nullptr;
}

ApvlvWindow *
ApvlvWindow::getRight ()
{
  if (mType == WindowType::FRAME && getFrame ()->toggledControlDirectory (true))
    {
      return this;
    }

  ApvlvWindow *fwin = this;

  while (fwin->parentWindow ())
    {
      fwin = fwin->parentWindow ();
      if (fwin->mType == WindowType::SP && fwin->secondWindow () != this)
        {
          if (fwin->firstWindow () == this)
            return fwin->secondWindow ();

          while (fwin->mType != WindowType::FRAME)
            {
              fwin = fwin->firstWindow ();
            }
          return fwin;
        }
    }

  return nullptr;
}

ApvlvWindow *
ApvlvWindow::getTop ()
{
  ApvlvWindow *fwin = this;

  while (fwin->parentWindow ())
    {
      fwin = fwin->parentWindow ();
      if (fwin->mType == WindowType::VSP && (fwin->firstWindow () != this))
        {
          if (fwin->secondWindow () == this)
            return fwin->firstWindow ();

          while (fwin->mType != WindowType::FRAME)
            {
              fwin = fwin->secondWindow ();
            }
          return fwin;
        }
    }

  return nullptr;
}

ApvlvWindow *
ApvlvWindow::getBottom ()
{
  ApvlvWindow *fwin = this;

  while (fwin->parentWindow ())
    {
      fwin = fwin->parentWindow ();
      if (fwin->mType == WindowType::VSP && fwin->secondWindow () != this)
        {
          if (fwin->firstWindow () == this)
            return fwin->secondWindow ();

          while (fwin->mType != WindowType::FRAME)
            {
              fwin = fwin->firstWindow ();
            }
          return fwin;
        }
    }

  return nullptr;
}

void
ApvlvWindow::splitWidget (WindowType type, QWidget *one, QWidget *other)
{
  setFrameStyle (QFrame::NoFrame);
  setLineWidth (0);

  mPaned.setOrientation (type == WindowType::SP ? Qt::Horizontal
                                                : Qt::Vertical);
  mPaned.setHandleWidth (10);
  mPaned.setStretchFactor (0, 1);
  mPaned.setStretchFactor (1, 1);

  mPaned.addWidget (one);
  mPaned.addWidget (other);

  mType = type;
}

void
ApvlvWindow::perishWidget ()
{
  auto par = parentWindow ();
  auto win1 = par->firstWindow ();
  auto win2 = par->secondWindow ();
  auto rewin = (this == win1 ? win2 : win1);

  auto par_par = par->parent ();
  if (par_par->inherits ("QStackedWidget"))
    {
      win1->setParent (nullptr);
      win2->setParent (nullptr);

      if (rewin->mType == WindowType::FRAME)
        {
          auto frame = rewin->stealFrame ();
          par->setFrame (frame);
        }
      else
        {
          par->splitWidget (rewin->mType, rewin->firstWindow (),
                            rewin->secondWindow ());
        }

      qDebug () << win1 << " will be deleted ";
      win1->deleteLater ();
      qDebug () << win2 << " will be deleted ";
      win2->deleteLater ();
    }
  else
    {
      auto par_splitter = dynamic_cast<QSplitter *> (par_par);
      auto par0 = dynamic_cast<ApvlvWindow *> (par_splitter->widget ((0)));
      auto par1 = dynamic_cast<ApvlvWindow *> (par_splitter->widget ((1)));
      rewin->setParent (nullptr);
      par0->setParent (nullptr);
      par1->setParent (nullptr);
      if (par0 == par)
        {
          qDebug () << rewin << " and " << par1 << " will be inserted ";
          par_splitter->addWidget (rewin);
          par_splitter->addWidget (par1);
          qDebug () << par0 << " will be deleted ";
          par0->deleteLater ();
        }
      else
        {
          qDebug () << par0 << " and " << rewin << " will be inserted ";
          par_splitter->addWidget (par0);
          par_splitter->addWidget (rewin);
          qDebug () << par1 << " will be deleted ";
          par1->deleteLater ();
        }
    }
}

void
ApvlvWindow::setAsRootActive ()
{
  auto root_win = rootWindow ();
  if (root_win)
    root_win->setActiveWindow (this);
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

// birth a new FRAME window, and the new window beyond the input doc
// this made a FRAME window to SP|VSP
bool
ApvlvWindow::birth (WindowType type, ApvlvFrame *doc)
{
  Q_ASSERT (mType == WindowType::FRAME);

  auto frame = dynamic_cast<ApvlvFrame *> (mPaned.widget (0));
  frame->setParent (nullptr);

  if (doc == nullptr)
    {
      doc = frame->clone ();
      if (doc == nullptr)
        {
          frame->mView->errorMessage ("can't split");
          return false;
        }

      frame->mView->regLoaded (doc);
    }

  auto win1 = new ApvlvWindow ();
  win1->setFrame (frame);

  auto win2 = new ApvlvWindow ();
  win2->setFrame (doc);

  qDebug () << "win: " << this << " birth two: " << win1 << " and " << win2;
  splitWidget (type, win1, win2);

  if (auto root = rootWindow (); root && root->mActive == this)
    {
      root->mActive = win1;
    }

  return true;
}

void
ApvlvWindow::perish ()
{
  qDebug () << "win: " << this << " try to perish ";
  if (mType != WindowType::FRAME)
    return;

  if (auto root = rootWindow (); root && root->mActive == this)
    {
      root->mActive = nullptr;
    }

  perishWidget ();
}

void
ApvlvWindow::setActive (bool act)
{
  Q_ASSERT (mType == WindowType::FRAME);
  QTimer::singleShot (50, getFrame (), SLOT (setFocus ()));
}

void
ApvlvWindow::setFrame (ApvlvFrame *doc)
{
  qDebug () << "win: " << this << ", set core: " << doc;
  setFrameStyle (QFrame::Raised | QFrame::Box);
  setLineWidth (1);

  if (mType == WindowType::FRAME)
    {
      auto frame = stealFrame ();
      if (frame)
        {
          frame->inuse (false);
        }
    }

  mPaned.setStretchFactor (0, 1);
  mPaned.addWidget (doc);
  doc->inuse (true);
  mType = WindowType::FRAME;

  QObject::connect (doc, SIGNAL (focusIn ()), this, SLOT (setAsRootActive ()));
}

ApvlvFrame *
ApvlvWindow::stealFrame ()
{
  Q_ASSERT (mType == WindowType::FRAME);
  auto frame = dynamic_cast<ApvlvFrame *> (mPaned.widget (0));
  if (frame)
    {
      frame->setParent (nullptr);
    }
  return frame;
}

ApvlvFrame *
ApvlvWindow::getFrame ()
{
  Q_ASSERT (mType == WindowType::FRAME);
  auto frame = dynamic_cast<ApvlvFrame *> (mPaned.widget (0));
  return frame;
}

ApvlvWindow *
ApvlvWindow::firstWindow ()
{
  Q_ASSERT (mType != WindowType::FRAME);
  auto win = dynamic_cast<ApvlvWindow *> (mPaned.widget (0));
  return win;
}

ApvlvWindow *
ApvlvWindow::secondWindow ()
{
  Q_ASSERT (mType != WindowType::FRAME);
  auto win = dynamic_cast<ApvlvWindow *> (mPaned.widget (1));
  return win;
}

ApvlvWindow *
ApvlvWindow::parentWindow ()
{
  auto win = parent ();
  Q_ASSERT (win);
  if (win->inherits ("QSplitter"))
    win = win->parent ();
  return dynamic_cast<ApvlvWindow *> (win);
}

ApvlvWindow *
ApvlvWindow::rootWindow ()
{
  auto win = this;
  while (win && !win->isRoot ())
    win = win->parentWindow ();
  return win;
}

ApvlvWindow *
ApvlvWindow::firstFrameWindow ()
{
  auto win = this;
  while (win->mType != WindowType::FRAME)
    {
      win = win->firstWindow ();
    }

  return win;
}

bool
ApvlvWindow::isRoot ()
{
  auto par = parentWindow ();
  if (par == nullptr)
    return true;
  else
    return false;
}

void
ApvlvWindow::smaller (int times)
{
  if (isRoot ())
    return;

  auto pwin = parentWindow ();
  auto sizes = pwin->mPaned.sizes ();
  int len = 20 * times;
  if (pwin->firstWindow () == this)
    {
      sizes[0] -= len;
      sizes[1] += len;
    }
  else
    {
      sizes[0] += len;
      sizes[1] -= len;
    }
  pwin->mPaned.setSizes (sizes);
}

void
ApvlvWindow::bigger (int times)
{
  if (isRoot ())
    return;

  smaller (0 - times);
}

}

// Local Variables:
// mode: c++
// End:

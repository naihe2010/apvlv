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
/* @CPPFILE ApvlvWindow.h
 *
 *  Author: Alf <naihe2010@126.com>
 */

#ifndef _APVLV_WINDOW_H_
#define _APVLV_WINDOW_H_

#include <iostream>

#include "ApvlvFrame.h"

using namespace std;

namespace apvlv
{
class ApvlvFrame;
class ApvlvWindowContext;

class ApvlvWindow : public QObject
{
  Q_OBJECT
public:
  ApvlvWindow (ApvlvWindowContext *context, ApvlvFrame *core);
  ~ApvlvWindow () override;

  /* WE operate the AW_DOC window
   * Any AW_SP, AW_VSP are a virtual window, just for contain the AW_DOC window
   * AW_NONE is an empty window, need free
   * So, ANY user interface function can only get the AW_DOC window
   * */
  enum WindowType
  {
    AW_SP,
    AW_VSP,
    AW_CORE
  } mType;

  bool birth (WindowType type, ApvlvFrame *doc);

  void unbirth ();

  void setActive (bool act);

  QWidget *widget ();

  void setCore (ApvlvFrame *doc);

  ApvlvFrame *getCore ();

  bool isRoot ();

  void smaller (int times = 1);
  void bigger (int times = 1);

  ApvlvWindow *getNeighbor (int count, uint key);

  ApvlvWindow *getNext ();

  ReturnType process (int times, uint keyval);

  ApvlvWindow *m_parent, *m_child_1, *m_child_2;

private:
  ApvlvWindow *getLeft ();
  ApvlvWindow *getRight ();
  ApvlvWindow *getTop ();
  ApvlvWindow *getBottom ();

  void splitWidget (WindowType type, QWidget *one, QWidget *other);

  QSplitter *mPaned;

  ApvlvFrame *mCore;

  ApvlvWindowContext *mContext;
};

class ApvlvWindowContext : public QSplitter
{
public:
  explicit ApvlvWindowContext (ApvlvView *view, ApvlvWindow *root = nullptr,
                               ApvlvWindow *active = nullptr, int count = 0);
  ~ApvlvWindowContext () override = default;

  void registerWindow (ApvlvWindow *);
  void unregisterWindow (ApvlvWindow *);

  ApvlvView *
  getView () const
  {
    return mView;
  }

  ApvlvWindow *
  getRootWindow ()
  {
    return mRootWindow;
  }

  void
  setRootWindow (ApvlvWindow *win)
  {
    mRootWindow = win;
  }

  ApvlvWindow *
  getActiveWindow ()
  {
    return mActiveWindow;
  }

  void
  setActiveWindow (ApvlvWindow *win)
  {
    if (mActiveWindow == win)
      return;

    if (mActiveWindow)
      mActiveWindow->setActive (false);
    mActiveWindow = win;
    mActiveWindow->setActive (true);
  }

  [[nodiscard]] int
  getWindowCount () const
  {
    return mWindowCount;
  }

  ApvlvWindow *findWindowByWidget (QWidget *widget);

private:
  ApvlvView *mView;
  ApvlvWindow *mRootWindow;
  ApvlvWindow *mActiveWindow;
  int mWindowCount;
};

}

#endif

/* Local Variables: */
/* mode: c++ */
/* End: */

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

namespace apvlv
{
class ApvlvFrame;
class ApvlvWindowContext;

class ApvlvWindow final : public QFrame
{
  Q_OBJECT
public:
  ApvlvWindow ();
  ~ApvlvWindow () override;

  /* WE operate the AW_DOC window
   * Any SP, VSP are a virtual window, just for contain the AW_DOC window
   * AW_NONE is an empty window, need free
   * So, ANY user interface function can only get the AW_DOC window
   * */
  enum class WindowType
  {
    FRAME,
    SP,
    VSP,
  };
  WindowType mType{ WindowType::FRAME };

  bool birth (WindowType type, ApvlvFrame *doc);

  void perish ();

  void setActive (bool act);

  void setFrame (ApvlvFrame *doc);
  ApvlvFrame *stealFrame ();
  ApvlvFrame *getFrame ();

  ApvlvWindow *firstWindow ();
  ApvlvWindow *secondWindow ();
  ApvlvWindow *parentWindow ();
  ApvlvWindow *rootWindow ();
  ApvlvWindow *firstFrameWindow ();

  void
  setActiveWindow (ApvlvWindow *win)
  {
    mActive = win;
  }
  ApvlvWindow *
  getActiveWindow ()
  {
    return mActive;
  }

  bool isRoot ();

  void smaller (int times = 1);
  void bigger (int times = 1);

  ApvlvWindow *getNeighbor (int count, uint key);

  ApvlvWindow *getNext ();

  CmdReturn process (int times, uint keyval);

  ApvlvWindow *findWindowByWidget (QWidget *widget);

private:
  QVBoxLayout mLayout;
  QSplitter mPaned;

  ApvlvWindow *mActive{ nullptr };

  ApvlvWindow *getLeft ();
  ApvlvWindow *getRight ();
  ApvlvWindow *getTop ();
  ApvlvWindow *getBottom ();

  void splitWidget (WindowType type, QWidget *one, QWidget *other);

private slots:
  void perishWidget ();
  void setAsRootActive ();
};

}

#endif

/* Local Variables: */
/* mode: c++ */
/* End: */

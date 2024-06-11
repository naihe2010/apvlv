/*
 * This file is part of the apvlv package
 * Copyright (C) <2008>  <Alf>
 *
 * Contact: Alf <naihe2010@126.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */
/* @CPPFILE ApvlvMenuAndTool.cc
 *
 *  Author: Alf <naihe2010@126.com>
 */
/* @date Created: 2010/01/21 15:09:58 Alf*/

#include "ApvlvMenuAndTool.h"
#include "ApvlvUtil.h"
#include "ApvlvView.h"

extern "C"
{
  void apvlv_on_file_open (QWidget *wid, apvlv::ApvlvView *view);
  void apvlv_on_file_opentab (QWidget *wid, apvlv::ApvlvView *view);
  void apvlv_on_file_saveas (QWidget *wid, apvlv::ApvlvView *view);
  void apvlv_on_file_print (QWidget *wid, apvlv::ApvlvView *view);
  void apvlv_on_file_quit (QWidget *wid, apvlv::ApvlvView *view);
  void apvlv_on_page_previous (QWidget *wid, apvlv::ApvlvView *view);
  void apvlv_on_page_next (QWidget *wid, apvlv::ApvlvView *view);
  void apvlv_on_page_scrollup (QWidget *wid, apvlv::ApvlvView *view);
  void apvlv_on_page_scrolldown (QWidget *wid, apvlv::ApvlvView *view);
  void apvlv_on_navigate_jumpto (QWidget *wid, apvlv::ApvlvView *view);
  void apvlv_on_navigate_jumpback (QWidget *wid, apvlv::ApvlvView *view);
  void apvlv_on_help_about (QWidget *wid, apvlv::ApvlvView *view);
}

namespace apvlv
{
ApvlvMenuAndTool::ApvlvMenuAndTool (ApvlvView *view)
{
  mMenuBar = make_unique<QMenuBar> ();
  mToolBar = make_unique<QToolBar> ();
}

ApvlvMenuAndTool::~ApvlvMenuAndTool (){};

extern "C"
{
  void
  apvlv_on_file_open (QWidget *wid, ApvlvView *view)
  {
    view->open ();
  }

  void
  apvlv_on_file_opentab (QWidget *wid, apvlv::ApvlvView *view)
  {
    view->newtab (view->crtadoc ());
  }

  void
  apvlv_on_file_saveas (QWidget *wid, apvlv::ApvlvView *view)
  {
    // view->crtadoc()->save ();
  }

  void
  apvlv_on_file_print (QWidget *wid, apvlv::ApvlvView *view)
  {
    // view->crtadoc()->print ();
  }

  void
  apvlv_on_file_quit (QWidget *wid, apvlv::ApvlvView *view)
  {
    view->quit ();
  }

  void
  apvlv_on_page_previous (QWidget *wid, apvlv::ApvlvView *view)
  {
    view->crtadoc ()->prepage (1);
  }

  void
  apvlv_on_page_next (QWidget *wid, apvlv::ApvlvView *view)
  {
    view->crtadoc ()->nextpage (1);
  }

  void
  apvlv_on_page_scrollup (QWidget *wid, apvlv::ApvlvView *view)
  {
    view->crtadoc ()->scrollup (1);
  }

  void
  apvlv_on_page_scrolldown (QWidget *wid, apvlv::ApvlvView *view)
  {
    view->crtadoc ()->scrolldown (1);
  }

  void
  apvlv_on_navigate_jumpto (QWidget *wid, apvlv::ApvlvView *view)
  {
  }

  void
  apvlv_on_navigate_jumpback (QWidget *wid, apvlv::ApvlvView *view)
  {
    view->crtadoc ()->jump ('\'');
  }

  void
  apvlv_on_help_about (QWidget *wid, apvlv::ApvlvView *view)
  {
  }
}
};

// Local Variables:
// mode: c++
// End:

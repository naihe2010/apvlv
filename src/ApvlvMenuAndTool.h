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
/* @CPPFILE ApvlvMenuAndTool.h
 *
 *  Author: Alf <naihe2010@126.com>
 */
/* @date Created: 2010/01/21 15:09:25 Alf*/

#ifndef _APVLV_MENU_H_
#define _APVLV_MENU_H_

#include <gtk/gtk.h>

namespace apvlv
{
class ApvlvView;
class ApvlvMenuAndTool
{
public:
  explicit ApvlvMenuAndTool (ApvlvView *);
  ~ApvlvMenuAndTool ();

  GtkWidget *menubar ();
  GtkWidget *toolbar ();

private:
  GtkBuilder *mBuilder;
};
};

#endif

/* Local Variables: */
/* mode: c++ */
/* End: */

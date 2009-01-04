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
/* @CFILE ApvlvIter.hpp
*
*  Author: Alf <naihe2010@gmail.com>
*/
/* @date Created: 2009/01/03 23:28:55 Alf*/

#ifndef _APVLV_ITER_H_
#define _APVLV_ITER_H_

#ifdef HAVE_CONFIG_H
# include "config.hpp"
#endif

#include "ApvlvCore.hpp"
#include "ApvlvDoc.hpp"

#include <glib/poppler-document.h>

namespace apvlv
{
  class ApvlvIter;
  class ApvlvIterStatus: public ApvlvCoreStatus
  {
public:
  ApvlvIterStatus (ApvlvIter *itr);

  ~ApvlvIterStatus ();

  void show ();

  void setsize (int, int);

  void active (bool act);

private:
  ApvlvIter *mIter;
#define AI_STATUS_SIZE  4
  GtkWidget *mStlab[AI_STATUS_SIZE];
  };

  class ApvlvIter: public ApvlvCore
  {
public:
  ApvlvIter (ApvlvDoc *doc);

  ~ApvlvIter ();

  void setactive (bool act);

  const char *filename ();

  ApvlvDoc *getdoc ();

  void show (gdouble s);

  returnType process (int times, guint keyval);

private:
  static void apvlv_iter_on_changed (GtkTreeSelection *, ApvlvDoc *);

  void walk_index (GtkTreeIter *titr, PopplerIndexIter *iter);

  ApvlvDoc *mDoc;

  GtkWidget *mTreeView;
  GtkTreeStore *mTreeStore;
  GtkTreeIter *mParentIter;
  };
}

#endif

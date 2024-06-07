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
/* @CPPFILE ApvlvCore.h
 *
 *  Author: Alf <naihe2010@126.com>
 */
/* @date Created: 2021/07/19 20:34:00 Alf */

#ifndef _APVLV_CONTENT_H_
#define _APVLV_CONTENT_H_

#include "ApvlvFile.h"
#include "ApvlvUtil.h"

#include <gtk/gtk.h>

#include <iostream>
#include <map>

using namespace std;

namespace apvlv
{
class ApvlvDoc;
class ApvlvContent
{
public:
  explicit ApvlvContent ();

  virtual ~ApvlvContent ();

  GtkWidget *widget ();

  const ApvlvFileIndex currentIndex ();

  void setIndex (const ApvlvFileIndex &index);

  void setIndex (const ApvlvFileIndex &index, GtkTreeIter *root_itr);

  void setCurrentIndex (int pn, const char *anchor);

  void
  setDoc (ApvlvDoc *doc)
  {
    mDoc = doc;
  }

  void scrollup (int times);

  void scrolldown (int times);

  void scrollleft (int times);

  void scrollright (int times);

private:
  GtkWidget *mTreeView;
  GtkTreeStore *mStore;
  GtkTreeSelection *mSelection;
  GtkTreeIter mCurrentIter;

  ApvlvFileIndex mIndex;

  std::pair<int, string> mTargetIndex;

  ApvlvDoc *mDoc;
  static gboolean apvlv_set_iter_by_index (GtkTreeModel *model,
                                           GtkTreePath *path,
                                           GtkTreeIter *iter,
                                           ApvlvContent *content);
  static void apvlv_content_on_changed (GtkTreeSelection *, ApvlvContent *);

  static void apvlv_content_on_row_activated (GtkTreeView *tree_view,
                                              GtkTreePath *path,
                                              GtkTreeViewColumn *column,
                                              ApvlvContent *content);

  static gboolean apvlv_content_first_select_cb (ApvlvContent *);

private:
};
}

#endif

/* Local Variables: */
/* mode: c++ */
/* End: */

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

#include <QTimer>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <iostream>
#include <map>

#include "ApvlvFile.h"
#include "ApvlvUtil.h"

using namespace std;

namespace apvlv
{
class ApvlvDoc;

const int CONTENT_COL_TITLE = 0;
const int CONTENT_COL_PAGE = 1;
const int CONTENT_COL_ANCHOR = 2;
const int CONTENT_COL_PATH = 3;

class ApvlvContent : public QTreeWidget
{
  Q_OBJECT
public:
  ApvlvContent ();

  ~ApvlvContent () override = default;

  bool isReady ();

  unique_ptr<ApvlvFileIndex> currentIndex ();

  bool find_index_and_select (QTreeWidgetItem *itr, int pn,
                              const char *anchor);

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

  bool
  isFocused ()
  {
    return mIsFocused;
  }

  void
  setIsFocused (bool is_focused)
  {
    mIsFocused = is_focused;
  }

private:
  bool mIsFocused;

  ApvlvFileIndex mIndex;

  ApvlvDoc *mDoc{ nullptr };

  unique_ptr<QTimer> mFirstTimer;

  QTreeWidgetItem *mCurrentItem{ nullptr };

  void setIndex (const ApvlvFileIndex &index, QTreeWidgetItem *root_itr);

private slots:
  void on_changed ();
  void on_row_activated (QTreeWidgetItem *item, int column);
  void on_row_doubleclicked ();
  void first_select_cb ();
  void setIndex (const ApvlvFileIndex &index);
};
}

#endif

/* Local Variables: */
/* mode: c++ */
/* End: */

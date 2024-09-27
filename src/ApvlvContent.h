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
/* @CPPFILE ApvlvFrame.h
 *
 *  Author: Alf <naihe2010@126.com>
 */

#ifndef _APVLV_CONTENT_H_
#define _APVLV_CONTENT_H_

#include <QTimer>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <iostream>
#include <map>

#include "ApvlvFile.h"
#include "ApvlvUtil.h"

namespace apvlv
{
class ApvlvFrame;

const int CONTENT_COL_TITLE = 0;
const int CONTENT_COL_PAGE = 1;
const int CONTENT_COL_ANCHOR = 2;
const int CONTENT_COL_PATH = 3;
const int CONTENT_COL_TYPE = 4;

class ApvlvContent : public QTreeWidget
{
  Q_OBJECT
public:
  ApvlvContent ();

  ~ApvlvContent () override = default;

  bool isReady ();

  const FileIndex *currentItemFileIndex ();

  const FileIndex *currentFileFileIndex ();

  bool find_index_and_select (QTreeWidgetItem *itr, const std::string &path,
                              int pn, const std::string &anchor);
  bool find_index_and_append (FileIndex &root, const QString &path,
                              const FileIndex &index);

  void setCurrentIndex (const std::string &path, int pn,
                        const std::string &anchor);

  void
  setFrame (ApvlvFrame *frame)
  {
    mFrame = frame;
  }

  void scrollUp (int times);

  void scrollDown (int times);

  void scrollLeft (int times);

  void scrollRight (int times);

  [[nodiscard]] bool
  isFocused () const
  {
    return mIsFocused;
  }

  void
  setIsFocused (bool is_focused)
  {
    mIsFocused = is_focused;
  }

private:
  std::map<FileIndexType, QIcon> mTypeIcons;

  bool mIsFocused;

  FileIndex mIndex;

  ApvlvFrame *mFrame{ nullptr };

  std::unique_ptr<QTimer> mFirstTimer;

  QTreeWidgetItem *mCurrentItem{ nullptr };

  void setIndex (const FileIndex &index, QTreeWidgetItem *root_itr);
  void refreshIndex (const FileIndex &index);
  void appendIndex (const FileIndex &index);

  const FileIndex *treeItemToIndex (QTreeWidgetItem *item) const;
  const FileIndex *treeItemToFileIndex (QTreeWidgetItem *item) const;

private slots:
  void on_changed ();
  void on_row_activated (const QTreeWidgetItem *item, int column);
  void on_row_doubleclicked ();
  void first_select_cb ();
  void set_index (const FileIndex &index);
};
}

#endif

/* Local Variables: */
/* mode: c++ */
/* End: */

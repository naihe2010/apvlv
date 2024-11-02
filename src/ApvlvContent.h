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

#include <QComboBox>
#include <QLineEdit>
#include <QTimer>
#include <QToolBar>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QVBoxLayout>
#include <iostream>
#include <map>
#include <string>

#include "ApvlvFile.h"
#include "ApvlvUtil.h"

namespace apvlv
{
class ApvlvFrame;

class ApvlvContent final : public QFrame
{
  Q_OBJECT
public:
  ApvlvContent ();

  ~ApvlvContent () override = default;

  bool isReady ();

  enum class Column : int
  {
    Title = 0,
    MTime,
    FileSize,
  };
  static std::vector<const char *> ColumnString;
  static std::vector<const char *> SortByColumnString;

  enum class FilterType : int
  {
    Title = 0,
    FileName,
    MTimeBe,
    MTimeLe,
    FileSizeBe,
    FileSizeLe,
  };
  static std::vector<const char *> FilterTypeString;

  FileIndex *currentItemFileIndex ();

  FileIndex *currentFileFileIndex ();

  QTreeWidgetItem *findTreeWidgetItem (QTreeWidgetItem *itr,
                                       FileIndexType type,
                                       const std::string &path, int pn,
                                       const std::string &anchor);

  bool setCurrentIndex (const std::string &path, int pn,
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
  QVBoxLayout mLayout;
  QToolBar mToolBar;
  QComboBox mFilterType;
  QLineEdit mFilterText;
  QTreeWidget mTreeWidget;

  std::map<FileIndexType, QIcon> mTypeIcons;

  bool mIsFocused{ false };

  FileIndex mIndex;

  ApvlvFrame *mFrame{ nullptr };

  std::unique_ptr<QTimer> mFirstTimer;

  bool mSortAscending{ true };

  void setupToolBar ();
  void setupTree ();

  QTreeWidgetItem *
  selectedTreeItem ()
  {
    auto selitems = mTreeWidget.selectedItems ();
    return selitems.isEmpty () ? nullptr : selitems[0];
  }
  void setItemSelected (QTreeWidgetItem *item);

  void setIndex (FileIndex &index, QTreeWidgetItem *root_itr);
  void refreshIndex (const FileIndex &index);

  void setFileIndexToTreeItem (QTreeWidgetItem *item, FileIndex *index);
  FileIndex *getFileIndexFromTreeItem (QTreeWidgetItem *item);

  FileIndex *treeItemToFileIndex (QTreeWidgetItem *item);

  using filterFuncReturn = std::tuple<bool, bool>;
  using filterFunc = std::function<filterFuncReturn (const FileIndex *)>;
  void filterItemBy (QTreeWidgetItem *root, const filterFunc &filter_func);
  void setItemChildrenFilter (QTreeWidgetItem *root, bool is_filter);

private slots:
  void onRefresh ();
  void onFilter ();
  void
  sortBy (int method)
  {
    mSortAscending = !mSortAscending;
    if (method == static_cast<int> (Column::Title))
      mIndex.sortByTitle (mSortAscending);
    else if (method == static_cast<int> (Column::MTime))
      mIndex.sortByMtime (mSortAscending);
    else if (method == static_cast<int> (Column::FileSize))
      mIndex.sortByFileSize (mSortAscending);
    refreshIndex (mIndex);
  }

  void onRowActivated (QTreeWidgetItem *item, int column);
  void onRowDoubleClicked ();
  void selectFirstItem ();
  void setIndex (const FileIndex &index);
};
}

#endif

/* Local Variables: */
/* mode: c++ */
/* End: */

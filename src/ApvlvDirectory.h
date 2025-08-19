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
#include <QMenu>
#include <QTimer>
#include <QToolBar>
#include <QTreeWidgetItem>
#include <QVBoxLayout>
#include <iostream>
#include <map>
#include <string>

#include "ApvlvFile.h"
#include "ApvlvUtil.h"
#include "ApvlvWidget.h"

namespace apvlv
{

class ContentTree : public QTreeWidget
{
protected:
  void keyPressEvent (QKeyEvent *event) override;
};

class ApvlvFrame;
class Directory final : public QFrame
{
  Q_OBJECT
public:
  Directory ();

  ~Directory () override = default;

  bool isReady ();

  enum class Column : int
  {
    Title = 0,
    MTime,
    FileSize,
    Tags,
    Score
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
    Tags,
    ScoreBe,
    ScoreLe
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

  void
  focusFilter ()
  {
    QTimer::singleShot (50, &mFilterText, SLOT (setFocus ()));
  }

  void scrollUp (int times);

  void scrollDown (int times);

  void scrollLeft (int times);

  void scrollRight (int times);

  void tag ();

  void
  setActive (bool active)
  {
    if (active)
      {
        QTimer::singleShot (50, &mTreeWidget, SLOT (setFocus ()));
      }
    else
      {
        mTreeWidget.clearFocus ();
      }
  }

  bool
  isActive ()
  {
    return mTreeWidget.hasFocus ();
  }

private:
  QVBoxLayout mLayout;
  QToolBar mToolBar;
  ApvlvLineEdit mFilterText;
  QComboBox mFilterType;
  QComboBox mSortType;
  ContentTree mTreeWidget;

  QMenu mItemMenu;

  std::map<FileIndexType, QIcon> mTypeIcons;

  FileIndex mIndex;
  QStringList mTags;
  Column mSortColumn{ Column::Title };

  ApvlvFrame *mFrame{ nullptr };

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
  void onFileRename ();
  void onFileDelete ();
  void onRefresh ();
  void onFilter ();
  void
  sortBy (int method)
  {
    mSortAscending = !mSortAscending;
    mSortColumn = static_cast<Column> (method);
    sortItems (mTreeWidget.invisibleRootItem ());
  }

  void sortItems (QTreeWidgetItem *root);

  void onRowActivated (QTreeWidgetItem *item, int column);
  void onRowDoubleClicked ();
  void onContextMenuRequest (const QPoint &point);
  void selectFirstItem ();
  void setIndex (const FileIndex &index);
  void preloadTags (const std::string &path);
};
}

#endif

/* Local Variables: */
/* mode: c++ */
/* End: */

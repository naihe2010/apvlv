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
/* @CPPFILE ApvlvFrame.cc
 *
 *  Author: Alf <naihe2010@126.com>
 */

#include <QApplication>
#include <QComboBox>
#include <QDateTime>
#include <QHeaderView>
#include <QTreeWidget>
#include <filesystem>

#include "ApvlvContent.h"
#include "ApvlvFrame.h"
#include "ApvlvParams.h"
#include "ApvlvUtil.h"

namespace apvlv
{
using namespace std;

ApvlvContent::ApvlvContent ()
{
  setFocusPolicy (Qt::NoFocus);

  setLayout (&mLayout);
  mLayout.addWidget (&mToolBar, 0);
  mLayout.addWidget (&mTreeWidget);
  setupToolBar ();
  setupTree ();

  mFirstTimer = make_unique<QTimer> ();
  QObject::connect (mFirstTimer.get (), SIGNAL (timeout ()), this,
                    SLOT (firstSelected ()));
}

void
ApvlvContent::setupToolBar ()
{
  auto refresh = mToolBar.addAction (tr ("Refresh"));
  refresh->setIcon (QIcon::fromTheme (QIcon::ThemeIcon::ViewRefresh));
  QObject::connect (refresh, SIGNAL (triggered (bool)), this,
                    SLOT (onRefresh ()));

  auto csort = new QComboBox ();
  mToolBar.addWidget (csort);
  csort->addItems ({ tr ("Sort By Title"), tr ("Sort By Modified Time"),
                     tr ("Sort By File Size") });
  QObject::connect (csort, SIGNAL (activated (int)), this,
                    SLOT (sortBy (int)));

  auto filter = mToolBar.addAction (tr ("Filter"));
  filter->setIcon (QIcon::fromTheme (QIcon::ThemeIcon::ToolsCheckSpelling));
  QObject::connect (filter, SIGNAL (triggered (bool)), this,
                    SLOT (onFilter ()));

  auto expand_all = mToolBar.addAction (tr ("Expand All"));
  expand_all->setIcon (QIcon::fromTheme (QIcon::ThemeIcon::ListAdd));
  QObject::connect (expand_all, SIGNAL (triggered (bool)), &mTreeWidget,
                    SLOT (expandAll ()));
  auto collapse_all = mToolBar.addAction (tr ("Collapse All"));
  collapse_all->setIcon (QIcon::fromTheme (QIcon::ThemeIcon::ListRemove));
  QObject::connect (collapse_all, SIGNAL (triggered (bool)), &mTreeWidget,
                    SLOT (collapseAll ()));
}

void
ApvlvContent::setupTree ()
{
  mTreeWidget.setColumnCount (3);
  mTreeWidget.setColumnWidth (CONTENT_COL_TITLE, 400);
  mTreeWidget.setColumnWidth (CONTENT_COL_MTIME, 120);
  mTreeWidget.setColumnWidth (CONTENT_COL_FILE_SIZE, 120);
  mTreeWidget.setHeaderHidden (false);
  mTreeWidget.setHeaderLabels (
      { tr ("title"), tr ("modified time"), tr ("size") });
  mTreeWidget.setSortingEnabled (false);

  auto headerview = mTreeWidget.header ();
  headerview->setSectionsClickable (true);
  QObject::connect (headerview, SIGNAL (sectionClicked (int)), this,
                    SLOT (sortBy (int)));

  mTreeWidget.setVerticalScrollMode (
      QAbstractItemView::ScrollMode::ScrollPerItem);
  mTreeWidget.setSelectionBehavior (
      QAbstractItemView::SelectionBehavior::SelectRows);
  mTreeWidget.setSelectionMode (
      QAbstractItemView::SelectionMode::SingleSelection);

  mTypeIcons[FileIndexType::DIR] = QIcon (icondir.c_str ());
  mTypeIcons[FileIndexType::FILE] = QIcon (iconfile.c_str ());
  mTypeIcons[FileIndexType::PAGE] = QIcon (iconpage.c_str ());

  QObject::connect (&mTreeWidget,
                    SIGNAL (itemActivated (QTreeWidgetItem *, int)), this,
                    SLOT (onRowActivated (QTreeWidgetItem *, int)));
  QObject::connect (&mTreeWidget,
                    SIGNAL (itemDoubleClicked (QTreeWidgetItem *, int)), this,
                    SLOT (onRowDoubleClicked ()));
}

bool
ApvlvContent::isReady ()
{
  return (mTreeWidget.topLevelItemCount () > 0);
}

void
ApvlvContent::setIndex (const FileIndex &index)
{
  auto cur_index = currentItemFileIndex ();
  if (cur_index == nullptr || index.type == FileIndexType::DIR)
    {
      refreshIndex (index);
      return;
    }

  if (mIndex.type == FileIndexType::DIR
      && cur_index->type == FileIndexType::FILE
      && index.type == FileIndexType::FILE)
    {
      if (cur_index->mChildrenIndex.empty ())
        {
          auto cur_item = selectedTreeItem ();
          cur_index->appendChild (index);
          for (auto &child : cur_index->mChildrenIndex)
            {
              setIndex (child, cur_item);
            }
        }
    }
}

void
ApvlvContent::setItemSelected (QTreeWidgetItem *item)
{
  auto selitems = mTreeWidget.selectedItems ();
  for (auto selitem : selitems)
    {
      selitem->setSelected (false);
      auto index = getFileIndexFromTreeItem (selitem);
      if (index)
        index->isSelected = false;
    }

  auto parent = item->parent ();
  while (parent)
    {
      mTreeWidget.expandItem (parent);
      parent = parent->parent ();
    }

  item->setSelected (true);
  if (item->isExpanded ())
    mTreeWidget.collapseItem (item);
  mTreeWidget.scrollToItem (item);

  auto index = getFileIndexFromTreeItem (item);
  if (index)
    {
      index->isSelected = true;
      index->isExpanded = false;
    }
}

void
ApvlvContent::setIndex (FileIndex &index, QTreeWidgetItem *root_itr)
{
  auto itr = new QTreeWidgetItem ();
  setFileIndexToTreeItem (itr, &index);

  if (root_itr == nullptr)
    {
      mTreeWidget.addTopLevelItem (itr);
    }
  else
    {
      root_itr->addChild (itr);
    }

  for (auto &child : index.mChildrenIndex)
    {
      setIndex (child, itr);
    }
}

void
ApvlvContent::refreshIndex (const FileIndex &index)
{
  mTreeWidget.clear ();

  mIndex = index;

  for (auto &child : mIndex.mChildrenIndex)
    {
      setIndex (child, nullptr);
    }

  mFirstTimer->start (50);
}

void
ApvlvContent::setFileIndexToTreeItem (QTreeWidgetItem *item, FileIndex *index)
{
  auto variant = QVariant::fromValue<FileIndex *> (index);
  item->setData (CONTENT_COL_TITLE, Qt::UserRole, variant);
  item->setText (CONTENT_COL_TITLE,
                 QString::fromLocal8Bit (index->title.c_str ()));
  item->setIcon (CONTENT_COL_TITLE, mTypeIcons[index->type]);
  item->setToolTip (CONTENT_COL_TITLE, QString::fromLocal8Bit (index->path));
  if (index->type == FileIndexType::FILE)
    {
      item->setText (CONTENT_COL_FILE_SIZE, QString::number (index->size));
      auto date
          = QDateTime::fromSecsSinceEpoch (index->mtime, Qt::TimeSpec::UTC);
      item->setText (CONTENT_COL_MTIME, date.toString ("yyyy-MM-dd HH:mm:ss"));
    }
}

FileIndex *
ApvlvContent::getFileIndexFromTreeItem (QTreeWidgetItem *item)
{
  if (item == nullptr)
    return nullptr;

  auto varaint = item->data (CONTENT_COL_TITLE, Qt::UserRole);
  auto index = varaint.value<FileIndex *> ();
  return index;
}

FileIndex *
ApvlvContent::treeItemToFileIndex (QTreeWidgetItem *item)
{
  while (item != nullptr)
    {
      auto index = getFileIndexFromTreeItem (item);
      if (index->type == FileIndexType::FILE)
        return index;
      else
        item = item->parent ();
    }

  return nullptr;
}

QTreeWidgetItem *
ApvlvContent::findTreeWidgetItem (QTreeWidgetItem *itr, FileIndexType type,
                                  const string &path, int pn,
                                  const string &anchor)
{
  auto index = getFileIndexFromTreeItem (itr);
  if (index == nullptr)
    return nullptr;

  if (index->type != type)
    {
      for (auto ind = 0; ind < itr->childCount (); ++ind)
        {
          auto child_itr = itr->child (ind);
          auto citr = findTreeWidgetItem (child_itr, type, path, pn, anchor);
          if (citr)
            return citr;
        }

      return nullptr;
    }

  auto file_index = treeItemToFileIndex (itr);
  if (file_index == nullptr)
    {
      if (index->page == pn && (anchor.empty () || index->anchor == anchor))
        {
          return itr;
        }

      return nullptr;
    }

  if (file_index->path == path && index->page == pn
      && (anchor.empty () || index->anchor == anchor))
    {
      return itr;
    }

  return nullptr;
}

void
ApvlvContent::setCurrentIndex (const string &path, int pn,
                               const string &anchor)
{
  auto itr = selectedTreeItem ();
  auto fitr = findTreeWidgetItem (itr, FileIndexType::PAGE, path, pn, anchor);

  for (auto ind = 0; fitr == nullptr && ind < mTreeWidget.topLevelItemCount ();
       ++ind)
    {
      itr = mTreeWidget.topLevelItem (ind);
      fitr = findTreeWidgetItem (itr, FileIndexType::PAGE, path, pn, anchor);
    }

  for (auto ind = 0; fitr == nullptr && ind < mTreeWidget.topLevelItemCount ();
       ++ind)
    {
      itr = mTreeWidget.topLevelItem (ind);
      fitr = findTreeWidgetItem (itr, FileIndexType::FILE, path, pn, anchor);
    }

  if (fitr)
    setItemSelected (fitr);
}

void
ApvlvContent::scrollUp (int times)
{
  auto item = selectedTreeItem ();
  if (item == nullptr)
    return;

  auto parent = item->parent ();
  if (parent == nullptr)
    {
      auto index = mTreeWidget.indexOfTopLevelItem (item);
      if (index > 0)
        {
          auto new_index = index > times ? index - times : 0;
          auto itr = mTreeWidget.topLevelItem (new_index);
          setItemSelected (itr);
        }
    }
  else
    {
      auto index = parent->indexOfChild (item);
      if (index > 0)
        {
          auto new_index = index > times ? index - times : 0;
          auto itr = parent->child (new_index);
          setItemSelected (itr);
        }
    }
}

void
ApvlvContent::scrollDown (int times)
{
  auto item = selectedTreeItem ();
  if (item == nullptr)
    return;

  auto parent = item->parent ();
  if (parent == nullptr)
    {
      auto index = mTreeWidget.indexOfTopLevelItem (item);
      auto count = mTreeWidget.topLevelItemCount ();
      auto new_index = index + times < count ? index + times : count - 1;
      auto itr = mTreeWidget.topLevelItem (new_index);
      setItemSelected (itr);
    }
  else
    {
      auto index = parent->indexOfChild (item);
      auto count = parent->childCount ();
      auto new_index = index + times < count ? index + times : count - 1;
      auto itr = parent->child (new_index);
      setItemSelected (itr);
    }
}

void
ApvlvContent::scrollLeft (int times)
{
  auto item = selectedTreeItem ();
  if (item == nullptr)
    return;

  if (item->parent () == nullptr)
    return;

  auto parent = item->parent ();

  while (--times > 0 && parent->parent ())
    {
      parent = parent->parent ();
    }

  setItemSelected (parent);
}

void
ApvlvContent::scrollRight (int times)
{
  auto item = selectedTreeItem ();
  if (item == nullptr)
    return;

  if (item->childCount () == 0)
    return;

  item = item->child (0);

  while (--times > 0 && item->childCount () > 0)
    {
      item = item->child (0);
    }
  setItemSelected (item);
}

void
ApvlvContent::onRefresh ()
{
  refreshIndex (mIndex);
}

void
ApvlvContent::onFilter ()
{
}

void
ApvlvContent::onRowActivated ([[maybe_unused]] QTreeWidgetItem *item,
                              [[maybe_unused]] int column)
{
  mFrame->contentShowPage (currentItemFileIndex (), true);
  mFrame->toggledControlContent (true);
}

void
ApvlvContent::onRowDoubleClicked ()
{
  setIsFocused (true);
  parentWidget ()->setFocus ();
}

void
ApvlvContent::firstSelected ()
{
  if (mIndex.type == FileIndexType::DIR)
    return;

  if (mTreeWidget.topLevelItemCount () > 0)
    {
      if (mFrame->pageNumber () <= 1)
        {
          auto itr = mTreeWidget.topLevelItem (0);
          setItemSelected (itr);
        }
      else
        {
          setCurrentIndex (mFrame->filename (), mFrame->pageNumber (), "");
        }
    }

  mFirstTimer->stop ();
}

FileIndex *
ApvlvContent::currentItemFileIndex ()
{
  auto item = selectedTreeItem ();
  auto index = getFileIndexFromTreeItem (item);
  return index;
}

FileIndex *
ApvlvContent::currentFileFileIndex ()
{
  auto item = selectedTreeItem ();
  auto index = treeItemToFileIndex (item);
  return index;
}

}

// Local Variables:
// mode: c++
// End:

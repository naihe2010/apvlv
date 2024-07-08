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
/* @CPPFILE ApvlvCore.cc
 *
 *  Author: Alf <naihe2010@126.com>
 */
/* @date Created: 2021/07/19 20:34:51 Alf*/

#include <filesystem>

#include "ApvlvContent.h"
#include "ApvlvDoc.h"
#include "ApvlvParams.h"
#include "ApvlvUtil.h"

namespace apvlv
{
ApvlvContent::ApvlvContent ()
{
  setHeaderHidden (true);
  setColumnCount (1);
  setVerticalScrollMode (QAbstractItemView::ScrollMode::ScrollPerItem);
  setSelectionBehavior (QAbstractItemView::SelectionBehavior::SelectRows);
  setSelectionMode (QAbstractItemView::SelectionMode::SingleSelection);

  setFocusPolicy (Qt::NoFocus);

  mTypeIcons[FILE_INDEX_DIR] = QIcon (icondir.c_str ());
  mTypeIcons[FILE_INDEX_FILE] = QIcon (iconfile.c_str ());
  mTypeIcons[FILE_INDEX_PAGE] = QIcon (iconpage.c_str ());

  QObject::connect (this, SIGNAL (itemSelectionChanged ()), this,
                    SLOT (on_changed ()));
  QObject::connect (this, SIGNAL (itemActivated (QTreeWidgetItem *, int)),
                    this, SLOT (on_row_activated (QTreeWidgetItem *, int)));
  QObject::connect (this, SIGNAL (itemDoubleClicked (QTreeWidgetItem *, int)),
                    this, SLOT (on_row_doubleclicked ()));

  mFirstTimer = make_unique<QTimer> ();
  QObject::connect (mFirstTimer.get (), SIGNAL (timeout ()), this,
                    SLOT (first_select_cb ()));
}

bool
ApvlvContent::isReady ()
{
  return (topLevelItemCount () > 0);
}

void
ApvlvContent::set_index (const ApvlvFileIndex &index)
{
  auto cur_index = currentIndex ();
  if (cur_index == nullptr)
    {
      refreshIndex (index);
      return;
    }

  if (mIndex.type == ApvlvFileIndexType::FILE_INDEX_DIR
      && cur_index->type == ApvlvFileIndexType::FILE_INDEX_FILE
      && index.type == ApvlvFileIndexType::FILE_INDEX_FILE)
    {
      if (cur_index->mChildrenIndex.empty ())
        appendIndex (index);
    }
}

void
ApvlvContent::setIndex (const ApvlvFileIndex &index, QTreeWidgetItem *root_itr)
{
  auto itr = new QTreeWidgetItem ();
  itr->setText (CONTENT_COL_TITLE, QString::fromStdString (index.title));
  itr->setIcon (CONTENT_COL_TITLE, mTypeIcons[index.type]);
  itr->setText (CONTENT_COL_PAGE, QString::number (index.page));
  itr->setText (CONTENT_COL_ANCHOR, QString::fromStdString (index.anchor));
  itr->setText (CONTENT_COL_PATH, QString::fromStdString (index.path));
  itr->setText (CONTENT_COL_TYPE, QString::number (index.type));
  if (root_itr == nullptr)
    {
      addTopLevelItem (itr);
    }
  else
    {
      root_itr->addChild (itr);
    }

  for (const auto &child : index.mChildrenIndex)
    {
      setIndex (child, itr);
    }
}

void
ApvlvContent::refreshIndex (const ApvlvFileIndex &index)
{
  mIndex = index;

  clear ();
  mCurrentItem = nullptr;

  for (auto &child : mIndex.mChildrenIndex)
    {
      setIndex (child, nullptr);
    }

  mFirstTimer->start (50);
}

void
ApvlvContent::appendIndex (const ApvlvFileIndex &index)
{
  auto path = mCurrentItem->text (CONTENT_COL_PATH);
  find_index_and_append (mIndex, path, index);
  for (const auto &child : index.mChildrenIndex)
    {
      setIndex (child, mCurrentItem);
    }
}

const ApvlvFileIndex *
ApvlvContent::treeItemToIndex (QTreeWidgetItem *item) const
{
  if (item == nullptr)
    return nullptr;

  ApvlvFileIndex tmp_index;
  tmp_index.title = item->text (CONTENT_COL_TITLE).toStdString ();
  tmp_index.page = item->text (CONTENT_COL_PAGE).toInt ();
  tmp_index.anchor = item->text (CONTENT_COL_ANCHOR).toStdString ();
  tmp_index.path = item->text (CONTENT_COL_PATH).toStdString ();
  tmp_index.type = static_cast<ApvlvFileIndexType> (
      item->text (CONTENT_COL_TYPE).toInt ());
  auto index = mIndex.findIndex (tmp_index);
  return index;
}

const ApvlvFileIndex *
ApvlvContent::treeItemToFileIndex (QTreeWidgetItem *item) const
{
  auto type = FILE_INDEX_PAGE;
  while (item != nullptr)
    {
      type = static_cast<ApvlvFileIndexType> (
          item->text (CONTENT_COL_TYPE).toInt ());
      if (type == FILE_INDEX_FILE)
        break;
      else
        item = item->parent ();
    }

  if (item != nullptr)
    {
      auto index = treeItemToIndex (item);
      return index;
    }

  return nullptr;
}

bool
ApvlvContent::find_index_and_select (QTreeWidgetItem *itr, const string &path,
                                     int pn, const char *anchor)
{
  auto index = treeItemToIndex (itr);
  auto file_index = treeItemToFileIndex (itr);
  if ((!file_index || file_index->path == path) && index->page == pn
      && index->anchor == anchor)
    {
      if (mCurrentItem)
        mCurrentItem->setSelected (false);

      auto parent = itr->parent ();
      while (parent)
        {
          expandItem (parent);
          parent = parent->parent ();
        }

      itr->setSelected (true);
      return true;
    }

  for (auto ind = 0; ind < itr->childCount (); ++ind)
    {
      auto child_itr = itr->child (ind);
      if (find_index_and_select (child_itr, path, pn, anchor))
        return true;
    }

  return false;
}

bool
ApvlvContent::find_index_and_append (ApvlvFileIndex &root, const QString &path,
                                     const ApvlvFileIndex &index)
{
  if (root.type == ApvlvFileIndexType::FILE_INDEX_FILE
      && root.path == path.toStdString ())
    {
      root.appendChild (index);
      return true;
    }

  for (auto &child : root.mChildrenIndex)
    {
      if (find_index_and_append (child, path, index))
        return true;
    }

  return false;
}

void
ApvlvContent::setCurrentIndex (const string &path, int pn, const char *anchor)
{
  if (mIndex.type == FILE_INDEX_DIR)
    return;

  for (auto ind = 0; ind < topLevelItemCount (); ++ind)
    {
      auto itr = topLevelItem (ind);
      if (find_index_and_select (itr, path, pn, anchor))
        return;
    }
}

void
ApvlvContent::scrollup (int times)
{
  if (mCurrentItem == nullptr)
    return;

  auto parent = mCurrentItem->parent ();
  if (parent == nullptr)
    {
      auto index = indexOfTopLevelItem (mCurrentItem);
      if (index > 0)
        {
          auto itr = topLevelItem (index - 1);
          mCurrentItem->setSelected (false);
          itr->setSelected (true);
        }
    }
  else
    {
      auto index = parent->indexOfChild (mCurrentItem);
      if (index > 0)
        {
          auto itr = parent->child (index - 1);
          mCurrentItem->setSelected (false);
          itr->setSelected (true);
        }
    }
}

void
ApvlvContent::scrolldown (int times)
{
  if (mCurrentItem == nullptr)
    return;

  auto parent = mCurrentItem->parent ();
  if (parent == nullptr)
    {
      auto index = indexOfTopLevelItem (mCurrentItem);
      if (index + 1 < topLevelItemCount ())
        {
          auto itr = topLevelItem (index + 1);
          mCurrentItem->setSelected (false);
          itr->setSelected (true);
        }
    }
  else
    {
      auto index = parent->indexOfChild (mCurrentItem);
      if (index + 1 < parent->childCount ())
        {
          auto itr = parent->child (index + 1);
          mCurrentItem->setSelected (false);
          itr->setSelected (true);
        }
    }
}

void
ApvlvContent::scrollleft (int times)
{
  if (mCurrentItem == nullptr)
    return;

  auto parent = mCurrentItem->parent ();
  if (parent == nullptr)
    return;

  mCurrentItem->setSelected (false);
  parent->setSelected (true);
  collapseItem (parent);
}

void
ApvlvContent::scrollright (int times)
{
  if (mCurrentItem == nullptr)
    return;

  if (mCurrentItem->childCount () > 0)
    {
      expandItem (mCurrentItem);
      auto itr = mCurrentItem->child (0);
      mCurrentItem->setSelected (false);
      itr->setSelected (true);
    }
}

void
ApvlvContent::on_changed ()
{
  auto selects = selectedItems ();
  if (!selects.isEmpty ())
    {
      mCurrentItem = selects[0];
    }
}

void
ApvlvContent::on_row_activated (QTreeWidgetItem *item, int column)
{
  mDoc->contentShowPage (currentIndex (), true);
}

void
ApvlvContent::on_row_doubleclicked ()
{
  setIsFocused (true);
  parentWidget ()->setFocus ();
}

void
ApvlvContent::first_select_cb ()
{
  if (mIndex.type == FILE_INDEX_DIR)
    return;

  if (topLevelItemCount () > 0)
    {
      if (mDoc->pagenumber () <= 1)
        {
          auto itr = topLevelItem (0);
          itr->setSelected (true);
        }
      else
        {
          setCurrentIndex (mDoc->filename (), mDoc->pagenumber () - 1, "");
        }
    }

  mFirstTimer->stop ();
}

const ApvlvFileIndex *
ApvlvContent::currentIndex ()
{
  auto index = treeItemToIndex (mCurrentItem);
  return index;
}

const ApvlvFileIndex *
ApvlvContent::currentFileIndex ()
{
  auto index = treeItemToFileIndex (mCurrentItem);
  return index;
}

}

// Local Variables:
// mode: c++
// End:

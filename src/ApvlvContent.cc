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
ApvlvContent::setIndex (const ApvlvFileIndex &index)
{
  mIndex = index;

  clear ();
  mCurrentItem = nullptr;

  for (auto &child : index.mChildrenIndex)
    {
      setIndex (child, nullptr);
    }

  mFirstTimer->start (50);
}

void
ApvlvContent::setIndex (const ApvlvFileIndex &index, QTreeWidgetItem *root_itr)
{
  auto itr = new QTreeWidgetItem ();
  itr->setText (CONTENT_COL_TITLE, QString::fromStdString (index.title));
  itr->setText (CONTENT_COL_PAGE, QString::number (index.page));
  itr->setText (CONTENT_COL_ANCHOR, QString::fromStdString (index.anchor));
  itr->setText (CONTENT_COL_PATH, QString::fromStdString (index.path));
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

bool
ApvlvContent::find_index_and_select (QTreeWidgetItem *itr, int pn,
                                     const char *anchor)
{
  auto ipn = itr->text (CONTENT_COL_PAGE).toInt ();
  auto ianchor = itr->text (CONTENT_COL_ANCHOR);
  if (ipn == pn && ianchor == anchor)
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
      if (find_index_and_select (child_itr, pn, anchor))
        return true;
    }

  return false;
}

void
ApvlvContent::setCurrentIndex (int pn, const char *anchor)
{
  if (mIndex.type == FILE_INDEX_DIR)
    return;

  for (auto ind = 0; ind < topLevelItemCount (); ++ind)
    {
      auto itr = topLevelItem (ind);
      if (find_index_and_select (itr, pn, anchor))
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
  mDoc->contentShowPage (currentIndex ().get (), true);
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
          setCurrentIndex (mDoc->pagenumber () - 1, "");
        }
    }

  mFirstTimer->stop ();
}

unique_ptr<ApvlvFileIndex>
ApvlvContent::currentIndex ()
{
  if (mCurrentItem == nullptr)
    return nullptr;

  auto index = make_unique<ApvlvFileIndex> ();
  index->type = mIndex.type;

  index->title = mCurrentItem->text (CONTENT_COL_TITLE).toStdString ();
  index->page = mCurrentItem->text (CONTENT_COL_PAGE).toInt ();
  index->anchor = mCurrentItem->text (CONTENT_COL_ANCHOR).toStdString ();
  index->path = mCurrentItem->text (CONTENT_COL_PATH).toStdString ();

  return index;
}
}

// Local Variables:
// mode: c++
// End:

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
#include <QFile>
#include <QHeaderView>
#include <QInputDialog>
#include <QLocale>
#include <QMessageBox>
#include <QTimeZone>
#include <stack>

#include "ApvlvDirectory.h"
#include "ApvlvFrame.h"
#include "ApvlvNote.h"
#include "ApvlvNoteWidget.h"
#include "ApvlvParams.h"
#include "ApvlvUtil.h"

namespace apvlv
{
using namespace std;

std::vector<const char *> Directory::ColumnString = {
  QT_TR_NOOP ("Title"),     QT_TR_NOOP ("Modified Time"),
  QT_TR_NOOP ("File Size"), QT_TR_NOOP ("Tags"),
  QT_TR_NOOP ("Score"),
};
std::vector<const char *> Directory::SortByColumnString = {
  QT_TR_NOOP ("Sort By Title"),     QT_TR_NOOP ("Sort By Modified Time"),
  QT_TR_NOOP ("Sort By File Size"), QT_TR_NOOP ("Sort By Tags"),
  QT_TR_NOOP ("Sort By Score"),
};
std::vector<const char *> Directory::FilterTypeString = {
  QT_TR_NOOP ("Filter Title"),
  QT_TR_NOOP ("Filter File Name"),
  QT_TR_NOOP ("Filter Modified Time >="),
  QT_TR_NOOP ("Filter Modified Time <="),
  QT_TR_NOOP ("Filter File Size >="),
  QT_TR_NOOP ("Filter File Size <="),
  QT_TR_NOOP ("Filter Tag"),
  QT_TR_NOOP ("Filter Score >="),
  QT_TR_NOOP ("Filter Score <="),
};

void
ContentTree::keyPressEvent (QKeyEvent *event)
{
  event->ignore ();
}

Directory::Directory ()
{
  setLayout (&mLayout);
  mLayout.addWidget (&mToolBar, 0);
  mLayout.addWidget (&mTreeWidget);
  setupToolBar ();
  setupTree ();
  auto guioptions
      = ApvlvParams::instance ()->getStringOrDefault ("guioptions");
  if (guioptions.find ('S') == string::npos)
    {
      mToolBar.hide ();
    }
  QTimer::singleShot (50, this, SLOT (selectFirstItem ()));
}

void
Directory::setupToolBar ()
{
  mToolBar.addWidget (&mFilterText);
  QObject::connect (&mFilterText, SIGNAL (textEdited (const QString &)), this,
                    SLOT (onFilter ()));
  mToolBar.addSeparator ();
  mToolBar.addWidget (&mFilterType);
  for (auto const &str : FilterTypeString)
    {
      mFilterType.addItem (tr (str));
    }
  mToolBar.addSeparator ();
  QObject::connect (&mFilterType, SIGNAL (activated (int)), this,
                    SLOT (onFilter ()));

  auto refresh = mToolBar.addAction (tr ("Refresh"));
  refresh->setIcon (QIcon::fromTheme (QIcon::ThemeIcon::ViewRefresh));
  QObject::connect (refresh, SIGNAL (triggered (bool)), this,
                    SLOT (onRefresh ()));
  mToolBar.addSeparator ();

  auto expand_all = mToolBar.addAction (tr ("Expand All"));
  expand_all->setIcon (QIcon::fromTheme (QIcon::ThemeIcon::ListAdd));
  QObject::connect (expand_all, SIGNAL (triggered (bool)), &mTreeWidget,
                    SLOT (expandAll ()));
  auto collapse_all = mToolBar.addAction (tr ("Collapse All"));
  collapse_all->setIcon (QIcon::fromTheme (QIcon::ThemeIcon::ListRemove));
  QObject::connect (collapse_all, SIGNAL (triggered (bool)), &mTreeWidget,
                    SLOT (collapseAll ()));
  mToolBar.addSeparator ();

  mToolBar.addWidget (&mSortType);
  for (auto const &str : SortByColumnString)
    {
      mSortType.addItem (tr (str));
    }
  QObject::connect (&mSortType, SIGNAL (activated (int)), this,
                    SLOT (sortBy (int)));
}

void
Directory::setupTree ()
{
  mTreeWidget.setColumnCount (3);
  mTreeWidget.setColumnWidth (static_cast<int> (Column::Title), 300);
  mTreeWidget.setColumnWidth (static_cast<int> (Column::MTime), 150);
  mTreeWidget.setColumnWidth (static_cast<int> (Column::FileSize), 50);
  mTreeWidget.setColumnWidth (static_cast<int> (Column::Tags), 100);
  mTreeWidget.setColumnWidth (static_cast<int> (Column::Score), 50);
  mTreeWidget.setSortingEnabled (false);
  mTreeWidget.setHeaderHidden (false);
  QStringList labels;
  for (auto const &str : ColumnString)
    {
      labels.append (tr (str));
    }
  mTreeWidget.setHeaderLabels (labels);

  auto headerview = mTreeWidget.header ();
  headerview->setSectionsClickable (true);
  QObject::connect (headerview, SIGNAL (sectionClicked (int)), this,
                    SLOT (sortBy (int)));

  mTreeWidget.setVerticalScrollMode (
      QAbstractItemView::ScrollMode::ScrollPerItem);
  mTreeWidget.setSelectionBehavior (
      QAbstractItemView::SelectionBehavior::SelectRows);
  mTreeWidget.setSelectionMode (
      QAbstractItemView::SelectionMode::ExtendedSelection);

  mTypeIcons[FileIndexType::DIR] = QIcon (IconDir.c_str ());
  mTypeIcons[FileIndexType::FILE] = QIcon (IconFile.c_str ());
  mTypeIcons[FileIndexType::PAGE] = QIcon (IconPage.c_str ());

  QObject::connect (&mTreeWidget,
                    SIGNAL (itemActivated (QTreeWidgetItem *, int)), this,
                    SLOT (onRowActivated (QTreeWidgetItem *, int)));
  QObject::connect (&mTreeWidget,
                    SIGNAL (itemDoubleClicked (QTreeWidgetItem *, int)), this,
                    SLOT (onRowDoubleClicked ()));
  mTreeWidget.setContextMenuPolicy (Qt::ContextMenuPolicy::CustomContextMenu);
  QObject::connect (&mTreeWidget,
                    SIGNAL (customContextMenuRequested (const QPoint &)),
                    this, SLOT (onContextMenuRequest (const QPoint &)));
}

bool
Directory::isReady ()
{
  return (mTreeWidget.topLevelItemCount () > 0);
}

void
Directory::setIndex (const FileIndex &index)
{
  using enum FileIndexType;
  if (mTreeWidget.topLevelItemCount () == 0 || index.type == DIR)
    {
      refreshIndex (index);
      return;
    }

  auto cur_index = currentItemFileIndex ();
  if (mIndex.type == DIR && cur_index->type == FILE && index.type == FILE)
    {
      if (cur_index->mChildrenIndex.empty ())
        {
          auto cur_item = selectedTreeItem ();
          cur_index->moveChildChildren (index);
          for (auto &child : cur_index->mChildrenIndex)
            {
              setIndex (child, cur_item);
            }
        }
    }
}

void
Directory::preloadTags (const std::string &path)
{
  if (path.empty ())
    {
      return;
    }

  const auto &exts = FileFactory::supportFileExts ();
  for (const auto &entry :
       filesystem::recursive_directory_iterator (filesystem::path (path)))
    {
      if (!entry.is_regular_file ())
        continue;

      const auto &p = entry.path ();
      if (std::find (exts.begin (), exts.end (), p.extension ())
          == exts.end ())
        continue;

      const auto &np = Note::notePathOfPath (p.string ());
      const auto &n = make_unique<Note> ();
      if (n->load (np))
        {
          const auto &tags = n->tag ();
          for (const auto &tag : tags)
            {
              mTags.append (QString::fromLocal8Bit (tag));
            }
        }
    }
}

void
Directory::setItemSelected (QTreeWidgetItem *item)
{
  auto selitems = mTreeWidget.selectedItems ();
  for (auto selitem : selitems)
    {
      selitem->setSelected (false);
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
}

void
Directory::setIndex (FileIndex &index, QTreeWidgetItem *root_itr)
{
  auto itr = new QTreeWidgetItem ();
  setFileIndexToTreeItem (itr, &index);

  root_itr->addChild (itr);

  for (auto &child : index.mChildrenIndex)
    {
      setIndex (child, itr);
    }
}

void
Directory::refreshIndex (const FileIndex &index)
{
  mTreeWidget.clear ();

  mIndex = index;

  for (auto &child : mIndex.mChildrenIndex)
    {
      setIndex (child, mTreeWidget.invisibleRootItem ());
    }

  sortItems (mTreeWidget.invisibleRootItem ());

  preloadTags (mIndex.path);

  QTimer::singleShot (50, this, SLOT (selectFirstItem ()));
}

void
Directory::setFileIndexToTreeItem (QTreeWidgetItem *item, FileIndex *index)
{
  using enum Column;
  auto variant = QVariant::fromValue<FileIndex *> (index);
  item->setData (static_cast<int> (Title), Qt::UserRole, variant);
  item->setText (static_cast<int> (Title),
                 QString::fromLocal8Bit (index->title.c_str ()));
  item->setIcon (static_cast<int> (Title), mTypeIcons[index->type]);
  item->setToolTip (static_cast<int> (Title),
                    QString::fromLocal8Bit (index->path));

  using enum FileIndexType;
  if (index->type == FILE)
    {
      const auto date = QDateTime::fromSecsSinceEpoch (
          index->mtime, QTimeZone::systemTimeZone ());
      item->setText (static_cast<int> (MTime),
                     date.toString ("yyyy-MM-dd HH:mm:ss"));
      auto size = QLocale ().formattedDataSize (index->size);
      item->setText (static_cast<int> (FileSize), size);
      item->setText (static_cast<int> (Tags),
                     QString::fromLocal8Bit (index->tags));
      item->setToolTip (static_cast<int> (Tags),
                        QString::fromLocal8Bit (index->tags));
      const auto score
          = index->score > 0 ? QString::number (index->score) : "";
      item->setText (static_cast<int> (Score), score);
    }
}

FileIndex *
Directory::getFileIndexFromTreeItem (QTreeWidgetItem *item)
{
  if (item == nullptr)
    return nullptr;

  auto varaint = item->data (static_cast<int> (Column::Title), Qt::UserRole);
  auto index = varaint.value<FileIndex *> ();
  return index;
}

FileIndex *
Directory::treeItemToFileIndex (QTreeWidgetItem *item)
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

void
Directory::filterItemBy (QTreeWidgetItem *root, const filterFunc &filter_func)
{
  std::stack<QTreeWidgetItem *> item_stack;
  for (auto i = 0; i < root->childCount (); ++i)
    {
      auto item = root->child (i);
      item_stack.push (item);
    }

  while (!item_stack.empty ())
    {
      auto item = item_stack.top ();
      item_stack.pop ();

      auto index = getFileIndexFromTreeItem (item);
      auto [is_filter, same_as_file] = filter_func (index);
      if (is_filter)
        {
          item->setHidden (false);

          auto parent = item->parent ();
          while (parent)
            {
              parent->setHidden (false);
              parent = parent->parent ();
            }
        }
      else
        {
          item->setHidden (true);
        }

      if (index->type == FileIndexType::FILE && same_as_file)
        {
          setItemChildrenFilter (item, is_filter);
        }
      else
        {
          for (int i = 0; i < item->childCount (); ++i)
            {
              auto child_item = item->child (i);
              item_stack.push (child_item);
            }
        }
    }
}

void
Directory::setItemChildrenFilter (QTreeWidgetItem *root, bool is_filter)
{
  filterItemBy (root,
                [is_filter] (const FileIndex *a) -> filterFuncReturn
                  {
                    return { is_filter, false };
                  });
}

QTreeWidgetItem *
Directory::findTreeWidgetItem (QTreeWidgetItem *itr, FileIndexType type,
                               const string &path, int pn,
                               const string &anchor)
{
  if (itr == nullptr)
    return nullptr;

  auto index = getFileIndexFromTreeItem (itr);
  if (index == nullptr || index->type != type)
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

bool
Directory::setCurrentIndex (const string &path, int pn, const string &anchor)
{
  auto itr = selectedTreeItem ();

  auto fitr = findTreeWidgetItem (itr, FileIndexType::PAGE, path, pn, anchor);

  if (fitr == nullptr)
    fitr = findTreeWidgetItem (mTreeWidget.invisibleRootItem (),
                               FileIndexType::PAGE, path, pn, anchor);

  if (fitr == nullptr)
    fitr = findTreeWidgetItem (mTreeWidget.invisibleRootItem (),
                               FileIndexType::FILE, path, pn, anchor);

  if (fitr)
    {
      setItemSelected (fitr);
      return true;
    }

  return false;
}

void
Directory::scrollUp (int times)
{
  auto item = selectedTreeItem ();
  if (item == nullptr)
    return;

  auto parent = item->parent ();
  if (parent == nullptr)
    {
      parent = mTreeWidget.invisibleRootItem ();
    }

  auto index = parent->indexOfChild (item);
  if (index > 0)
    {
      auto new_index = index > times ? index - times : 0;
      auto itr = parent->child (new_index);
      setItemSelected (itr);
    }
}

void
Directory::scrollDown (int times)
{
  auto item = selectedTreeItem ();
  if (item == nullptr)
    return;

  auto parent = item->parent ();
  if (parent == nullptr)
    {
      parent = mTreeWidget.invisibleRootItem ();
    }

  auto index = parent->indexOfChild (item);
  auto count = parent->childCount ();
  auto new_index = index + times < count ? index + times : count - 1;
  auto itr = parent->child (new_index);
  setItemSelected (itr);
}

void
Directory::scrollLeft (int times)
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
Directory::scrollRight (int times)
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
Directory::tag ()
{
  auto item = selectedTreeItem ();
  auto cur = getFileIndexFromTreeItem (item);
  if (cur == nullptr)
    return;
  if (cur->type != FileIndexType::FILE)
    return;
  auto path = Note::notePathOfPath (cur->path);
  auto note = make_unique<Note> ();
  filesystem::path note_path (path);
  if (exists (note_path))
    {
      if (note->load (path) == false)
        {
          auto msg = QString (tr ("load note of %s error")).arg (cur->path);
          QMessageBox::warning (nullptr, tr ("error"), msg);
          return;
        }
    }

  filesystem::path file_path{ cur->path };
  string filename = file_path.filename ();
  auto ans = NoteDialog::getTag (filename, note->tag (), mTags);
  if (ans.isEmpty ())
    {
      return;
    }

  note->addTag (ans.toStdString ());
  if (!exists (note_path))
    {
      note->dump (path);
    }

  auto tags = note->tagString ();
  if (cur->tags != tags)
    {
      cur->tags = tags;
      item->setText (static_cast<int> (Column::Tags),
                     QString::fromLocal8Bit (cur->tags));
    }
}

void
Directory::onFileRename ()
{
  auto items = mTreeWidget.selectedItems ();
  if (items.isEmpty ())
    return;

  auto item = items[0];
  auto index = getFileIndexFromTreeItem (item);
  if (index->type != FileIndexType::FILE)
    return;

  auto qpath = QString::fromLocal8Bit (index->path);
  auto text = QString (tr ("Input new name of %1")).arg (qpath);
  auto user_text = QInputDialog::getText (this, tr ("Rename"), text,
                                          QLineEdit::Normal, qpath);
  auto nname = user_text.trimmed ();
  if (nname.isEmpty ())
    return;

  if (QFile (qpath).rename (nname))
    {
      index->path = nname.toStdString ();
      index->title = index->path.substr (index->path.rfind ('/') + 1);
      setFileIndexToTreeItem (item, index);
    }
  else
    {
      text = QString (tr ("Rename %1 to %2 failed")).arg (qpath).arg (nname);
      QMessageBox::warning (this, tr ("Warning"), text);
    }
}

void
Directory::onFileDelete ()
{
  auto items = mTreeWidget.selectedItems ();
  if (items.isEmpty ())
    return;

  auto msg_res = QMessageBox::No;
  for (auto item : items)
    {
      auto index = getFileIndexFromTreeItem (item);
      if (index->type != FileIndexType::FILE)
        continue;

      auto qpath = QString::fromLocal8Bit (index->path);
      auto text
          = QString (tr ("Will delete the \n%1, confirm ?")).arg (qpath);
      if (msg_res == QMessageBox::No || msg_res == QMessageBox::Yes)
        {
          auto buttons = QMessageBox::Yes | QMessageBox::No;
          if (items.size () > 1)
            buttons = QMessageBox::Yes | QMessageBox::YesToAll
                      | QMessageBox::No | QMessageBox::NoToAll;
          msg_res = QMessageBox::question (this, tr ("Confirm"), text,
                                           buttons, QMessageBox::No);
        }

      if (msg_res == QMessageBox::NoToAll)
        return;

      else if (msg_res == QMessageBox::No)
        continue;

      auto parent = item->parent ();
      if (parent == nullptr)
        {
          auto offset = mTreeWidget.indexOfTopLevelItem (item);
          mIndex.removeChild (*index);
          item = mTreeWidget.takeTopLevelItem (offset);
          delete item;
        }
      else
        {
          auto pindex = getFileIndexFromTreeItem (parent);
          pindex->removeChild (*index);
          parent->removeChild (item);
        }

      qDebug () << "delete " << qpath;
      QFile::remove (qpath);
    }
}

void
Directory::onRefresh ()
{
  refreshIndex (mIndex);
}

void
Directory::onFilter ()
{
  auto root = mTreeWidget.invisibleRootItem ();
  auto cur = mFilterType.currentIndex ();
  auto type = static_cast<FilterType> (cur);
  auto text = mFilterText.text ().trimmed ();
  if (text.isEmpty ())
    {
      setItemChildrenFilter (root, true);
      return;
    }

  QDateTime datetime;
  qint64 qsize;
  decltype (FileIndex::size) size;
  float score;

  switch (type)
    {
    case FilterType::Title:
      filterItemBy (root,
                    [text] (const FileIndex *a) -> filterFuncReturn
                      {
                        return { a->title.find (text.toStdString ())
                                     != string::npos,
                                 false };
                      });
      break;

    case FilterType::FileName:
      filterItemBy (root,
                    [text] (const FileIndex *a) -> filterFuncReturn
                      {
                        return { a->type == FileIndexType::FILE
                                     && a->title.find (text.toStdString ())
                                            != string::npos,
                                 true };
                      });
      break;

    case FilterType::MTimeBe:
      datetime = QDateTime::fromString (text);
      filterItemBy (root,
                    [datetime] (const FileIndex *a) -> filterFuncReturn
                      {
                        return { a->type == FileIndexType::FILE
                                     && a->mtime
                                            >= datetime.toMSecsSinceEpoch (),
                                 true };
                      });
      break;

    case FilterType::MTimeLe:
      datetime = QDateTime::fromString (text);
      filterItemBy (root,
                    [datetime] (const FileIndex *a) -> filterFuncReturn
                      {
                        return { a->type == FileIndexType::FILE
                                     && a->mtime
                                            <= datetime.toMSecsSinceEpoch (),
                                 true };
                      });
      break;

    case FilterType::FileSizeBe:
      qsize = parseFormattedDataSize (text);
      size = static_cast<decltype (size)> (qsize);
      filterItemBy (root,
                    [size] (const FileIndex *a) -> filterFuncReturn
                      {
                        return { a->type == FileIndexType::FILE
                                     && a->size >= size,
                                 true };
                      });
      break;

    case FilterType::FileSizeLe:
      qsize = parseFormattedDataSize (text);
      size = static_cast<decltype (size)> (qsize);
      filterItemBy (root,
                    [size] (const FileIndex *a) -> filterFuncReturn
                      {
                        return { a->type == FileIndexType::FILE
                                     && a->size <= size,
                                 true };
                      });
      break;

    case FilterType::Tags:
      filterItemBy (root,
                    [text] (const FileIndex *a) -> filterFuncReturn
                      {
                        return { a->type == FileIndexType::FILE
                                     && a->tags.find (text.toStdString ())
                                            != string::npos,
                                 true };
                      });
      break;

    case FilterType::ScoreBe:
      score = QString (text).toFloat ();
      filterItemBy (root,
                    [score] (const FileIndex *a) -> filterFuncReturn
                      {
                        return { a->type == FileIndexType::FILE
                                     && a->score >= score,
                                 true };
                      });
      break;

    case FilterType::ScoreLe:
      score = QString (text).toFloat ();
      filterItemBy (root,
                    [score] (const FileIndex *a) -> filterFuncReturn
                      {
                        return { a->type == FileIndexType::FILE
                                     && a->score <= score,
                                 true };
                      });
      break;

    default:
      qWarning () << tr ("Filter Type is invalid");
      break;
    }

  mTreeWidget.expandAll ();
}

void
Directory::sortItems (QTreeWidgetItem *tree_iter)
{
  stack<QTreeWidgetItem *> needSort;
  needSort.push (tree_iter);

  using itemState = pair<QTreeWidgetItem *, bool>;
  while (!needSort.empty ())
    {
      auto root = needSort.top ();
      needSort.pop ();

      vector<itemState> item_list;
      for (auto i = 0; i < root->childCount (); ++i)
        {
          auto item = root->child (i);
          auto index = getFileIndexFromTreeItem (item);
          if (index->type != FileIndexType::PAGE)
            item_list.emplace_back (item, item->isExpanded ());
        }

      std::ranges::for_each (
          item_list,
          [this, &needSort] (itemState &p)
            {
              if (p.first->childCount () > 1)
                {
                  auto index = getFileIndexFromTreeItem (p.first);
                  if (index && index->type == FileIndexType::DIR)
                    {
                      needSort.push (p.first);
                    }
                }
            });

      if (mSortAscending)
        {
          switch (mSortColumn)
            {
              using enum Column;
            case Title:
              std::ranges::sort (item_list, std::ranges::greater (),
                                 [this] (itemState &p)
                                   {
                                     auto index
                                         = getFileIndexFromTreeItem (p.first);
                                     return index ? index->title : "";
                                   });
              break;
            case MTime:
              std::ranges::sort (item_list, std::ranges::greater (),
                                 [this] (itemState &p)
                                   {
                                     auto index
                                         = getFileIndexFromTreeItem (p.first);
                                     return index ? index->mtime : 0;
                                   });
              break;
            case FileSize:
              std::ranges::sort (item_list, std::ranges::greater (),
                                 [this] (itemState &p)
                                   {
                                     auto index
                                         = getFileIndexFromTreeItem (p.first);
                                     return index ? index->size : 0;
                                   });
              break;
            case Tags:
              std::ranges::sort (item_list, std::ranges::greater (),
                                 [this] (itemState &p)
                                   {
                                     auto index
                                         = getFileIndexFromTreeItem (p.first);
                                     return index ? index->tags : "";
                                   });
              break;
            case Score:
              std::ranges::sort (item_list, std::ranges::greater (),
                                 [this] (itemState &p)
                                   {
                                     auto index
                                         = getFileIndexFromTreeItem (p.first);
                                     return index ? index->score : 0.0f;
                                   });
              break;
            }
        }
      else
        {
          switch (mSortColumn)
            {
              using enum Column;
            case Title:
              std::ranges::sort (item_list, std::ranges::less (),
                                 [this] (itemState &p)
                                   {
                                     auto index
                                         = getFileIndexFromTreeItem (p.first);
                                     return index ? index->title : "";
                                   });
              break;
            case MTime:
              std::ranges::sort (item_list, std::ranges::less (),
                                 [this] (itemState &p)
                                   {
                                     auto index
                                         = getFileIndexFromTreeItem (p.first);
                                     return index ? index->mtime : 0;
                                   });
              break;
            case FileSize:
              std::ranges::sort (item_list, std::ranges::less (),
                                 [this] (itemState &p)
                                   {
                                     auto index
                                         = getFileIndexFromTreeItem (p.first);
                                     return index ? index->size : 0;
                                   });
              break;
            case Tags:
              std::ranges::sort (item_list, std::ranges::less (),
                                 [this] (itemState &p)
                                   {
                                     auto index
                                         = getFileIndexFromTreeItem (p.first);
                                     return index ? index->tags : "";
                                   });
              break;
            case Score:
              std::ranges::sort (item_list, std::ranges::less (),
                                 [this] (itemState &p)
                                   {
                                     auto index
                                         = getFileIndexFromTreeItem (p.first);
                                     return index ? index->score : 0.0f;
                                   });
              break;
            }
        }

      mTreeWidget.setUpdatesEnabled (false);
      for (auto i = 0;
           static_cast<decltype (item_list.size ())> (i) < item_list.size ();
           ++i)
        {
          auto p = item_list[i];
          auto oi = root->indexOfChild (p.first);
          if (oi != i)
            {
              root->takeChild (oi);
              root->insertChild (i, p.first);
              p.first->setExpanded (p.second);
            }
        }
      mTreeWidget.setUpdatesEnabled (true);
    }

  mTreeWidget.update ();
}

void
Directory::onRowActivated ([[maybe_unused]] QTreeWidgetItem *item,
                           [[maybe_unused]] int column)
{
  mFrame->directoryShowPage (currentItemFileIndex (), true);
  mFrame->toggledControlDirectory (true);
}

void
Directory::onRowDoubleClicked ()
{
  parentWidget ()->setFocus ();
}

void
Directory::onContextMenuRequest (const QPoint &point)
{
  auto items = mTreeWidget.selectedItems ();
  if (items.isEmpty ())
    return;

  auto item = items[0];
  auto index = getFileIndexFromTreeItem (item);
  if (index->type == FileIndexType::FILE)
    {
      mItemMenu.clear ();
      if (items.size () == 1)
        {
          auto rename_action = mItemMenu.addAction (tr ("Rename File"));
          QObject::connect (rename_action, SIGNAL (triggered (bool)), this,
                            SLOT (onFileRename ()));
        }
      if (std::all_of (items.begin (), items.end (),
                       [this] (QTreeWidgetItem *a)
                         {
                           auto i = getFileIndexFromTreeItem (a);
                           return i && i->type == FileIndexType::FILE;
                         }))
        {
          auto del_action = mItemMenu.addAction (tr ("Delete File"));
          del_action->setIcon (
              QIcon::fromTheme (QIcon::ThemeIcon::EditDelete));
          QObject::connect (del_action, SIGNAL (triggered (bool)), this,
                            SLOT (onFileDelete ()));
        }
      mItemMenu.popup (mTreeWidget.mapToGlobal (point));
    }
}

void
Directory::selectFirstItem ()
{
  if (setCurrentIndex (mFrame->filename (), mFrame->pageNumber (), ""))
    return;

  if (mTreeWidget.topLevelItemCount () > 0)
    {
      auto itr = mTreeWidget.topLevelItem (0);
      setItemSelected (itr);
    }
}

FileIndex *
Directory::currentItemFileIndex ()
{
  auto item = selectedTreeItem ();
  auto index = getFileIndexFromTreeItem (item);
  return index;
}

FileIndex *
Directory::currentFileFileIndex ()
{
  auto item = selectedTreeItem ();
  auto index = treeItemToFileIndex (item);
  return index;
}

}

// Local Variables:
// mode: c++
// End:

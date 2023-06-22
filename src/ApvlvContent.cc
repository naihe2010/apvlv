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

#include "ApvlvContent.h"
#include "ApvlvDoc.h"
#include "ApvlvParams.h"

#include <glib/gstdio.h>

#include <iostream>

namespace apvlv
{
ApvlvContent::ApvlvContent ()
{
  mIndex = nullptr;
  mDoc = nullptr;
  memset (&mCurrentIter, 0, sizeof (mCurrentIter));

  mStore = gtk_tree_store_new (2, G_TYPE_POINTER, G_TYPE_STRING);
  mTreeView = gtk_tree_view_new_with_model (GTK_TREE_MODEL (mStore));
  gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (mTreeView), FALSE);

  apvlv_widget_set_background (mTreeView);
  g_signal_connect (G_OBJECT (mTreeView), "row-activated",
                    G_CALLBACK (apvlv_content_on_row_activated), this);

  mSelection = gtk_tree_view_get_selection (GTK_TREE_VIEW (mTreeView));
  g_signal_connect (G_OBJECT (mSelection), "changed",
                    G_CALLBACK (apvlv_content_on_changed), this);

  auto *renderer = gtk_cell_renderer_text_new ();
  auto *column = gtk_tree_view_column_new ();
  gtk_tree_view_column_pack_start (column, renderer, FALSE);
  gtk_tree_view_column_add_attribute (column, renderer, "text", 1);
  gtk_tree_view_append_column (GTK_TREE_VIEW (mTreeView), column);
  gtk_tree_view_column_clicked (column);
}

ApvlvContent::~ApvlvContent () = default;

GtkWidget *
ApvlvContent::widget ()
{
  return mTreeView;
}

void
ApvlvContent::setIndex (ApvlvFileIndex *index)
{
  setIndex (index, nullptr);

  g_timeout_add (50, (gboolean (*) (gpointer))apvlv_content_first_select_cb,
                 this);
}

void
ApvlvContent::setIndex (ApvlvFileIndex *index, GtkTreeIter *root_itr)
{
  GtkTreeIter iter;

  if (index == nullptr)
    return;

  if (root_itr == nullptr)
    {
      mIndex = index;
      gtk_tree_store_clear (mStore);
    }

  for (const auto child : index->children)
    {
      gtk_tree_store_append (mStore, &iter, root_itr);
      gtk_tree_store_set (mStore, &iter, 0, child, 1, child->title.c_str (),
                          -1);
      setIndex (child, &iter);
    }
}

void
ApvlvContent::scrollup (int times)
{
  if (!mIndex)
    return;

  GtkTreePath *path;

  if ((path = gtk_tree_model_get_path (GTK_TREE_MODEL (mStore), &mCurrentIter))
      == nullptr)
    {
      return;
    }

  for (gboolean ret = TRUE; times > 0 && ret; times--)
    {
      ret = gtk_tree_path_prev (path);
    }

  gtk_tree_model_get_iter (GTK_TREE_MODEL (mStore), &mCurrentIter, path);
  gtk_tree_selection_select_iter (mSelection, &mCurrentIter);
  gtk_tree_view_scroll_to_cell (GTK_TREE_VIEW (mTreeView), path, nullptr, TRUE,
                                0.5, 0.0);
  gtk_tree_path_free (path);
}

void
ApvlvContent::scrolldown (int times)
{
  if (!mIndex)
    return;

  GtkTreeIter itr;
  gboolean ret;

  for (ret = TRUE, itr = mCurrentIter; times > 0 && ret; times--)
    {
      mCurrentIter = itr;
      ret = gtk_tree_model_iter_next (GTK_TREE_MODEL (mStore), &itr);
      if (ret)
        {
          mCurrentIter = itr;
        }
    }

  gtk_tree_selection_select_iter (mSelection, &mCurrentIter);

  GtkTreePath *path
      = gtk_tree_model_get_path (GTK_TREE_MODEL (mStore), &mCurrentIter);
  if (path)
    {
      gtk_tree_view_scroll_to_cell (GTK_TREE_VIEW (mTreeView), path, nullptr,
                                    TRUE, 0.5, 0.0);
      gtk_tree_path_free (path);
    }
}

void
ApvlvContent::scrollleft (int times)
{
  if (!mIndex)
    return;

  GtkTreeIter itr;
  for (gboolean ret = TRUE; times > 0 && ret; times--)
    {
      ret = gtk_tree_model_iter_parent (GTK_TREE_MODEL (mStore), &itr,
                                        &mCurrentIter);
      if (ret)
        {
          mCurrentIter = itr;
        }
    }

  gtk_tree_selection_select_iter (mSelection, &mCurrentIter);

  GtkTreePath *path
      = gtk_tree_model_get_path (GTK_TREE_MODEL (mStore), &mCurrentIter);
  if (path)
    {
      gtk_tree_view_scroll_to_cell (GTK_TREE_VIEW (mTreeView), path, nullptr,
                                    TRUE, 0.5, 0.0);
      gtk_tree_view_collapse_row (GTK_TREE_VIEW (mTreeView), path);
      gtk_tree_path_free (path);
    }
}

void
ApvlvContent::scrollright (int times)
{
  if (!mIndex)
    return;

  GtkTreeIter itr;
  for (gboolean ret = TRUE; times > 0 && ret; times--)
    {
      ret = gtk_tree_model_iter_children (GTK_TREE_MODEL (mStore), &itr,
                                          &mCurrentIter);
      if (ret)
        {
          mCurrentIter = itr;
        }
    }

  GtkTreePath *path
      = gtk_tree_model_get_path (GTK_TREE_MODEL (mStore), &mCurrentIter);
  if (path)
    {
      gtk_tree_view_expand_to_path (GTK_TREE_VIEW (mTreeView), path);
      gtk_tree_selection_select_iter (mSelection, &mCurrentIter);
      gtk_tree_view_scroll_to_cell (GTK_TREE_VIEW (mTreeView), path, nullptr,
                                    TRUE, 0.5, 0.0);
      gtk_tree_path_free (path);
    }
}

void
ApvlvContent::apvlv_content_on_changed (GtkTreeSelection *selection,
                                        ApvlvContent *content)
{
  GtkTreeModel *model;
  gtk_tree_selection_get_selected (selection, &model, &content->mCurrentIter);
}

void
ApvlvContent::apvlv_content_on_row_activated (GtkTreeView *tree_view,
                                              GtkTreePath *path,
                                              GtkTreeViewColumn *column,
                                              ApvlvContent *content)
{
  if (content->mDoc)
    content->mDoc->contentShowPage (content->currentIndex (), true);
}

gboolean
ApvlvContent::apvlv_content_first_select_cb (ApvlvContent *content)
{
  if (!content->mIndex)
    return FALSE;

  GtkTreeIter tree_iter;

  if (gtk_tree_model_get_iter_first (GTK_TREE_MODEL (content->mStore),
                                     &tree_iter))
    {
      gtk_tree_selection_select_iter (content->mSelection, &tree_iter);
      if (content->mDoc)
        content->mDoc->contentShowPage (content->currentIndex (), true);
    }
  return FALSE;
}

ApvlvFileIndex *
ApvlvContent::currentIndex ()
{
  if (!mIndex)
    return nullptr;

  ApvlvFileIndex *index;
  gtk_tree_model_get (GTK_TREE_MODEL (mStore), &mCurrentIter, 0, &index, -1);
  return index;
}
}

// Local Variables:
// mode: c++
// End:

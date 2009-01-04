/*
* This file is part of the apvlv package
*
* Copyright (C) 2008 Alf.
*
* Contact: Alf <naihe2010@gmail.com>
*
* This library is free software; you can redistribute it and/or
* modify it under the terms of the GNU Lesser General Public License
* as published by the Free Software Foundation; either version 2.1 of
* the License, or (at your option) any later version.
*
* This library is distributed in the hope that it will be useful, but
* WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
* Lesser General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public
* License along with this library; if not, write to the Free Software
* Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
* 02110-1301 USA
*
*/
/* @CFILE ApvlvIter.cpp
*
*  Author: Alf <naihe2010@gmail.com>
*/
/* @date Created: 2009/01/03 23:34:50 Alf*/

#include "ApvlvDoc.hpp"
#include "ApvlvIter.hpp"

#include <sys/stat.h>

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <glib/poppler.h>

#include <iostream>
#include <fstream>

namespace apvlv
{
  ApvlvIter::ApvlvIter (ApvlvDoc *doc)
    {
      mReady = false;

      mProCmd = 0;

      mRotatevalue = 0;

      mDoc = NULL;

      mResults = NULL;
      mSearchstr = "";

      mTreeStore = gtk_tree_store_new (2, G_TYPE_POINTER, G_TYPE_STRING);

      mTreeView = gtk_tree_view_new_with_model (GTK_TREE_MODEL (mTreeStore));
      gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (mTreeView), FALSE);
      gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (mScrollwin),
                                             mTreeView);

      GtkCellRenderer *renderer = gtk_cell_renderer_text_new ();
      g_object_set (G_OBJECT (renderer), "foreground", "black", "background", "white", NULL);
      GtkTreeViewColumn *column = gtk_tree_view_column_new_with_attributes ("title", renderer, "text", 1, NULL);
      gtk_tree_view_column_set_resizable (column, FALSE);
      gtk_tree_view_append_column (GTK_TREE_VIEW (mTreeView), column);

      mStatus = new ApvlvIterStatus (this);

      gtk_box_pack_start (GTK_BOX (mVbox), mScrollwin, FALSE, FALSE, 0);
      gtk_box_pack_end (GTK_BOX (mVbox), mStatus->widget (), FALSE, FALSE, 0);

      gtk_widget_show_all (mVbox);

      mDoc = doc;

      if (mDoc != NULL)
        {
          GtkTreeSelection *select = gtk_tree_view_get_selection (GTK_TREE_VIEW (mTreeView));
          g_signal_connect (G_OBJECT (select), "changed", G_CALLBACK (apvlv_iter_on_changed), mDoc);

          mZoominit = false;
          mLines = 50;
          mChars = 80;

          show (0.0);

          mStatus->show ();

          setactive (true);

          mReady = true;
        }
    }

  ApvlvIter::~ApvlvIter ()
    {
      delete mStatus;
    }

  returnType 
    ApvlvIter::process (int times, guint key)
      {
        return MATCH;
      }

  const char *
    ApvlvIter::filename ()
      {
        return mDoc->filename ();
      }

  ApvlvDoc *
    ApvlvIter::getdoc ()
      {
        return mDoc;
      }

  void
    ApvlvIter::show (gdouble s)
      {
        GtkTreeIter *itr = NULL;
        PopplerIndexIter *iiter= mDoc->indexiter ();
        walk_index (itr, iiter);
      }

  void
    ApvlvIter::setactive (bool act)
      {
        mStatus->active (act);
        mActive = act;
      }

  void
    ApvlvIter::walk_index (GtkTreeIter *titr, PopplerIndexIter *iter)
      {
        do
          {
            PopplerAction *act = poppler_index_iter_get_action (iter);
            GtkTreeIter nitr;
            if (* (PopplerActionType *) act == POPPLER_ACTION_GOTO_DEST)
              {
                PopplerActionGotoDest *pagd = (PopplerActionGotoDest *) act;
                gtk_tree_store_append (mTreeStore, &nitr, titr);
                gtk_tree_store_set (mTreeStore, &nitr, 0, pagd->dest, 1, pagd->title, -1);
              }

            PopplerIndexIter *child = poppler_index_iter_get_child (iter);
            if (child)
              {
                walk_index (&nitr, child);
                poppler_index_iter_free (child);
              }
          }
        while (poppler_index_iter_next (iter));
        gtk_tree_view_expand_all (GTK_TREE_VIEW (mTreeView));
      }

  ApvlvIterStatus::ApvlvIterStatus (ApvlvIter *itr)
    {
      mIter = itr;
      for (int i=0; i<AI_STATUS_SIZE; ++i)
        {
          mStlab[i] = gtk_label_new ("");
          gtk_box_pack_start (GTK_BOX (mHbox), mStlab[i], FALSE, FALSE, 0);
        }
    }

  ApvlvIterStatus::~ApvlvIterStatus ()
    {
    }

  void
    ApvlvIterStatus::active (bool act)
      {
        GdkColor c;

        if (act)
          {
            c.red = 300;
            c.green = 300;
            c.blue = 300;
          }
        else
          {
            c.red = 30000;
            c.green = 30000;
            c.blue = 30000;
          }

        for (unsigned int i=0; i<AI_STATUS_SIZE; ++i)
          {
            gtk_widget_modify_fg (mStlab[i], GTK_STATE_NORMAL, &c);
          }
      }

  void
    ApvlvIterStatus::setsize (int w, int h)
      {
        int sw[AI_STATUS_SIZE];
        sw[0] = w >> 1;
        sw[1] = sw[0] >> 1;
        sw[2] = sw[1] >> 1;
        for (unsigned int i=0; i<AI_STATUS_SIZE; ++i)
          {
            gtk_widget_set_usize (mStlab[i], sw[i], h);
          }
      }

  void
    ApvlvIterStatus::show ()
      {
        if (mIter)
          {
            char temp[AI_STATUS_SIZE][256];
            gchar *bn;
            bn = g_path_get_basename (mIter->filename ());
            snprintf (temp[0], sizeof temp[0], "%s", bn);
            snprintf (temp[1], sizeof temp[1], "%d%%", (int) (mIter->zoomvalue () * 100));
            snprintf (temp[2], sizeof temp[2], "%d%%", (int) (mIter->scrollrate () * 100));
            for (unsigned int i=0; i<AI_STATUS_SIZE; ++i)
              {
                gtk_label_set_text (GTK_LABEL (mStlab[i]), temp[i]);
              }
            g_free (bn);
          }
      }

  void
    ApvlvIter::apvlv_iter_on_changed (GtkTreeSelection *selection, ApvlvDoc *doc)
      {
        PopplerDest *dest;
        char *title;
        GtkTreeIter iter;
        GtkTreeModel *model;

        if (gtk_tree_selection_get_selected (selection, &model, &iter))
          {
            gtk_tree_model_get (model, &iter, 0, &dest, 1, &title, -1);

            doc->showpage (dest->page_num - 1, 0);
            g_print ("You selected a ref %s\n", title);

            g_free (title);
          }
      }

}

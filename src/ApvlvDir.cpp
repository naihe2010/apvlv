/*
* This file is part of the apvlv package
*
* Copyright (C) 2008 Alf.
*
* Contact: Alf <naihe2010@gmail.com>
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* as published by the Free Software Foundation; either version 2.1 of
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
/* @CFILE ApvlvDir.cpp
*
*  Author: Alf <naihe2010@gmail.com>
*/
/* @date Created: 2009/01/03 23:28:26 Alf*/

#include "ApvlvDir.hpp"

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <sys/stat.h>

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <glib/poppler.h>

#include <iostream>
#include <fstream>
#include <sstream>

namespace apvlv
{
  ApvlvDirNode::ApvlvDirNode (gint p)
    {
      mPagenum = p;
    }

  ApvlvDirNode::~ApvlvDirNode ()
    {
    }

  void 
    ApvlvDirNode::show (ApvlvWindow *win)
      {
        win->getDoc (false)->showpage (mPagenum);
      }

  ApvlvDirNodeDir::ApvlvDirNodeDir (const char *filename):
    ApvlvDirNode (0)
  {
  }

  ApvlvDirNodeDir::~ApvlvDirNodeDir ()
    {
    }

  void 
    ApvlvDirNodeDir::show (ApvlvWindow *win)
      {
      }

  ApvlvDirNodeFile::ApvlvDirNodeFile (const char *path, const char *file):
    ApvlvDirNodeDir (path)
    {
    }

  ApvlvDirNodeFile::~ApvlvDirNodeFile ()
    {
    }

  void 
    ApvlvDirNodeFile::show (ApvlvWindow *win)
      {
      }

  ApvlvDir::ApvlvDir (const char *zm, const char *path)
    {
      mReady = false;

      mProCmd = 0;

      mRotatevalue = 0;

      GtkTreeStore *store = gtk_tree_store_new (2, G_TYPE_POINTER, G_TYPE_STRING);
      mDirView = gtk_tree_view_new_with_model (GTK_TREE_MODEL (store));
      gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (mDirView), FALSE);
      gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (mScrollwin),
                                             mDirView);

      GtkCellRenderer *renderer = gtk_cell_renderer_text_new ();
      g_object_set (G_OBJECT (renderer), "foreground", "black", "background", "white", NULL);
      GtkTreeViewColumn *column = gtk_tree_view_column_new_with_attributes ("title", renderer, "text", 1, NULL);
      gtk_tree_view_column_set_resizable (column, FALSE);
      gtk_tree_view_append_column (GTK_TREE_VIEW (mDirView), column);

      GtkTreeSelection *select = gtk_tree_view_get_selection (GTK_TREE_VIEW (mDirView));
      g_signal_connect (G_OBJECT (select), "changed", G_CALLBACK (apvlv_dir_on_changed), this);

      mStatus = new ApvlvDirStatus (this);

      gtk_box_pack_start (GTK_BOX (mVbox), mScrollwin, FALSE, FALSE, 0);
      gtk_box_pack_end (GTK_BOX (mVbox), mStatus->widget (), FALSE, FALSE, 0);

      gtk_widget_show_all (mVbox);

      setzoom (zm);
    }

  ApvlvDir::ApvlvDir (const char *zm, ApvlvDoc *doc)
    {
      mReady = false;

      mProCmd = 0;

      mRotatevalue = 0;

      GtkTreeStore *store = gtk_tree_store_new (2, G_TYPE_POINTER, G_TYPE_STRING);
      mDirView = gtk_tree_view_new_with_model (GTK_TREE_MODEL (store));
      gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (mDirView), FALSE);
      gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (mScrollwin),
                                             mDirView);

      GtkCellRenderer *renderer = gtk_cell_renderer_text_new ();
      g_object_set (G_OBJECT (renderer), "foreground", "black", "background", "white", NULL);
      GtkTreeViewColumn *column = gtk_tree_view_column_new_with_attributes ("title", renderer, "text", 1, NULL);
      gtk_tree_view_column_set_resizable (column, FALSE);
      gtk_tree_view_append_column (GTK_TREE_VIEW (mDirView), column);

      GtkTreeSelection *select = gtk_tree_view_get_selection (GTK_TREE_VIEW (mDirView));
      g_signal_connect (G_OBJECT (select), "changed", G_CALLBACK (apvlv_dir_on_changed), this);

      mStatus = new ApvlvDirStatus (this);

      gtk_box_pack_start (GTK_BOX (mVbox), mScrollwin, FALSE, FALSE, 0);
      gtk_box_pack_end (GTK_BOX (mVbox), mStatus->widget (), FALSE, FALSE, 0);

      gtk_widget_show_all (mVbox);

      setzoom (zm);

      GtkTreeIter *itr = NULL;
      PopplerIndexIter *iiter= doc->indexiter ();
      walk_poppler_iter_index (itr, iiter);
    }

  ApvlvDir::~ApvlvDir ()
    {
      delete mStatus;
    }

  returnType
    ApvlvDir::subprocess (int ct, guint key)
      {
        guint procmd = mProCmd;
        mProCmd = 0;
        switch (procmd)
          {
          }

        return MATCH;
      }

  returnType
    ApvlvDir::process (int ct, guint key)
      {
        if (mProCmd != 0)
          {
            return subprocess (ct, key);
          }

        switch (key)
          {
          }

        return MATCH;
      }

  void
    ApvlvDir::setactive (bool act)
      {
        mStatus->active (act);
        mActive = act;
      }

  void
    ApvlvDir::walk_poppler_iter_index (GtkTreeIter *titr, PopplerIndexIter *iter)
      {
        GtkTreeStore *store = GTK_TREE_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (mDirView)));
        do
          {
            PopplerAction *act = poppler_index_iter_get_action (iter);
            GtkTreeIter nitr;
            if (* (PopplerActionType *) act == POPPLER_ACTION_GOTO_DEST)
              {
                PopplerActionGotoDest *pagd = (PopplerActionGotoDest *) act;
                ApvlvDirNode *node = new ApvlvDirNode (pagd->dest->page_num - 1);
                gtk_tree_store_append (store, &nitr, titr);
                gtk_tree_store_set (store, &nitr, 0, node, 1, pagd->title, -1);
              }

            PopplerIndexIter *child = poppler_index_iter_get_child (iter);
            if (child)
              {
                walk_poppler_iter_index (&nitr, child);
                poppler_index_iter_free (child);
              }
          }
        while (poppler_index_iter_next (iter));
      }

  void
    ApvlvDir::walk_path_file_index (GtkTreeIter *titr, const char *path)
      {
      }

  void
    ApvlvDir::apvlv_dir_on_changed (GtkTreeSelection *selection, ApvlvDir *dir)
      {
        ApvlvDirNode *node;
        char *title;
        GtkTreeIter iter;
        GtkTreeModel *model;
        ApvlvWindow *win;

        win = ApvlvWindow::currentWindow ()->getnext (1);

        if (gtk_tree_selection_get_selected (selection, &model, &iter))
          {
            gtk_tree_model_get (model, &iter, 0, &node, 1, &title, -1);

            node->show (win);
            g_print ("You selected a ref %s\n", title);

            g_free (title);
          }
      }

  ApvlvDirStatus::ApvlvDirStatus (ApvlvDir *doc)
    {
      mDoc = doc;
      for (int i=0; i<AD_STATUS_SIZE; ++i)
        {
          mStlab[i] = gtk_label_new ("");
          gtk_box_pack_start (GTK_BOX (mHbox), mStlab[i], FALSE, FALSE, 0);
        }
    }

  ApvlvDirStatus::~ApvlvDirStatus ()
    {
    }

  void
    ApvlvDirStatus::active (bool act)
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

        for (unsigned int i=0; i<AD_STATUS_SIZE; ++i)
          {
            gtk_widget_modify_fg (mStlab[i], GTK_STATE_NORMAL, &c);
          }
      }

  void
    ApvlvDirStatus::setsize (int w, int h)
      {
        int sw[AD_STATUS_SIZE];
        sw[0] = w >> 1;
        sw[1] = sw[0] >> 1;
        sw[2] = sw[1] >> 1;
        sw[3] = sw[1] >> 1;
        for (unsigned int i=0; i<AD_STATUS_SIZE; ++i)
          {
            gtk_widget_set_usize (mStlab[i], sw[i], h);
          }
      }

  void
    ApvlvDirStatus::show ()
      {
        if (mDoc->filename ())
          {
            char temp[AD_STATUS_SIZE][256];
            gchar *bn;
            bn = g_path_get_basename (mDoc->filename ());
            snprintf (temp[0], sizeof temp[0], "%s", bn);
            snprintf (temp[1], sizeof temp[1], "%d/%d", mDoc->pagenumber (), mDoc->pagesum ());
            snprintf (temp[2], sizeof temp[2], "%d%%", (int) (mDoc->zoomvalue () * 100));
            snprintf (temp[3], sizeof temp[3], "%d%%", (int) (mDoc->scrollrate () * 100));
            for (unsigned int i=0; i<AD_STATUS_SIZE; ++i)
              {
                gtk_label_set_text (GTK_LABEL (mStlab[i]), temp[i]);
              }
            g_free (bn);
          }
      }
}

/*
 * This file is part of the apvlv package
 *
 * Copyright (C) 2008 Alf.
 *
 * Contact: Alf <naihe2010@gmail.com>
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
/* @CFILE ApvlvDir.cpp
 *
 *  Author: Alf <naihe2010@gmail.com>
 */
/* @date Created: 2009/01/03 23:28:26 Alf*/

#include "ApvlvView.hpp"
#include "ApvlvParams.hpp"
#include "ApvlvDir.hpp"

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <sys/stat.h>

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <glib/poppler.h>
#include <glib/gstdio.h>

#include <iostream>
#include <fstream>
#include <sstream>

namespace apvlv
{
  ApvlvDirNode::ApvlvDirNode (gint p)
    {
      mPagenum = p;
      realname = NULL;
    }

  ApvlvDirNode::ApvlvDirNode (bool isdir, const char *real, const char *file)
    {
      mPagenum = isdir? -1: 0;
      g_snprintf (filename, sizeof filename, file);
      realname = g_strdup (real);
    }

  ApvlvDirNode::~ApvlvDirNode ()
    {
      if (realname)
        {
          g_free (realname);
        }
    }

  bool
    ApvlvDirNode::dest (const char **path, int *pn)
      {
        if (mPagenum == 0)
          {
            if (path != NULL)
              {
                *path = realname;
                return true;
              }
          }

        else if (mPagenum > 0)
          {
            if (pn != NULL)
              {
                *pn = mPagenum;
                return true;
              }
          }

        return false;
      }

  ApvlvDir::ApvlvDir (int w, int h)
    {
      mReady = false;

      mProCmd = 0;

      mRotatevalue = 0;

      mDirNodes = NULL;

      mStore = gtk_tree_store_new (2, G_TYPE_POINTER, G_TYPE_STRING);
      mDirView = gtk_tree_view_new_with_model (GTK_TREE_MODEL (mStore));
      gtk_container_add (GTK_CONTAINER (mScrollwin), mDirView);
      gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (mDirView), FALSE);

      mSelection = gtk_tree_view_get_selection (GTK_TREE_VIEW (mDirView));
      g_signal_connect (G_OBJECT (mSelection), "changed", G_CALLBACK (apvlv_dir_on_changed), this);

      /* Title Column */
      GtkCellRenderer *renderer = gtk_cell_renderer_text_new ();
      GtkTreeViewColumn *column = gtk_tree_view_column_new_with_attributes ("title", renderer, "text", 1, NULL);
      gtk_tree_view_column_set_sort_column_id (column, 1);
      gtk_tree_view_append_column (GTK_TREE_VIEW (mDirView), column);

      mStatus = new ApvlvDirStatus (this);

      gtk_box_pack_start (GTK_BOX (mVbox), mScrollwin, FALSE, FALSE, 0);
      gtk_box_pack_end (GTK_BOX (mVbox), mStatus->widget (), FALSE, FALSE, 0);

      gtk_widget_show_all (mVbox);

      setsize (w, h);
    }

  bool
    ApvlvDir::loadfile (const char *path, bool check)
      {
        struct stat buf[1];
        stat (path, buf);
        if (S_ISDIR (buf->st_mode))
          {
            mReady = walk_dir_path_index (NULL, path);
          }
        else
          {
            mDoc = file_to_popplerdoc (path);
            PopplerIndexIter *iter;
            if (mDoc != NULL
                && (iter = poppler_index_iter_new (mDoc)) != NULL
            )
              {
                mReady = walk_poppler_iter_index (NULL, iter);
              }
          }

        if (mReady)
          {
            mFilestr = path;
            mFirstSelTimer = g_timeout_add (50, (gboolean (*) (gpointer)) apvlv_dir_first_select_cb, this);
          }

        return mReady;
      }

  ApvlvDir::~ApvlvDir ()
    {
      if (mDirNodes)
        {
          for (GSList *list = mDirNodes; list; list = g_slist_next (list))
            {
              ApvlvDirNode *info = (ApvlvDirNode *) list->data;
              delete info;
            }
          g_slist_free (mDirNodes);
        }

      delete mStatus;
    }

  returnType
    ApvlvDir::subprocess (int ct, guint key)
      {
        guint procmd = mProCmd;
        mProCmd = 0;
        switch (procmd)
          {
          default:
            return NO_MATCH;
            break;
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
          case ':':
          case '/':
          case '?':
            gView->promptcommand (key);
            return NEED_MORE;
          case 't':
          case 'o':
          case GDK_Return:
            enter (key);
            break;
          case 'H':
            scrollto (0.0);
            break;
          case 'M':
            scrollto (0.5);
            break;
          case 'L':
            scrollto (1.0);
            break;
          case GDK_Up:
          case 'k':
            scrollup (ct);
            break;
          case CTRL ('n'):
          case CTRL ('j'):
          case GDK_Down:
          case 'j':
            scrolldown (ct);
            break;
          case GDK_BackSpace:
          case GDK_Left:
          case CTRL ('h'):
          case 'h':
            scrollleft (ct);
            break;
          case GDK_space:
          case GDK_Right:
          case CTRL ('l'):
          case 'l':
            scrollright (ct);
          }

        return MATCH;
      }

  bool 
    ApvlvDir::enter (guint key)
      {
        debug ("enter pressed");
        ApvlvDirNode *node;

        gtk_tree_model_get (GTK_TREE_MODEL (mStore), &mCurrentIter, 0, &node, -1);
        if (node == NULL)
          {
            return false;
          }

        const char *name = NULL;
        int pn = -1;
        if (!node->dest (&name, &pn))
          {
            return false;
          }

        ApvlvCore *ndoc = NULL;
        if (name != NULL)
          {
            if (gParams->valueb ("content"))
              {
                ndoc = new ApvlvDir (mWidth, mHeight);
                if (!ndoc->loadfile (name))
                  {
                    delete ndoc;
                    ndoc = NULL;
                  }
              }

            if (ndoc == NULL)
              {
                ndoc = new ApvlvDoc (mWidth, mHeight, gParams->values ("zoom"), gParams->valueb ("cache"));
                ndoc->loadfile (name);
              }
          }
        else
          {
            ApvlvDoc *adoc = new ApvlvDoc (mWidth, mHeight, gParams->values ("zoom"), gParams->valueb ("cache"));
            if (!adoc->loadfile (filename ()))
              {
                delete adoc;
                adoc = NULL;
              }

            if (adoc != NULL)
              {
                adoc->showpage (pn);
                ndoc = adoc;
              }
          }

        if (ndoc == NULL)
          {
            return false;
          }

        switch (key)
          {
          case GDK_Return:
            ApvlvWindow::currentWindow ()->setCore (ndoc);
            break;

          case 'o':
            ApvlvWindow::currentWindow ()->birth (false, ndoc);
            break;

          case 't':
            gView->newtab (ndoc);
            break;

          default:
            return false;
          }

        return true;
      }

  void
    ApvlvDir::scrollup (int times)
      {
        if (!mReady)
          return;

        GtkTreePath *path = gtk_tree_model_get_path (GTK_TREE_MODEL (mStore), &mCurrentIter);
        for (gboolean ret = TRUE; times > 0 && ret; times --)
          {
            ret = gtk_tree_path_prev (path);
          }

        gtk_tree_model_get_iter (GTK_TREE_MODEL (mStore), &mCurrentIter, path);
        gtk_tree_selection_select_iter (mSelection, &mCurrentIter);
        gtk_tree_view_scroll_to_cell (GTK_TREE_VIEW (mDirView), path, NULL, TRUE, 0.5, 0.0);
        gtk_tree_path_free (path);

        mStatus->show ();
      }

  void
    ApvlvDir::scrolldown (int times)
      {
        if (!mReady)
          return;

        GtkTreeIter itr;
        gboolean ret;

        for (ret = TRUE, itr = mCurrentIter; times > 0 && ret; times --)
          {
            mCurrentIter = itr;
            ret = gtk_tree_model_iter_next (GTK_TREE_MODEL (mStore), &itr);
            if (ret)
              {
                mCurrentIter = itr;
              }
          }

        gtk_tree_selection_select_iter (mSelection, &mCurrentIter);
        GtkTreePath *path = gtk_tree_model_get_path (GTK_TREE_MODEL (mStore), &mCurrentIter);
        gtk_tree_view_scroll_to_cell (GTK_TREE_VIEW (mDirView), path, NULL, TRUE, 0.5, 0.0);
        gtk_tree_path_free (path);

        mStatus->show ();
      }

  void
    ApvlvDir::scrollleft (int times)
      {
        if (!mReady)
          return;

        GtkTreeIter itr;
        for (gboolean ret = TRUE; times > 0 && ret; times --)
          {
            ret = gtk_tree_model_iter_parent (GTK_TREE_MODEL (mStore), &itr, &mCurrentIter);
            if (ret)
              {
                mCurrentIter = itr;
              }
          }

        gtk_tree_selection_select_iter (mSelection, &mCurrentIter);
        GtkTreePath *path = gtk_tree_model_get_path (GTK_TREE_MODEL (mStore), &mCurrentIter);
        gtk_tree_view_scroll_to_cell (GTK_TREE_VIEW (mDirView), path, NULL, TRUE, 0.5, 0.0);
        gtk_tree_view_collapse_row (GTK_TREE_VIEW (mDirView), path);
        gtk_tree_path_free (path);

        mStatus->show ();
      }

  void
    ApvlvDir::scrollright (int times)
      {
        if (!mReady)
          return;

        GtkTreeIter itr;
        for (gboolean ret = TRUE; times > 0 && ret; times --)
          {
            ret = gtk_tree_model_iter_children (GTK_TREE_MODEL (mStore), &itr, &mCurrentIter);
            if (ret)
              {
                mCurrentIter = itr;
              }
          }

        GtkTreePath *path = gtk_tree_model_get_path (GTK_TREE_MODEL (mStore), &mCurrentIter);
        gtk_tree_view_expand_to_path (GTK_TREE_VIEW (mDirView), path);
        gtk_tree_selection_select_iter (mSelection, &mCurrentIter);
        gtk_tree_view_scroll_to_cell (GTK_TREE_VIEW (mDirView), path, NULL, TRUE, 0.5, 0.0);
        gtk_tree_path_free (path);

        mStatus->show ();
      }

  void
    ApvlvDir::setactive (bool act)
      {
        mStatus->active (act);
        mActive = act;
      }

  bool
    ApvlvDir::walk_poppler_iter_index (GtkTreeIter *titr, PopplerIndexIter *iter)
      {
        bool has = true;
        do
          {
            GtkTreeIter nitr;

            PopplerAction *act = poppler_index_iter_get_action (iter);
            if (act && * (PopplerActionType *) act == POPPLER_ACTION_GOTO_DEST)
              {
                PopplerActionGotoDest *pagd = (PopplerActionGotoDest *) act;
                ApvlvDirNode *node = NULL;
                if (pagd->dest->type == POPPLER_DEST_NAMED) 
                  {
                    PopplerDest *destnew = poppler_document_find_dest (mDoc, pagd->dest->named_dest);
                    int pn = 1;
                    if (destnew != NULL)
                      {
                        pn = destnew->page_num - 1;
                        poppler_dest_free (destnew);
                      }
                    node = new ApvlvDirNode (pn);
                  }
                else
                  {
                    node = new ApvlvDirNode (pagd->dest->page_num - 1);
                  }

                if (node != NULL)
                  {
                    mDirNodes = g_slist_append (mDirNodes, node);
                    gtk_tree_store_append (mStore, &nitr, titr);
                    gtk_tree_store_set (mStore, &nitr, 0, node, 1, pagd->title, -1);
                  }
              }
            poppler_action_free (act);

            PopplerIndexIter *child = poppler_index_iter_get_child (iter);
            if (child)
              {
                walk_poppler_iter_index (&nitr, child);
                poppler_index_iter_free (child);
              }
          }
        while (poppler_index_iter_next (iter));
        return has;
      }

  void
    ApvlvDir::apvlv_dir_on_changed (GtkTreeSelection *selection, ApvlvDir *dir)
      {
        GtkTreeModel *model;
        gtk_tree_selection_get_selected (selection, &model, &dir->mCurrentIter);
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
            g_snprintf (temp[0], sizeof temp[0], "%s", bn);
            g_snprintf (temp[1], sizeof temp[1], "%d/%d", mDoc->pagenumber (), mDoc->pagesum ());
            g_snprintf (temp[2], sizeof temp[2], "%d%%", (int) (mDoc->zoomvalue () * 100));
            g_snprintf (temp[3], sizeof temp[3], "%d%%", (int) (mDoc->scrollrate () * 100));
            for (unsigned int i=0; i<AD_STATUS_SIZE; ++i)
              {
                gtk_label_set_text (GTK_LABEL (mStlab[i]), temp[i]);
              }
            g_free (bn);
          }
      }

  gboolean 
    ApvlvDir::apvlv_dir_first_select_cb (ApvlvDir *dir)
      {
        GtkTreeIter gtir;
        if (gtk_tree_model_get_iter_first (GTK_TREE_MODEL (dir->mStore), &gtir))
          {
            gtk_tree_selection_select_iter (dir->mSelection, &gtir);
          }
        return FALSE;
      }

  bool
    ApvlvDir::walk_dir_path_index (GtkTreeIter *itr, const char *path)
      {
        bool has = false;
        GDir *dir = g_dir_open (path, 0, NULL);
        if (dir != NULL)
          {
            const gchar *name;
            while ((name = g_dir_read_name (dir)) != NULL)
              {
                if (strcmp (name, ".") == 0)
                  {
                    debug ("avoid hidden file: %s", name);
                    continue;
                  }

                gchar *realname = g_strjoin ("/", path, name, NULL);
                debug ("add a item: %s[%s]", name, realname);

                ApvlvDirNode *node = NULL;
                struct stat buf[1];
                g_stat (realname, buf);
                if (S_ISDIR (buf->st_mode))
                  {
                    node = new ApvlvDirNode (true, realname, name);

                    GtkTreeIter mitr[1];
                    gtk_tree_store_append (mStore, mitr, itr);
                    gtk_tree_store_set (mStore, mitr, 0, node, 1, name, -1);

                    if (!walk_dir_path_index (mitr, realname))
                      {
                        gtk_tree_store_remove (mStore, mitr);
                        delete node;
                        node = NULL;
                      }

                    if (node != NULL)
                      {
                        mDirNodes = g_slist_append (mDirNodes, node);
                      }
                  }
                else if (g_ascii_strncasecmp (name + strlen (name) - 4, ".pdf", 4) == 0)
                  {
                    node = new ApvlvDirNode (false, realname, name);

                    mDirNodes = g_slist_append (mDirNodes, node);
                    GtkTreeIter mitr[1];
                    gtk_tree_store_append (mStore, mitr, itr);
                    gtk_tree_store_set (mStore, mitr, 0, node, 1, name, -1);

                    has = true;
                  }

                g_free (realname);
              }
          }
        g_dir_close (dir);

        return has;
      }
}

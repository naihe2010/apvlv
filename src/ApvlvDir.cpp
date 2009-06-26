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
  typedef struct
    {
      GtkTreeIter itr[1];

      gboolean isdir;
      char last[0x40];
      char realname[PATH_MAX];
      char filename[0x100];
      char modified[0x100];

      GSList *subfiles;

    } fileinfo_t;

  static void fileinfo_free (GSList *);
  static GSList * fileinfo_new (GSList *, const char *, const char *, struct stat *);
  static fileinfo_t * fileinfo_find (GSList *, GtkTreeModel *, GtkTreeIter *);

  static GSList * dir_to_list (const char *);
  static void list_to_store (GSList *, GtkTreeStore *, GtkTreeIter *);

  ApvlvDirNode::ApvlvDirNode (gint p)
    {
      mPagenum = p;
      mNamed = NULL;
    }

  ApvlvDirNode::ApvlvDirNode (const char *d)
    {
      mPagenum = 1;
      mNamed = strdup (d);
    }

  ApvlvDirNode::~ApvlvDirNode ()
    {
      if (mNamed != NULL)
        {
          free (mNamed);
          mNamed = NULL;
        }
    }

  void 
    ApvlvDirNode::show (ApvlvWindow *win)
      {
        if (mNamed != NULL)
          {
            PopplerDest *destnew = poppler_document_find_dest (win->getDoc (false)->getdoc (), 
                                                               mNamed);
            if (destnew != NULL)
              {
                mPagenum = destnew->page_num - 1;
                poppler_dest_free (destnew);
                free (mNamed);
                mNamed = NULL;
              }
          }
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

      fileinfos = NULL;

      GType types[] = {
          G_TYPE_STRING,		/* name */
          G_TYPE_STRING,		/* modified */
      };

      mStore = gtk_tree_store_newv (sizeof types / sizeof types[0], types);

      mDirView = gtk_tree_view_new_with_model (GTK_TREE_MODEL (mStore));
      gtk_container_add (GTK_CONTAINER (mScrollwin), mDirView);

      mSelection = gtk_tree_view_get_selection (GTK_TREE_VIEW (mDirView));
      g_signal_connect (G_OBJECT (mSelection), "changed", G_CALLBACK (apvlv_dir_on_changed0), this);

      /* Name Column */
      GtkCellRenderer *renderer = gtk_cell_renderer_text_new ();
      GtkTreeViewColumn *column = gtk_tree_view_column_new_with_attributes ("Name", renderer, "text", 0, NULL);
      gtk_tree_view_column_set_sort_column_id (column, 0);

      gtk_tree_view_append_column (GTK_TREE_VIEW (mDirView), column);

      /* Modification column */
      renderer = gtk_cell_renderer_text_new ();
      column = gtk_tree_view_column_new_with_attributes ("Modified", renderer, "text", 1, NULL);
      gtk_tree_view_column_set_sort_column_id (column, 1);
/*  gtk_tree_view_column_set_cell_data_func (column, renderer,
                                               text_data_func,
                                               this, NULL);*/      

      gtk_tree_view_append_column (GTK_TREE_VIEW (mDirView), column);

      mStatus = new ApvlvDirStatus (this);

      gtk_box_pack_start (GTK_BOX (mVbox), mScrollwin, FALSE, FALSE, 0);
      gtk_box_pack_end (GTK_BOX (mVbox), mStatus->widget (), FALSE, FALSE, 0);

      gtk_widget_show_all (mVbox);

      fileinfos = dir_to_list (path);
      list_to_store (fileinfos, mStore, NULL);

      setzoom (zm);

      mFirstSelTimer = g_timeout_add (50, (gboolean (*) (gpointer)) apvlv_dir_first_select_cb, this);

      mReady = true;
    }

  ApvlvDir::ApvlvDir (const char *zm, ApvlvDoc *doc)
    {
      mReady = false;

      mProCmd = 0;

      fileinfos = NULL;

      mRotatevalue = 0;

      mStore = gtk_tree_store_new (2, G_TYPE_POINTER, G_TYPE_STRING);
      mDirView = gtk_tree_view_new_with_model (GTK_TREE_MODEL (mStore));
      gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (mDirView), FALSE);
      gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (mScrollwin),
                                             mDirView);

      GtkCellRenderer *renderer = gtk_cell_renderer_text_new ();
      g_object_set (G_OBJECT (renderer), "foreground", "black", "background", "white", NULL);
      GtkTreeViewColumn *column = gtk_tree_view_column_new_with_attributes ("title", renderer, "text", 1, NULL);
      gtk_tree_view_column_set_resizable (column, FALSE);
      gtk_tree_view_append_column (GTK_TREE_VIEW (mDirView), column);

      mSelection = gtk_tree_view_get_selection (GTK_TREE_VIEW (mDirView));
      g_signal_connect (G_OBJECT (mSelection), "changed", G_CALLBACK (apvlv_dir_on_changed), this);

      mStatus = new ApvlvDirStatus (this);

      gtk_box_pack_start (GTK_BOX (mVbox), mScrollwin, FALSE, FALSE, 0);
      gtk_box_pack_end (GTK_BOX (mVbox), mStatus->widget (), FALSE, FALSE, 0);

      gtk_widget_show_all (mVbox);

      setzoom (zm);

      GtkTreeIter *itr = NULL;
      PopplerIndexIter *iiter= doc->indexiter ();
      walk_poppler_iter_index (itr, iiter);

      mFirstSelTimer = g_timeout_add (50, (gboolean (*) (gpointer)) apvlv_dir_first_select_cb, this);

      mReady = true;
    }

  ApvlvDir::~ApvlvDir ()
    {
      if (fileinfos)
        {
          fileinfo_free (fileinfos);
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
          case GDK_Return:
            enter ();
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
    ApvlvDir::enter ()
      {
        debug ("enter pressed");
        fileinfo_t *info;

        info = fileinfo_find (fileinfos, GTK_TREE_MODEL (mStore), &mCurrentIter);
        if (info == NULL
            || info->isdir)
          {
            return false;
          }

        bool bcache = false;
        const char *scache = gParams->value ("cache");
        if (strcmp (scache, "yes") == 0)
          {
            bcache = true;
          }

        ApvlvDoc *ndoc = new ApvlvDoc (gParams->value ("zoom"), bcache);
        ndoc->setsize (mWidth, mHeight);
        ndoc->loadfile (info->realname);
        ndoc->setsize (mWidth, mHeight);

        ApvlvWindow *win = ApvlvWindow::currentWindow ();
        win->setCore (ndoc);

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
        gtk_tree_path_free (path);
        gtk_tree_selection_select_iter (mSelection, &mCurrentIter);

        mStatus->show ();
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
            GtkTreeIter nitr;

            PopplerAction *act = poppler_index_iter_get_action (iter);
            if (act && * (PopplerActionType *) act == POPPLER_ACTION_GOTO_DEST)
              {
                PopplerActionGotoDest *pagd = (PopplerActionGotoDest *) act;
                ApvlvDirNode *node = NULL;
                if (pagd->dest->type == POPPLER_DEST_NAMED) 
                  {
                    node = new ApvlvDirNode (pagd->dest->named_dest);
                  }
                else
                  {
                    node = new ApvlvDirNode (pagd->dest->page_num - 1);
                  }

                gtk_tree_store_append (store, &nitr, titr);
                gtk_tree_store_set (store, &nitr, 0, node, 1, pagd->title, -1);
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
      }

  void
    ApvlvDir::walk_path_file_index (GtkTreeIter *titr, const char *path)
      {
      }

  void
    ApvlvDir::apvlv_dir_on_changed0 (GtkTreeSelection *selection, ApvlvDir *dir)
      {
        GtkTreeModel *model;
        gtk_tree_selection_get_selected (selection, &model, &dir->mCurrentIter);
      }

  void
    ApvlvDir::apvlv_dir_on_changed (GtkTreeSelection *selection, ApvlvDir *dir)
      {
        ApvlvDirNode *node;
        char *title;
        GtkTreeModel *model;
        ApvlvWindow *win;

        win = ApvlvWindow::currentWindow ()->getnext (1);

        if (gtk_tree_selection_get_selected (selection, &model, &dir->mCurrentIter))
          {
            gtk_tree_model_get (model, &dir->mCurrentIter, 0, &node, 1, &title, -1);
            node->show (win);
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

  static void
    fileinfo_free (GSList *fileinfos)
      {
        GSList *list;
        fileinfo_t *info;

        if ((list = fileinfos) != NULL)
          {
            do
              {
                info = (fileinfo_t *) list->data;

                if (info->subfiles != NULL)
                  {
                    fileinfo_free (info->subfiles);
                  }
                free (info);
              }
            while ((list = g_slist_next (list)) != NULL);

            g_slist_free (fileinfos);
            fileinfos = NULL;
          }
      }

  static GSList *
    fileinfo_new (GSList *list, const char *real, const char *file, struct stat *stat)
      {
        fileinfo_t *info;
        struct tm *tmp;
        gboolean isdir;
        GSList *sublist;

        if (S_ISDIR (stat->st_mode))
          {
            isdir = true;
            sublist = dir_to_list (real);
          }
        else
          {
            if (g_ascii_strncasecmp (file + strlen (file) - 4, ".pdf", 4) != 0)
              {
                debug ("avoid file: %s", file);
                return list;
              }

            isdir = false;
            sublist = NULL;
          }

        info = (fileinfo_t *) calloc (1, sizeof (fileinfo_t));
        if (info == NULL)
          {
            return list;
          }

        tmp = localtime (&stat->st_mtime);
        strftime (info->last, sizeof info->last, "%Y-%h-%m %H:%M:%S", tmp);

        info->isdir = isdir;
        if (info->isdir)
          {
            info->subfiles = sublist;
          }
        g_snprintf (info->realname, sizeof info->realname, real);
        g_snprintf (info->filename, sizeof info->filename, file);

        if (isdir)
          {
            list = g_slist_prepend (list, info);
          }
        else
          {
            list = g_slist_append (list, info);
          }

        return list;
      }

  static fileinfo_t *
    fileinfo_find (GSList *fileinfos, GtkTreeModel *model, GtkTreeIter * itr)
      {
        GSList *list;
        GtkTreePath *path, *ipath;
        fileinfo_t *info;

        path = gtk_tree_model_get_path (model, itr);
        if (path == NULL)
          {
            return NULL;
          }

        for (list = fileinfos; list != NULL; list = g_slist_next (list))
          {
            info = (fileinfo_t *) list->data;
            ipath = gtk_tree_model_get_path (model, info->itr);
            if (ipath == NULL)
              {
                continue;
              }

            if (gtk_tree_path_compare (path, ipath) == 0)
              {
                gtk_tree_path_free (ipath);
                gtk_tree_path_free (path);
                return info;
              }
            gtk_tree_path_free (ipath);

            if (info->subfiles != NULL)
              {
                info = fileinfo_find (info->subfiles, model, itr);
                if (info != NULL)
                  {
                    gtk_tree_path_free (path);
                    return info;
                  }
              }
          }

        gtk_tree_path_free (path);
        return NULL;
      }

  static GSList * 
    dir_to_list (const char *path)
      {
        GSList *list = NULL;

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

                struct stat buf[1];
                g_stat (realname, buf);
                list = fileinfo_new (list, realname, name, buf);
                g_free (realname);
              }
          }
        g_dir_close (dir);

        return list;
      }

  static void 
    list_to_store (GSList *list, GtkTreeStore *store, GtkTreeIter *itr)
      {
        fileinfo_t *info;
        GSList *node;

        for (node = list; node; node = g_slist_next (node))
          {
            debug ("");
            info = (fileinfo_t *) node->data;
            gtk_tree_store_append (store, info->itr, itr);
            gtk_tree_store_set (store, info->itr, 0, info->filename, 1, info->last, -1);
            if (info->subfiles != NULL)
              {
                list_to_store (info->subfiles, store, info->itr);
              }
          }
      }

  void
    ApvlvDir::icon_data_func (GtkTreeViewColumn * column,
                    GtkCellRenderer * cell,
                    GtkTreeModel * model, GtkTreeIter * iter, gpointer data)
      {
        GSList *list = ((ApvlvDir *) data)->fileinfos;
        fileinfo_t *info;
        GdkPixbuf *pixbuf;

        info = fileinfo_find (list, model, iter);
        if (info)
          {
            if (info->isdir)
              {
                pixbuf =
                  gdk_pixbuf_new_from_file_at_size (icondir.c_str (), 40, 20, NULL);
              }
            else if (info->isdir == false)
              {
                pixbuf =
                  gdk_pixbuf_new_from_file_at_size (iconreg.c_str (), 40, 20, NULL);
              }
            else
              {
                pixbuf =
                  gdk_pixbuf_new_from_file_at_size (iconpdf.c_str (), 40, 20, NULL);
              }

            if (pixbuf)
              {
                g_object_set (cell, "pixbuf", pixbuf, NULL);
                g_object_unref (pixbuf);
              }
          }
      }

  void
    ApvlvDir::text_data_func (GtkTreeViewColumn * column,
                    GtkCellRenderer * cell,
                    GtkTreeModel * model, GtkTreeIter * iter, gpointer data)
      {
        GSList *list = ((ApvlvDir *) data)->fileinfos;
        fileinfo_t *info;
        gint id;

        info = fileinfo_find (list, model, iter);
        if (info)
          {
            id = gtk_tree_view_column_get_sort_column_id (column);
            asst (id >= 0);
            if (id == 0)
              {
                g_object_set (cell, "text", info->filename, NULL);
              }
            else if (id == 1)
              {
                g_object_set (cell, "text", info->last, NULL);
              }
          }
      }
}

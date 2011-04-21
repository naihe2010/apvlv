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

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <glib/gstdio.h>

#include <cerrno>
#include <sys/stat.h>
#include <iostream>
#include <fstream>
#include <sstream>

namespace apvlv
{
ApvlvDirNode::ApvlvDirNode (ApvlvDir *dir, GtkTreeIter * ir, gint p)
{
  mDir = dir;
  *itr = *ir;
  mPagenum = p;
  realname = NULL;
}

ApvlvDirNode::ApvlvDirNode (ApvlvDir *dir, GtkTreeIter * ir, bool isdir,
                            const char *real, const char *file)
{
  mDir = dir;
  *itr = *ir;
  mPagenum = isdir ? -1 : 0;
  g_snprintf (filename, sizeof filename, "%s", file);
  realname = g_strdup (real);

  if (isdir && gParams->valuei ("autoreload") > 0)
    {
      mGFile = g_file_new_for_path (realname);
      if (mGFile)
        {
          GError *error = NULL;
          mGMonitor = g_file_monitor (mGFile, G_FILE_MONITOR_NONE, NULL, NULL);
          if (error != NULL)
            {
              debug ("Create file monitor failed: %s\n", error->message);
              g_error_free (error);
            }
        }
      else
        {
          mGMonitor = NULL;
        }

      if (mGMonitor)
        {
          g_file_monitor_set_rate_limit (mGMonitor, gParams->valuei ("autoreload") * 1000);
          g_signal_connect (G_OBJECT (mGMonitor), "changed", G_CALLBACK (apvlv_dirnode_monitor_callback), this);
        }
    }
}

ApvlvDirNode::~ApvlvDirNode ()
{
  if (realname)
    {
      g_free (realname);
    }
}

void ApvlvDirNode::apvlv_dirnode_monitor_callback (GFileMonitor *mon, GFile *gf1, GFile *gf2, GFileMonitorEvent ev, ApvlvDirNode *node)
{
  if (ev != G_FILE_MONITOR_EVENT_DELETED
      && ev != G_FILE_MONITOR_EVENT_CREATED)
    {
      return;
    }

  gchar * name = g_file_get_path (gf1);
  if (name == NULL)
    {
      debug ("Can't get path name.\n");
      return;
    }

  gchar *basename = g_file_get_basename (gf1);
  if (basename == NULL)
    {
      basename = g_path_get_basename (name);
    }
  if (basename == NULL)
    {
      basename = g_strdup (name);
    }

  if (g_ascii_strncasecmp (basename + strlen (basename) - 4, ".pdf", 4)
      != 0
#ifdef HAVE_LIBUMD
      && g_ascii_strncasecmp (basename + strlen (basename) - 5,
                              ".djvu", 5) != 0
      && g_ascii_strncasecmp (basename + strlen (basename) - 4, ".djv",
                              4) != 0
#endif
#ifdef HAVE_LIBUMD
      && g_ascii_strncasecmp (basename + strlen (basename) - 4,
                              ".umd", 4) != 0
#endif
     )
    {
      g_free (name);
      g_free (basename);
      return;
    }

  if (ev == G_FILE_MONITOR_EVENT_DELETED)
    {
      debug ("delete file: %s", name);

      GList *listnode;
      ApvlvDirNode *nnode;
      for (listnode = g_list_first (node->mDir->mDirNodes);
           listnode != NULL;
           listnode = g_list_next (listnode))
        {
          nnode = (ApvlvDirNode *) listnode->data;
          if (strcmp (nnode->filename, basename) == 0)
            {
              break;
            }
        }

      if (nnode != NULL)
        {
          gtk_tree_store_remove (node->mDir->mStore, nnode->itr);
          node->mDir->mDirNodes = g_list_remove (node->mDir->mDirNodes, nnode);
          delete nnode;
        }
    }

  else if (ev == G_FILE_MONITOR_EVENT_CREATED)
    {
      debug ("add file: %s", name);

      GtkTreeIter mitr[1];
      gtk_tree_store_append (node->mDir->mStore, mitr, node->itr);
      ApvlvDirNode *nnode = new ApvlvDirNode (node->mDir, mitr, false, name, basename);
      node->mDir->mDirNodes = g_list_append (node->mDir->mDirNodes, nnode);

      GdkPixbuf *pix =
        gdk_pixbuf_new_from_file_at_size (iconpdf.c_str (), 40, 20,
                                          NULL);
      if (pix)
        {
          gtk_tree_store_set (node->mDir->mStore, mitr, 0, nnode, 1, pix, 2,
                              basename, -1);
          g_object_unref (pix);
        }
      else
        {
          gtk_tree_store_set (node->mDir->mStore, mitr, 0, nnode, 2, basename, -1);
        }
    }

  g_free (basename);
  g_free (name);
}

bool ApvlvDirNode::dest (const char **path, int *pn)
{
  if (mPagenum == 0 && path != NULL)
    {
      *path = realname;
      return true;
    }

  else if (mPagenum > 0 && pn != NULL)
    {
      *pn = mPagenum;
      return true;
    }

  return false;
}

const char *ApvlvDirNode::phrase ()
{
  return filename;
}

const GtkTreeIter *ApvlvDirNode::iter ()
{
  return itr;
}

ApvlvDir::ApvlvDir (int w, int h)
{
  mReady = false;

  mProCmd = 0;

  mRotatevalue = 0;

  mDirNodes = NULL;

  mIndex = NULL;

  mFile = NULL;

  mStore =
    gtk_tree_store_new (3, G_TYPE_POINTER, G_TYPE_OBJECT, G_TYPE_STRING);
  mDirView = gtk_tree_view_new_with_model (GTK_TREE_MODEL (mStore));
  gtk_container_add (GTK_CONTAINER (mScrollwin), mDirView);
  gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (mDirView), FALSE);

  mSelection = gtk_tree_view_get_selection (GTK_TREE_VIEW (mDirView));
  g_signal_connect (G_OBJECT (mSelection), "changed",
                    G_CALLBACK (apvlv_dir_on_changed), this);

  /* Title Column */
  GtkCellRenderer *renderer0 = gtk_cell_renderer_pixbuf_new ();
  GtkCellRenderer *renderer = gtk_cell_renderer_text_new ();
  GtkTreeViewColumn *column = gtk_tree_view_column_new ();
  gtk_tree_view_column_pack_start (column, renderer0, FALSE);
  gtk_tree_view_column_pack_start (column, renderer, FALSE);
  gtk_tree_view_column_add_attribute (column, renderer0, "pixbuf", 1);
  gtk_tree_view_column_add_attribute (column, renderer, "text", 2);
  gtk_tree_view_column_set_sort_column_id (column, 2);
  gtk_tree_view_column_set_sort_order (column, GTK_SORT_ASCENDING);
  gtk_tree_view_append_column (GTK_TREE_VIEW (mDirView), column);
  gtk_tree_view_column_clicked (column);

  mStatus = new ApvlvDirStatus (this);

  gtk_box_pack_start (GTK_BOX (mVbox), mScrollwin, FALSE, FALSE, 0);
  gtk_box_pack_end (GTK_BOX (mVbox), mStatus->widget (), FALSE, FALSE, 0);

  gtk_widget_show_all (mVbox);

  setsize (w, h);
}

bool ApvlvDir::reload ()
{
  gtk_tree_store_clear (mStore);

  if (mDirNodes)
    {
      for (GList * list = mDirNodes; list; list = g_list_next (list))
        {
          ApvlvDirNode *info = (ApvlvDirNode *) list->data;
          delete info;
        }
      g_list_free (mDirNodes);
      mDirNodes = NULL;
    }

  if (mIndex != NULL)
    {
      mFile->free_index (mIndex);
      mIndex = NULL;
    }

  if (mFile != NULL)
    {
      delete mFile;
      mFile = NULL;
    }

  loadfile (mFilestr.c_str (), FALSE);

  return true;
}

bool ApvlvDir::loadfile (const char *path, bool check)
{
  gchar *rpath;

  if (path == NULL
      || *path == '\0'
      || (rpath = g_locale_from_utf8 (path, -1, NULL, NULL, NULL)) == NULL)
    {
      gView->errormessage ("path error: %s", path ? path : "No path");
      return false;
    }

  struct stat buf[1];
  int ret = stat (rpath, buf);
  g_free (rpath);
  if (ret < 0)
    {
      gView->errormessage ("stat error: %d:%s", errno, strerror (errno));
      return false;
    }

  if (S_ISDIR (buf->st_mode))
    {
      mType = CORE_DIR;
      mReady = walk_dir_path_index (NULL, path);
    }
  else
    {
      mType = CORE_CONTENT;
      mFile = ApvlvFile::newfile (path);

      if (mFile != NULL && (mIndex = mFile->new_index ()) != NULL)
        {
          for (ApvlvFileIndexIter itr = mIndex->children.begin ();
               itr != mIndex->children.end (); ++itr)
            {
              bool ready = walk_file_index (NULL, itr);
              if (mReady == false)
                {
                  mReady = ready;
                }
            }
        }
      else
        {
          mReady = false;
        }
    }

  if (mReady)
    {
      mFilestr = path;
      mFirstSelTimer =
        g_timeout_add (50,
                       (gboolean (*)(gpointer)) apvlv_dir_first_select_cb,
                       this);

      if (gParams->valuei ("autoreload") > 0)
        {
          mGFile = g_file_new_for_path (path);
          if (mGFile)
            {
              GError *error = NULL;
              mGMonitor = g_file_monitor (mGFile, G_FILE_MONITOR_NONE, NULL, NULL);
              if (error != NULL)
                {
                  debug ("Create file monitor failed: %s\n", error->message);
                  g_error_free (error);
                }
            }
          else
            {
              mGMonitor = NULL;
            }

          if (mGMonitor)
            {
              g_file_monitor_set_rate_limit (mGMonitor, gParams->valuei ("autoreload") * 1000);
              g_signal_connect (G_OBJECT (mGMonitor), "changed", G_CALLBACK (apvlv_dir_monitor_callback), this);
            }
        }
    }

  return mReady;
}

ApvlvDir::~ApvlvDir ()
{
  if (mDirNodes)
    {
      for (GList * list = mDirNodes; list; list = g_list_next (list))
        {
          ApvlvDirNode *info = (ApvlvDirNode *) list->data;
          delete info;
        }
      g_list_free (mDirNodes);
    }

  if (mIndex != NULL)
    {
      mFile->free_index (mIndex);
    }

  if (mFile != NULL)
    {
      delete mFile;
    }

  delete mStatus;
}

returnType ApvlvDir::subprocess (int ct, guint key)
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

returnType ApvlvDir::process (int has, int ct, guint key)
{
  if (ct == 0)
    {
      ct++;
    }

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

    case 'n':
      markposition ('\'');
      search ("");
      break;
    case 'N':
      markposition ('\'');
      search ("", true);
      break;

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
      break;

    case 'R':
      reload ();
      break;
    }

  return MATCH;
}

bool ApvlvDir::enter (guint key)
{
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
          ndoc =
            new ApvlvDoc (mWidth, mHeight, gParams->values ("zoom"),
                          gParams->valueb ("cache"));
          if (!ndoc->loadfile (name))
            {
              delete ndoc;
              ndoc = NULL;
            }
        }
    }
  else
    {
      ndoc =
        new ApvlvDoc (mWidth, mHeight, gParams->values ("zoom"),
                      gParams->valueb ("cache"));
      if (!ndoc->loadfile (filename ()))
        {
          delete ndoc;
          ndoc = NULL;
        }

      if (ndoc != NULL)
        {
          ((ApvlvDoc *) ndoc)->showpage (pn);
        }
    }

  if (ndoc == NULL)
    {
      return false;
    }

  gView->regloaded (ndoc);

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

void ApvlvDir::scrollup (int times)
{
  GtkTreePath *path;

  if (!mReady
      || (path =
            gtk_tree_model_get_path (GTK_TREE_MODEL (mStore),
                                     &mCurrentIter)) == NULL)
    {
      return;
    }

  for (gboolean ret = TRUE; times > 0 && ret; times--)
    {
      ret = gtk_tree_path_prev (path);
    }

  gtk_tree_model_get_iter (GTK_TREE_MODEL (mStore), &mCurrentIter, path);
  gtk_tree_selection_select_iter (mSelection, &mCurrentIter);
  gtk_tree_view_scroll_to_cell (GTK_TREE_VIEW (mDirView), path, NULL, TRUE,
                                0.5, 0.0);
  gtk_tree_path_free (path);

  mStatus->show ();
}

void ApvlvDir::scrolldown (int times)
{
  if (!mReady)
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

  GtkTreePath *path =
    gtk_tree_model_get_path (GTK_TREE_MODEL (mStore), &mCurrentIter);
  if (path)
    {
      gtk_tree_view_scroll_to_cell (GTK_TREE_VIEW (mDirView), path, NULL,
                                    TRUE, 0.5, 0.0);
      gtk_tree_path_free (path);
    }

  mStatus->show ();
}

void ApvlvDir::scrollleft (int times)
{
  if (!mReady)
    return;

  GtkTreeIter itr;
  for (gboolean ret = TRUE; times > 0 && ret; times--)
    {
      ret =
        gtk_tree_model_iter_parent (GTK_TREE_MODEL (mStore), &itr,
                                    &mCurrentIter);
      if (ret)
        {
          mCurrentIter = itr;
        }
    }

  gtk_tree_selection_select_iter (mSelection, &mCurrentIter);

  GtkTreePath *path =
    gtk_tree_model_get_path (GTK_TREE_MODEL (mStore), &mCurrentIter);
  if (path)
    {
      gtk_tree_view_scroll_to_cell (GTK_TREE_VIEW (mDirView), path, NULL,
                                    TRUE, 0.5, 0.0);
      gtk_tree_view_collapse_row (GTK_TREE_VIEW (mDirView), path);
      gtk_tree_path_free (path);
    }

  mStatus->show ();
}

void ApvlvDir::scrollright (int times)
{
  if (!mReady)
    return;

  GtkTreeIter itr;
  for (gboolean ret = TRUE; times > 0 && ret; times--)
    {
      ret =
        gtk_tree_model_iter_children (GTK_TREE_MODEL (mStore), &itr,
                                      &mCurrentIter);
      if (ret)
        {
          mCurrentIter = itr;
        }
    }

  GtkTreePath *path =
    gtk_tree_model_get_path (GTK_TREE_MODEL (mStore), &mCurrentIter);
  if (path)
    {
      gtk_tree_view_expand_to_path (GTK_TREE_VIEW (mDirView), path);
      gtk_tree_selection_select_iter (mSelection, &mCurrentIter);
      gtk_tree_view_scroll_to_cell (GTK_TREE_VIEW (mDirView), path, NULL,
                                    TRUE, 0.5, 0.0);
      gtk_tree_path_free (path);
    }

  mStatus->show ();
}

bool ApvlvDir::search (const char *str, bool reverse)
{
  bool next;

  if (!mReady)
    return false;

  if (*str == '\0' && mSearchStr == "")
    {
      return false;
    }

  next = true;
  if (*str != '\0')
    {
      mSearchStr = str;
      next = false;
    }

  ApvlvDirNode *info = NULL;
  gtk_tree_model_get (GTK_TREE_MODEL (mStore), &mCurrentIter, 0, &info, -1);
  if (info == NULL || mDirNodes == NULL)
    {
      gView->errormessage ("can't find word: '%s'", mSearchStr.c_str ());
      return false;
    }

  GList *list;
  for (list = mDirNodes; list; list = g_list_next (list))
    {
      if (info == list->data)
        {
          break;
        }
    }

  if (list == NULL)
    {
      gView->errormessage ("can't find word: '%s'", mSearchStr.c_str ());
      return false;
    }

  if (next)
    {
      if (reverse)
        {
          list = g_list_previous (list);
        }
      else
        {
          list = g_list_next (list);
        }
    }

  bool wrap = gParams->valueb ("wrapscan");

  for (GList * origin = list; list != NULL;)
    {
      info = (ApvlvDirNode *) list->data;
      if (strstr (info->phrase (), mSearchStr.c_str ()) != NULL)
        {
          break;
        }

      if (reverse)
        {
          list = g_list_previous (list);
        }
      else
        {
          list = g_list_next (list);
        }

      if (list == origin)
        {
          list = NULL;
          break;
        }

      if (list == NULL && wrap)
        {
          if (reverse)
            {
              list = g_list_last (mDirNodes);
            }
          else
            {
              list = mDirNodes;
            }
        }
    }

  if (list == NULL)
    {
      gView->errormessage ("can't find word: '%s'", mSearchStr.c_str ());
      return false;
    }

  mCurrentIter = *info->iter ();
  GtkTreePath *path =
    gtk_tree_model_get_path (GTK_TREE_MODEL (mStore), &mCurrentIter);
  if (path)
    {
      gtk_tree_view_expand_to_path (GTK_TREE_VIEW (mDirView), path);
      gtk_tree_selection_select_iter (mSelection, &mCurrentIter);
      gtk_tree_view_scroll_to_cell (GTK_TREE_VIEW (mDirView), path, NULL,
                                    TRUE, 0.5, 0.0);
      gtk_tree_path_free (path);
    }

  mStatus->show ();
  return true;
}

void ApvlvDir::setactive (bool act)
{
  mStatus->active (act);
  mActive = act;
}

bool ApvlvDir::walk_file_index (GtkTreeIter * titr, ApvlvFileIndexIter iter)
{
  bool has = false;

  GtkTreeIter nitr[1];

  has = true;
  gtk_tree_store_append (mStore, nitr, titr);
  ApvlvDirNode *node = new ApvlvDirNode (this, nitr, iter->page);
  mDirNodes = g_list_append (mDirNodes, node);

  GdkPixbuf *pix = gdk_pixbuf_new_from_file_at_size (iconreg.c_str (), 40,
                   20, NULL);
  if (pix)
    {
      gtk_tree_store_set (mStore, nitr, 0, node, 1, pix, 2,
                          iter->title.c_str (), -1);
      g_object_unref (pix);
    }
  else
    {
      gtk_tree_store_set (mStore, nitr, 0, node, 2,
                          iter->title.c_str (), -1);
    }

  for (ApvlvFileIndexIter itr = iter->children.begin ();
       itr != iter->children.end (); ++itr)
    {
      bool chas = walk_file_index (has ? nitr : titr, itr);
      if (has == false)
        {
          has = chas;
        }
    }

  return has;
}

void
ApvlvDir::apvlv_dir_on_changed (GtkTreeSelection * selection,
                                ApvlvDir * dir)
{
  GtkTreeModel *model;
  gtk_tree_selection_get_selected (selection, &model, &dir->mCurrentIter);
}

ApvlvDirStatus::ApvlvDirStatus (ApvlvDir * doc)
{
  mDoc = doc;
  for (int i = 0; i < AD_STATUS_SIZE; ++i)
    {
      mStlab[i] = gtk_label_new ("");
      gtk_box_pack_start (GTK_BOX (mHbox), mStlab[i], FALSE, FALSE, 0);
    }
}

ApvlvDirStatus::~ApvlvDirStatus ()
{
}

void ApvlvDirStatus::active (bool act)
{
  for (unsigned int i = 0; i < AD_STATUS_SIZE; ++i)
    {
      gtk_widget_modify_fg (mStlab[i],
                            (act) ? GTK_STATE_ACTIVE :
                            GTK_STATE_INSENSITIVE, NULL);
    }
}

void ApvlvDirStatus::setsize (int w, int h)
{
  int sw[AD_STATUS_SIZE];
  sw[0] = w >> 1;
  sw[1] = sw[0] >> 1;
  sw[2] = sw[1] >> 1;
  sw[3] = sw[1] >> 1;
  for (unsigned int i = 0; i < AD_STATUS_SIZE; ++i)
    {
      gtk_widget_set_size_request (mStlab[i], sw[i], h);
    }
}

void ApvlvDirStatus::show ()
{
  if (mDoc->filename ())
    {
      char temp[AD_STATUS_SIZE][256];
      gchar *bn;
      bn = g_path_get_basename (mDoc->filename ());
      g_snprintf (temp[0], sizeof temp[0], "%s", bn);
      g_snprintf (temp[1], sizeof temp[1], "apvlv");
      g_snprintf (temp[2], sizeof temp[2], "%d%%",
                  (int) (mDoc->zoomvalue () * 100));
      g_snprintf (temp[3], sizeof temp[3], "%d%%",
                  (int) (mDoc->scrollrate () * 100));
      for (unsigned int i = 0; i < AD_STATUS_SIZE; ++i)
        {
          gtk_label_set_text (GTK_LABEL (mStlab[i]), temp[i]);
        }
      g_free (bn);
    }
}

gboolean ApvlvDir::apvlv_dir_first_select_cb (ApvlvDir * dir)
{
  GtkTreeIter gtir;
  if (gtk_tree_model_get_iter_first (GTK_TREE_MODEL (dir->mStore), &gtir))
    {
      gtk_tree_selection_select_iter (dir->mSelection, &gtir);
    }
  return FALSE;
}

void ApvlvDir::apvlv_dir_monitor_callback (GFileMonitor *gfm, GFile *gf1, GFile *gf2, GFileMonitorEvent ev, ApvlvDir *dir)
{
  if (ev == G_FILE_MONITOR_EVENT_CHANGED
      || ev == G_FILE_MONITOR_EVENT_DELETED
      || ev == G_FILE_MONITOR_EVENT_CREATED)
    {
      gView->errormessage ("Contents is modified, apvlv reload it automatically");
      dir->reload ();
    }
}

bool ApvlvDir::walk_dir_path_index (GtkTreeIter * itr, const char *path)
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

          gchar *realname = g_strjoin (PATH_SEP_S, path, name, NULL);
//          debug ("add a item: %s[%s]", name, realname);

          ApvlvDirNode *node = NULL;
          struct stat buf[1];
          char *wrealname =
            g_locale_from_utf8 (realname, -1, NULL, NULL, NULL);
          if (wrealname == NULL)
            {
              g_free (realname);
              continue;
            }

          int ret = stat (wrealname, buf);
          g_free (wrealname);

          if (ret < 0)
            {
              g_free (realname);
              continue;
            }

          if (S_ISDIR (buf->st_mode))
            {
              GtkTreeIter mitr[1];
              gtk_tree_store_append (mStore, mitr, itr);
              node = new ApvlvDirNode (this, mitr, true, realname, name);

              GdkPixbuf *pix =
                gdk_pixbuf_new_from_file_at_size (icondir.c_str (), 40, 20,
                                                  NULL);
              if (pix)
                {
                  gtk_tree_store_set (mStore, mitr, 0, node, 1, pix, 2,
                                      name, -1);
                  g_object_unref (pix);
                }
              else
                {
                  gtk_tree_store_set (mStore, mitr, 0, node, 2, name, -1);
                }

              if (!walk_dir_path_index (mitr, realname))
                {
                  gtk_tree_store_remove (mStore, mitr);
                  delete node;
                  node = NULL;
                }

              if (node != NULL)
                {
                  mDirNodes = g_list_append (mDirNodes, node);
                  has = true;
                }
            }
          else if (g_ascii_strncasecmp (name + strlen (name) - 4, ".pdf", 4)
                   == 0
#ifdef HAVE_LIBUMD
                   || g_ascii_strncasecmp (name + strlen (name) - 5,
                                           ".djvu", 5) == 0
                   || g_ascii_strncasecmp (name + strlen (name) - 4, ".djv",
                                           4) == 0
#endif
#ifdef HAVE_LIBUMD
                   || g_ascii_strncasecmp (name + strlen (name) - 4,
                                           ".umd", 4) == 0
#endif
                  )
            {
              GtkTreeIter mitr[1];
              gtk_tree_store_append (mStore, mitr, itr);
              node = new ApvlvDirNode (this, mitr, false, realname, name);
              mDirNodes = g_list_append (mDirNodes, node);

              GdkPixbuf *pix =
                gdk_pixbuf_new_from_file_at_size (iconpdf.c_str (), 40, 20,
                                                  NULL);
              if (pix)
                {
                  gtk_tree_store_set (mStore, mitr, 0, node, 1, pix, 2,
                                      name, -1);
                  g_object_unref (pix);
                }
              else
                {
                  gtk_tree_store_set (mStore, mitr, 0, node, 2, name, -1);
                }

              has = true;
            }

          g_free (realname);
        }
    }
  g_dir_close (dir);

  return has;
}
}

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
/* @CPPFILE ApvlvView.cc
 *
 *  Author: Alf <naihe2010@126.com>
 */
/* @date Created: 2008/09/30 00:00:00 Alf */

#include "ApvlvView.h"
#include "ApvlvInfo.h"
#include "ApvlvMenuAndTool.h"
#include "ApvlvParams.h"
#include <fstream>
#include <gdk/gdk.h>
#include <gdk/gdkkeysyms.h>
#include <gio/gio.h>
#include <glib.h>
#include <gtk/gtk.h>
#include <iostream>
#include <sstream>

namespace apvlv
{
ApvlvView::ApvlvView (ApvlvView *parent) : mCurrTabPos (-1), mCmds (this)
{
  mParent = parent;
  if (mParent)
    {
      mParent->append_child (this);
    }

  mCmdType = CMD_NONE;

  mProCmd = 0;
  mProCmdCnt = 0;

  mCurrHistroy = -1;

  mHasFull = FALSE;

  keyLastEnd = true;

  processInLast = false;

  mMainWindow = gtk_window_new (GTK_WINDOW_TOPLEVEL);

  int w = gParams->valuei ("width");
  int h = gParams->valuei ("height");

  if (gParams->valueb ("fullscreen"))
    {
#if GTK_CHECK_VERSION(3, 22, 0)
      GdkRectangle rect[1];
      GdkDisplay *display = gdk_display_get_default ();
      GdkMonitor *monitor = gdk_display_get_primary_monitor (display);
      gdk_monitor_get_geometry (monitor, rect);
#else
      GdkScreen *scr = gdk_screen_get_default ();
      mWidth = gdk_screen_get_width (scr);
      mHeight = gdk_screen_get_height (scr);
#endif
      fullscreen ();
    }
  else
    {
      gtk_window_set_default_size (GTK_WINDOW (mMainWindow), w > 1 ? w : 800,
                                   h > 1 ? h : 600);
    }

  mViewBox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_container_add (GTK_CONTAINER (mMainWindow), mViewBox);

  mMenu = new ApvlvMenuAndTool (this);

  if (strchr (gParams->values ("guioptions"), 'm') != nullptr)
    {
      gtk_box_pack_start (GTK_BOX (mViewBox), mMenu->menubar (), FALSE, FALSE,
                          0);
    }
  if (strchr (gParams->values ("guioptions"), 'T') != nullptr)
    {
      gtk_box_pack_start (GTK_BOX (mViewBox), mMenu->toolbar (), FALSE, FALSE,
                          0);
    }

  mTabContainer = gtk_notebook_new ();
  gtk_notebook_set_show_tabs (GTK_NOTEBOOK (mTabContainer), FALSE);
  gtk_notebook_set_scrollable (GTK_NOTEBOOK (mTabContainer), TRUE);
  gtk_box_pack_start (GTK_BOX (mViewBox), mTabContainer, TRUE, TRUE, 0);

  mCommandBar = gtk_entry_new ();
  gtk_box_pack_end (GTK_BOX (mViewBox), mCommandBar, FALSE, FALSE, 0);

  g_signal_connect (G_OBJECT (mMainWindow), "key-press-event",
                    G_CALLBACK (apvlv_view_keypress_cb), this);

  g_signal_connect (G_OBJECT (mMainWindow), "delete-event",
                    G_CALLBACK (apvlv_view_delete_cb), this);
  g_signal_connect (G_OBJECT (mMainWindow), "destroy-event",
                    G_CALLBACK (apvlv_view_delete_cb), this);

  g_signal_connect (G_OBJECT (mTabContainer), "switch-page",
                    G_CALLBACK (apvlv_notebook_switch_cb), this);

  g_signal_connect (G_OBJECT (mCommandBar), "key-press-event",
                    G_CALLBACK (apvlv_view_commandbar_cb), this);

  apvlv_widget_set_background (mMainWindow);

  gtk_widget_show_all (mMainWindow);

  cmd_hide ();
}

ApvlvView::~ApvlvView ()
{
  if (mParent)
    {
      mParent->erase_child (this);
    }

  auto itr = mChildren.begin ();
  while (itr != mChildren.end ())
    {
      delete *itr;
      itr = mChildren.begin ();
    }

  size_t i;

  delete mMenu;

  for (auto &t : mTabList)
    {
      delete t.mRootWindow;
    }
  mTabList.clear ();

  mCmdHistroy.clear ();

  for (i = 0; i < mDocs.size (); ++i)
    {
      auto *core = (ApvlvCore *)mDocs[i];
      delete core;
    }
}

GtkWidget *
ApvlvView::widget ()
{
  return mMainWindow;
}

ApvlvWindow *
ApvlvView::currentWindow ()
{
  if (mCurrTabPos == -1)
    return nullptr;

  return mTabList[mCurrTabPos].mRootWindow->activeCoreWindow ();
}

void
ApvlvView::delcurrentWindow ()
{
  mTabList[mCurrTabPos].mRootWindow = currentWindow ()->unbirth ();
  mTabList[mCurrTabPos].mWindowCount--;
  updatetabname ();
}

void
ApvlvView::open ()
{
  gchar *dirname;

  GtkWidget *dia = gtk_file_chooser_dialog_new (
      "Open ...", GTK_WINDOW (mMainWindow), GTK_FILE_CHOOSER_ACTION_SAVE,
      ("_Cancel"), GTK_RESPONSE_CANCEL, ("_OK"), GTK_RESPONSE_ACCEPT, nullptr);
  infofile *fp = gInfo->file (0);
  dirname = fp ? g_path_get_dirname (fp->file.c_str ())
               : g_strdup (gParams->values ("defaultdir"));
  debug ("lastfile: [%s], dirname: [%s]", fp ? fp->file.c_str () : "",
         dirname);
  gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (dia), dirname);
  g_free (dirname);

  GtkFileFilter *filter = gtk_file_filter_new ();
  gtk_file_filter_add_mime_type (filter, "PDF File");
  gtk_file_filter_add_pattern (filter, "*.pdf");
  gtk_file_filter_add_pattern (filter, "*.PDF");
  gtk_file_filter_add_mime_type (filter, "HTML File");
  gtk_file_filter_add_pattern (filter, "*.HTM");
  gtk_file_filter_add_pattern (filter, "*.htm");
  gtk_file_filter_add_pattern (filter, "*.HTML");
  gtk_file_filter_add_pattern (filter, "*.html");
  gtk_file_filter_add_mime_type (filter, "ePub File");
  gtk_file_filter_add_pattern (filter, "*.EPUB");
  gtk_file_filter_add_pattern (filter, "*.epub");
#ifdef APVLV_WITH_DJVU
  gtk_file_filter_add_mime_type (filter, "DJVU File");
  gtk_file_filter_add_pattern (filter, "*.DJV");
  gtk_file_filter_add_pattern (filter, "*.djv");
  gtk_file_filter_add_pattern (filter, "*.DJVU");
  gtk_file_filter_add_pattern (filter, "*.djvu");
#endif
  gtk_file_filter_add_mime_type (filter, "TXT File");
  gtk_file_filter_add_pattern (filter, "*.TXT");
  gtk_file_filter_add_pattern (filter, "*.txt");
  gtk_file_filter_add_mime_type (filter, "FB2 File");
  gtk_file_filter_add_pattern (filter, "*.FB2");
  gtk_file_filter_add_pattern (filter, "*.fb2");
  gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (dia), filter);

  gint ret = gtk_dialog_run (GTK_DIALOG (dia));
  if (ret == GTK_RESPONSE_ACCEPT)
    {
      gchar *filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dia));
      loadfile (filename);
      g_free (filename);
    }
  gtk_widget_destroy (dia);
}

void
ApvlvView::opendir ()
{
  gchar *dirname;

  GtkWidget *dia = gtk_file_chooser_dialog_new (
      "", GTK_WINDOW (mMainWindow), GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
      ("_Cancel"), GTK_RESPONSE_CANCEL, ("_OK"), GTK_RESPONSE_ACCEPT, nullptr);
  infofile *fp = gInfo->file (0);
  dirname = fp ? g_path_get_dirname (fp->file.c_str ())
               : g_strdup (gParams->values ("defaultdir"));
  gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (dia), dirname);
  g_free (dirname);

  gint ret = gtk_dialog_run (GTK_DIALOG (dia));
  if (ret == GTK_RESPONSE_ACCEPT)
    {
      gchar *filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dia));

      loaddir (filename);
      g_free (filename);
    }
  gtk_widget_destroy (dia);
}

bool
ApvlvView::loaddir (const char *path)
{
  crtadoc ()->setDirIndex (path);
  return true;
}

bool
ApvlvView::loadfile (const string &file)
{
  return loadfile (file.c_str ());
}

void
ApvlvView::quit ()
{
  if (int (mTabList.size ()) == 1)
    {
      mTabList.clear ();
      apvlv_view_delete_cb (nullptr, nullptr, this);
      return;
    }

  auto p = mCurrTabPos;
  if (mCurrTabPos < int (mTabList.size ()) - 1)
    switch_tabcontext (mCurrTabPos + 1);
  else
    switch_tabcontext (mCurrTabPos - 1);

  gtk_notebook_set_current_page (GTK_NOTEBOOK (mTabContainer),
                                 (gint)mCurrTabPos);
  gtk_notebook_remove_page (GTK_NOTEBOOK (mTabContainer), (gint)p);
  delete_tabcontext (p);

  if (mCurrTabPos > p)
    --mCurrTabPos;
}

bool
ApvlvView::newtab (const char *filename, bool disable_content)
{
  ApvlvCore *ndoc = hasloaded (filename);

  if (ndoc == nullptr)
    {
      ndoc = new ApvlvDoc (this, gParams->values ("zoom"), false);
      if (!ndoc->loadfile (filename, true, gParams->valueb ("content")))
        {
          delete ndoc;
          ndoc = nullptr;
        }

      if (ndoc)
        {
          regloaded (ndoc);
        }
    }

  if (ndoc)
    {
      newtab (ndoc);
      return true;
    }
  else
    {
      return false;
    }
}

bool
ApvlvView::newtab (ApvlvCore *core)
{
  auto pos = new_tabcontext (core);

  switch_tabcontext (pos);

  gchar *base = core->filename () ? g_path_get_basename (core->filename ())
                                  : g_strdup ("NONE");
  GtkWidget *tabname = gtk_label_new (base);
  g_free (base);

  auto *parentbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_box_pack_start (GTK_BOX (parentbox),
                      mTabList[mCurrTabPos].mRootWindow->widget (), TRUE, TRUE,
                      0);

  gtk_notebook_insert_page (GTK_NOTEBOOK (mTabContainer), parentbox, tabname,
                            gint (pos));

  gtk_widget_show_all (parentbox);
  gtk_notebook_set_current_page (GTK_NOTEBOOK (mTabContainer),
                                 (gint)mCurrTabPos);
  return true;
}

__attribute__ ((unused)) bool
ApvlvView::newview (const gchar *filename, gint pn, bool disable_content)
{
  auto *view = new ApvlvView (this);
  view->newtab (filename, disable_content);
  view->crtadoc ()->showpage (pn, 0.0);
  return TRUE;
}

long
ApvlvView::new_tabcontext (ApvlvCore *core)
{
  auto *troot = new ApvlvWindow (core, this);
  TabEntry context (troot, 1);
  if (mCurrTabPos == -1)
    {
      mTabList.push_back (context);
      mCurrTabPos = 0;
      return 0;
    }

  auto insPos = mTabList.begin () + mCurrTabPos + 1;
  mTabList.insert (insPos, context);

  if (mTabList.size () > 1)
    {
      gtk_notebook_set_show_tabs (GTK_NOTEBOOK (mTabContainer), TRUE);
    }

  return ++mCurrTabPos;
}

void
ApvlvView::delete_tabcontext (long tabPos)
{
  auto remPos = mTabList.begin () + tabPos;

  if (remPos->mRootWindow != nullptr)
    {
      delete remPos->mRootWindow;
      remPos->mRootWindow = nullptr;
    }

  mTabList.erase (remPos);

  if (int (mTabList.size ()) <= 1)
    {
      gtk_notebook_set_show_tabs (GTK_NOTEBOOK (mTabContainer), FALSE);
    }
}

void
ApvlvView::switch_tabcontext (long tabPos)
{
  mCurrTabPos = tabPos;
}

bool
ApvlvView::loadfile (const char *filename)
{
  if (filename == nullptr || *filename == '\0')
    {
      return false;
    }

  char *abpath = absolutepath (filename);
  if (abpath == nullptr)
    {
      return false;
    }

  ApvlvWindow *win = currentWindow ();
  ApvlvCore *ndoc;

  ndoc = hasloaded (abpath);

  if (ndoc == nullptr)
    {
      ndoc = new ApvlvDoc (this, gParams->values ("zoom"), false);
      if (!ndoc->loadfile (filename, true, gParams->valueb ("content")))
        {
          delete ndoc;
          ndoc = nullptr;
        }

      if (ndoc != nullptr)
        {
          regloaded (ndoc);
        }
    }

  if (ndoc != nullptr)
    {
      win->setCore (ndoc);
      updatetabname ();
    }
  g_free (abpath);

  return ndoc != nullptr;
}

ApvlvCore *
ApvlvView::hasloaded (const char *abpath)
{
  ApvlvCore *core;
  size_t i;
  for (i = 0; i < mDocs.size (); ++i)
    {
      core = (ApvlvCore *)mDocs[i];
      if (!core->inuse () && strcmp (core->filename (), abpath) == 0)
        {
          return core;
        }
    }
  return nullptr;
}

void
ApvlvView::regloaded (ApvlvCore *core)
{
  if (gParams->valuei ("pdfcache") < 2)
    {
      gParams->push ("pdfcache", "2");
    }

  if (mDocs.size () >= (size_t)gParams->valuei ("pdfcache"))
    {
      auto itr = mDocs.begin ();
      debug ("to pdf cache size: %d, remove first: %p\n",
             gParams->valuei ("pdfcache"), *itr);
      if ((*itr)->inuse () == false)
        {
          delete *itr;
        }
      mDocs.erase (itr);
    }

  mDocs.push_back (core);
}

ApvlvCompletion *
ApvlvView::filecompleteinit (const char *path)
{
  auto *comp = new ApvlvCompletion;
  GList *list = g_list_alloc ();
  gchar *dname, *bname;
  const gchar *name;

  dname = g_path_get_dirname (path);
  GDir *dir = g_dir_open ((const char *)dname, 0, nullptr);
  if (dir != nullptr)
    {
      bname = g_path_get_basename (path);
      size_t len = strlen (bname);
      while ((name = g_dir_read_name (dir)) != nullptr)
        {
          gchar *fname
              = g_locale_from_utf8 (name, -1, nullptr, nullptr, nullptr);

          if (strcmp (bname, PATH_SEP_S) != 0)
            {
              if (strncmp (fname, bname, len) != 0)
                {
                  g_free (fname);
                  continue;
                }
            }

          if (strcmp (dname, ".") == 0)
            {
              list->data = g_strdup (fname);
            }
          else
            {
              if (dname[strlen (dname) - 1] == PATH_SEP_C)
                {
                  list->data = g_strjoin ("", dname, fname, nullptr);
                }
              else
                {
                  list->data = g_strjoin (PATH_SEP_S, dname, fname, nullptr);
                }
            }

          g_free (fname);

          debug ("add a item: %s", (char *)list->data);
          comp->add_items (list);
        }
      g_free (bname);
      g_dir_close (dir);
    }
  g_free (dname);

  g_list_free (list);

  return comp;
}

void
ApvlvView::promptcommand (char ch)
{
  char s[2] = { 0 };
  s[0] = ch;
  gtk_entry_set_text (GTK_ENTRY (mCommandBar), s);
  cmd_show (CMD_CMD);
}

void
ApvlvView::promptcommand (const char *s)
{
  gtk_entry_set_text (GTK_ENTRY (mCommandBar), s);
  cmd_show (CMD_CMD);
}

void
ApvlvView::errormessage (const char *str, ...)
{
  gchar estr[512];
  gint pos;
  va_list vap;
  va_start (vap, str);
  vsnprintf (estr, sizeof estr, str, vap);
  va_end (vap);
  gtk_entry_set_text (GTK_ENTRY (mCommandBar), "ERROR: ");
  gtk_editable_set_position (GTK_EDITABLE (mCommandBar), -1);
  pos = gtk_editable_get_position (GTK_EDITABLE (mCommandBar));
  gtk_editable_insert_text (GTK_EDITABLE (mCommandBar), estr, -1, &pos);
  cmd_show (CMD_MESSAGE);
}

void
ApvlvView::infomessage (const char *str, ...)
{
  gchar estr[512];
  gint pos;
  va_list vap;
  va_start (vap, str);
  vsnprintf (estr, sizeof estr, str, vap);
  va_end (vap);
  gtk_entry_set_text (GTK_ENTRY (mCommandBar), "INFO: ");
  gtk_editable_set_position (GTK_EDITABLE (mCommandBar), -1);
  pos = gtk_editable_get_position (GTK_EDITABLE (mCommandBar));
  gtk_editable_insert_text (GTK_EDITABLE (mCommandBar), estr, -1, &pos);
  cmd_show (CMD_MESSAGE);
}

gchar *
ApvlvView::input (const char *str, int width, int height, string content)
{
  auto flags = GTK_DIALOG_DESTROY_WITH_PARENT;
  auto dialog = gtk_dialog_new_with_buttons (str, nullptr, flags, "OK",
                                             GTK_RESPONSE_ACCEPT, "Cancel",
                                             GTK_RESPONSE_CANCEL, NULL);
  auto content_area = gtk_dialog_get_content_area (GTK_DIALOG (dialog));
  auto entry = gtk_text_view_new ();
  gtk_widget_set_size_request (entry, width, height);
  gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (entry), GTK_WRAP_WORD_CHAR);
  gtk_text_buffer_set_text (gtk_text_view_get_buffer (GTK_TEXT_VIEW (entry)),
                            content.c_str (), content.size ());
  gtk_container_add (GTK_CONTAINER (content_area), entry);
  gtk_widget_show_all (content_area);

  auto provider = gtk_css_provider_new ();
  gtk_css_provider_load_from_data (provider,
                                   "textview {"
                                   " font: 20 Sans;"
                                   " color: red;"
                                   "}",
                                   -1, NULL);
  auto context = gtk_widget_get_style_context (entry);
  gtk_style_context_add_provider (context, GTK_STYLE_PROVIDER (provider),
                                  GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

  gchar *response = nullptr;
  auto result = gtk_dialog_run (GTK_DIALOG (dialog));
  if (result == GTK_RESPONSE_ACCEPT)
    {
      auto buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (entry));
      GtkTextIter start, end;
      gtk_text_buffer_get_start_iter (buffer, &start);
      gtk_text_buffer_get_end_iter (buffer, &end);
      response = gtk_text_buffer_get_text (buffer, &start, &end, FALSE);
    }
  gtk_widget_destroy (dialog);
  return response;
}

void
ApvlvView::cmd_show (int cmdtype)
{
  if (mMainWindow == nullptr)
    return;

  mCmdType = cmdtype;

  gtk_widget_show (mCommandBar);
  gtk_widget_grab_focus (mCommandBar);
  gtk_editable_set_position (GTK_EDITABLE (mCommandBar), -1);
}

void
ApvlvView::cmd_hide ()
{
  if (mMainWindow == nullptr)
    return;
  mCmdType = CMD_NONE;

  gtk_widget_hide (mCommandBar);

  gtk_widget_grab_focus (mMainWindow);
}

void
ApvlvView::cmd_auto (const char *ps)
{
  ApvlvCompletion *comp = nullptr;

  stringstream ss (ps);
  string cmd, np, argu;
  ss >> cmd >> np >> argu;

  if (np[np.length () - 1] == '\\')
    {
      np.replace (np.length () - 1, 1, 1, ' ');
      np += argu;
      ss >> argu;
    }

  if (cmd.empty () || np.empty ())
    {
      return;
    }

  if (cmd == "o" || cmd == "open" || cmd == "TOtext")
    {
      comp = filecompleteinit (np.c_str ());
    }
  else if (cmd == "doc")
    {
      comp = new ApvlvCompletion;
      GList *list = g_list_alloc ();
      size_t i;
      for (i = 0; i < mDocs.size (); ++i)
        {
          list->data = g_strdup (((ApvlvCore *)mDocs[i])->filename ());
          comp->add_items (list);
        }
      g_list_free (list);
    }

  if (comp != nullptr)
    {
      char *comtext = nullptr;
      debug ("find match: %s", np.c_str ());
      comp->complete (np.c_str (), &comtext);
      if (comtext != nullptr)
        {
          char text[PATH_MAX];

          debug ("get a match: %s", comtext);
          gchar **v;

          v = g_strsplit (comtext, " ", -1);
          if (v != nullptr)
            {
              gchar *comstr = g_strjoinv ("\\ ", v);
              g_snprintf (text, sizeof text, ":%s %s", cmd.c_str (), comstr);
              g_free (comstr);
              g_strfreev (v);
            }
          else
            {
              g_snprintf (text, sizeof text, ":%s %s", cmd.c_str (), comtext);
            }
          gtk_entry_set_text (GTK_ENTRY (mCommandBar), text);
          gtk_editable_set_position (GTK_EDITABLE (mCommandBar), -1);
          g_free (comtext);
        }
      else
        {
          debug ("no get match");
        }

      delete comp;
    }
}

void
ApvlvView::fullscreen ()
{
  if (mHasFull == false)
    {
      ApvlvCore *core = crtadoc ();
      if (core)
        core->toggleContent (false);

      gtk_window_fullscreen (GTK_WINDOW (mMainWindow));
      mHasFull = true;
    }
  else
    {
      gtk_window_unfullscreen (GTK_WINDOW (mMainWindow));
      mHasFull = false;
    }
}

ApvlvCore *
ApvlvView::crtadoc ()
{
  ApvlvWindow *curwin = currentWindow ();
  if (curwin == nullptr)
    return nullptr;

  return curwin->getCore ();
}

returnType
ApvlvView::subprocess (int ct, guint key)
{
  guint procmd = mProCmd;
  mProCmd = 0;
  mProCmdCnt = 0;
  switch (procmd)
    {
    case 'Z':
      if (key == 'Z')
        quit ();

    case CTRL ('w'):
      if (key == 'q' || key == CTRL ('Q'))
        {
          if (currentWindow ()->isAncestor ())
            {
              quit ();
            }
          else
            {
              delcurrentWindow ();
            }
        }
      else
        {
          returnType rv = currentWindow ()->process (ct, key);
          updatetabname ();
          return rv;
        }
      break;

    case 'g':
      if (key == 't')
        {
          if (ct == 0)
            switchtab (mCurrTabPos + 1);
          else
            switchtab (ct - 1);
        }
      else if (key == 'T')
        {
          if (ct == 0)
            switchtab (mCurrTabPos - 1);
          else
            switchtab (ct - 1);
        }
      else if (key == 'g')
        {
          if (ct == 0)
            ct = 1;
          crtadoc ()->showpage (ct - 1, 0.0);
        }
      break;

    default:
      return NO_MATCH;
    }

  return MATCH;
}

returnType
ApvlvView::process (int has, int ct, guint key)
{
  if (mProCmd != 0)
    {
      returnType ret = subprocess (mProCmdCnt, key);
      if (ret == MATCH)
        {
          saveKey (has, ct, key, true);
        }
      return ret;
    }

  switch (key)
    {
    case 'Z':
      mProCmd = 'Z';
      return NEED_MORE;

    case CTRL ('w'):
      mProCmd = CTRL ('w');
      mProCmdCnt = has ? ct : 1;
      saveKey (has, ct, key, false);
      return NEED_MORE;
    case 'q':
      quit ();
      break;
    case 'f':
      fullscreen ();
      break;
    case '.':
      processLastKey ();
      break;
    case 'g':
      mProCmd = 'g';
      mProCmdCnt = has ? ct : 0;
      saveKey (has, ct, key, false);
      return NEED_MORE;
    default:
      returnType ret = crtadoc ()->process (has, ct, key);
      if (ret == NEED_MORE)
        {
          saveKey (has, ct, key, false);
        }
      else if (ret == MATCH)
        {
          saveKey (has, ct, key, true);
        }
    }

  return MATCH;
}

bool
ApvlvView::run (const char *str)
{
  bool ret;

  switch (*str)
    {
    case SEARCH:
      crtadoc ()->markposition ('\'');
      ret = crtadoc ()->search (str + 1, false);
      break;

    case BACKSEARCH:
      crtadoc ()->markposition ('\'');
      ret = crtadoc ()->search (str + 1, true);
      break;

    case COMMANDMODE:
      ret = runcmd (str + 1);
      break;

    case FIND:
      ret = crtadoc ()->find (str + 1);
      break;

    default:
      ret = false;
      break;
    }

  return ret;
}

void
ApvlvView::settitle (const char *title)
{
  gtk_window_set_title (GTK_WINDOW (mMainWindow), title);
}

bool
ApvlvView::runcmd (const char *str)
{
  bool ret = true;

  if (*str == '!')
    {
      apvlv_system (str + 1);
    }
  else
    {
      stringstream ss (str);
      string cmd, subcmd, argu;
      ss >> cmd >> subcmd >> argu;

      if (subcmd.length () > 0 && subcmd[subcmd.length () - 1] == '\\')
        {
          subcmd.replace (subcmd.length () - 1, 1, 1, ' ');
          subcmd += argu;
          ss >> argu;
        }

      if (cmd == "set")
        {
          if (subcmd == "cache")
            {
              gParams->push ("cache", "yes");
              crtadoc ()->usecache (true);
            }
          else if (subcmd == "nocache")
            {
              gParams->push ("cache", "no");
              crtadoc ()->usecache (false);
            }
          else if (subcmd == "skip")
            {
              crtadoc ()->setskip (int (strtol (argu.c_str (), nullptr, 10)));
            }
          else
            {
              gParams->push (subcmd, argu);
            }
        }
      else if (cmd == "map" && !subcmd.empty ())
        {
          apvlv::ApvlvCmds::buildmap (subcmd.c_str (), argu.c_str ());
        }
      else if ((cmd == "o" || cmd == "open" || cmd == "doc")
               && !subcmd.empty ())
        {
          char *home;

          if (subcmd[0] == '~')
            {
              home = getenv ("HOME");

              if (home)
                {
                  subcmd.replace (0, 1, home);
                }
            }

          if (g_file_test (subcmd.c_str (), G_FILE_TEST_IS_DIR))
            {
              ret = loaddir (subcmd.c_str ());
            }
          else if (g_file_test (subcmd.c_str (), G_FILE_TEST_EXISTS))
            {
              ret = loadfile (subcmd.c_str ());
            }
          else
            {
              errormessage ("no file '%s'", subcmd.c_str ());
              ret = false;
            }
        }
      else if (cmd == "TOtext" && !subcmd.empty ())
        {
          crtadoc ()->totext (subcmd.c_str ());
        }
      else if ((cmd == "pr" || cmd == "print"))
        {
          crtadoc ()->print (
              subcmd.empty () ? -1
                              : int (strtol (subcmd.c_str (), nullptr, 10)));
        }
      else if (cmd == "sp")
        {
          currentWindow ()->birth (ApvlvWindow::AW_SP, nullptr);
          windowadded ();
        }
      else if (cmd == "vsp")
        {
          currentWindow ()->birth (ApvlvWindow::AW_VSP, nullptr);
          windowadded ();
        }
      else if ((cmd == "zoom" || cmd == "z") && !subcmd.empty ())
        {
          crtadoc ()->setzoom (subcmd.c_str ());
        }
      else if (cmd == "forwardpage" || cmd == "fp")
        {
          if (subcmd.empty ())
            crtadoc ()->nextpage (1);
          else
            crtadoc ()->nextpage (int (strtol (subcmd.c_str (), nullptr, 10)));
        }
      else if (cmd == "prewardpage" || cmd == "bp")
        {
          if (subcmd.empty ())
            crtadoc ()->prepage (1);
          else
            crtadoc ()->prepage (int (strtol (subcmd.c_str (), nullptr, 10)));
        }
      else if (cmd == "content")
        {
          crtadoc ()->toggleContent ();
        }
      else if (cmd == "goto" || cmd == "g")
        {
          crtadoc ()->markposition ('\'');
          auto p = strtol (subcmd.c_str (), nullptr, 10);
          p += crtadoc ()->getskip ();
          crtadoc ()->showpage (int (p - 1), 0.0);
        }
      else if (cmd == "help" || cmd == "h")
        {
          loadfile (helppdf);
        }
      else if (cmd == "q" || cmd == "quit")
        {
          if (currentWindow ()->isAncestor ())
            {
              quit ();
            }
          else
            {
              delcurrentWindow ();
            }
        }
      else if (cmd == "qall")
        {
          quit ();
        }
      else if (cmd == "tabnew")
        {
          newtab (helppdf.c_str ());
        }
      else if (cmd == "tabn" || cmd == "tabnext")
        {
          switchtab (mCurrTabPos + 1);
        }
      else if (cmd == "tabp" || cmd == "tabprevious")
        {
          switchtab (mCurrTabPos - 1);
        }
      else if (cmd == "w" || cmd == "write")
        {
          crtadoc ()->writefile (!subcmd.empty () ? subcmd.c_str () : nullptr);
        }
      else if (cmd == "toc")
        {
          loadfile (crtadoc ()->filename ());
        }
      else
        {
          bool isn = true;
          for (char i : cmd)
            {
              if (i < '0' || i > '9')
                {
                  isn = false;
                  break;
                }
            }
          if (isn && crtadoc ())
            {
              auto p = strtol (cmd.c_str (), nullptr, 10);
              p += crtadoc ()->getskip ();
              if (p != crtadoc ()->pagenumber ())
                {
                  crtadoc ()->showpage (int (p - 1), 0.0);
                }
            }
          else
            {
              errormessage ("no command: '%s'", cmd.c_str ());
              ret = false;
            }
        }
    }

  return ret;
}

gint
ApvlvView::apvlv_view_keypress_cb (__attribute__ ((unused)) GtkWidget *wid,
                                   GdkEvent *ev, ApvlvView *view)
{
  if (view->mCmdType == CMD_NONE)
    {
      view->mCmds.append ((GdkEventKey *)ev);
      return TRUE;
    }

  return FALSE;
}

bool
isalt_escape (GdkEventKey *event)
{
  /* Grab the default modifier mask... */
  guint modifiers = gtk_accelerator_get_default_mod_mask ();

  if (event->keyval == GDK_KEY_bracketleft &&
      /* ...so we can ignore mod keys like numlock and capslock. */
      (event->state & modifiers) == GDK_CONTROL_MASK)
    {
      return true;
    }

  return false;
}

gint
ApvlvView::apvlv_view_commandbar_cb (__attribute__ ((unused)) GtkWidget *wid,
                                     GdkEvent *ev, ApvlvView *view)
{
  if (view->mCmdType == CMD_CMD)
    {
      auto *gek = (GdkEventKey *)ev;
      if (gek->keyval == GDK_KEY_Return)
        {
          auto *str
              = (gchar *)gtk_entry_get_text (GTK_ENTRY (view->mCommandBar));
          if (str && strlen (str) > 0)
            {
              if (view->run (str))
                {
                  view->mCmdHistroy.emplace_back (str);
                  view->mCurrHistroy = view->mCmdHistroy.size () - 1;
                  view->cmd_hide ();
                  return TRUE;
                }
              else
                {
                  debug ("");
                  return TRUE;
                }
            }
          else
            {
              view->cmd_hide ();
              return TRUE;
            }
        }
      else if (gek->keyval == GDK_KEY_Tab)
        {
          auto *str
              = (gchar *)gtk_entry_get_text (GTK_ENTRY (view->mCommandBar));
          if (str && strlen (str) > 0)
            {
              view->cmd_auto (str + 1);
            }
          return TRUE;
        }
      else if (gek->keyval == GDK_KEY_BackSpace)
        {
          auto *str
              = (gchar *)gtk_entry_get_text (GTK_ENTRY (view->mCommandBar));
          if (str == nullptr || strlen (str) == 1)
            {
              view->cmd_hide ();
              view->mCurrHistroy = view->mCmdHistroy.size () - 1;
              return TRUE;
            }
        }
      else if (gek->keyval == GDK_KEY_Escape || isalt_escape (gek))
        {
          view->cmd_hide ();
          view->mCurrHistroy = view->mCmdHistroy.size () - 1;
          return TRUE;
        }
      else if (gek->keyval == GDK_KEY_Up)
        {
          if (view->mCmdHistroy.empty ())
            {
              return TRUE;
            }

          gtk_entry_set_text (
              GTK_ENTRY (view->mCommandBar),
              view->mCurrHistroy > 0
                  ? view->mCmdHistroy[view->mCurrHistroy--].c_str ()
                  : view->mCmdHistroy[0].c_str ());
          return TRUE;
        }
      else if (gek->keyval == GDK_KEY_Down)
        {
          if (view->mCmdHistroy.empty ())
            {
              return TRUE;
            }

          gtk_entry_set_text (
              GTK_ENTRY (view->mCommandBar),
              (size_t)view->mCurrHistroy < view->mCmdHistroy.size () - 1
                  ? view->mCmdHistroy[++view->mCurrHistroy].c_str ()
                  : view->mCmdHistroy[view->mCmdHistroy.size () - 1].c_str ());
          return TRUE;
        }

      return FALSE;
    }
  else if (view->mCmdType == CMD_MESSAGE)
    {
      debug ("");
      view->cmd_hide ();
      return TRUE;
    }

  return FALSE;
}

void
ApvlvView::apvlv_view_delete_cb (__attribute__ ((unused)) GtkWidget *wid,
                                 __attribute__ ((unused)) GtkAllocation *al,
                                 ApvlvView *view)
{
  debug ("delete cb: %p", view);
  GtkWidget *widget = view->mMainWindow;
  if (widget)
    {
      gtk_widget_destroy (widget);
    }
  view->mMainWindow = nullptr;
  if (view->mParent == nullptr)
    {
      gtk_main_quit ();
    }
}

void
ApvlvView::apvlv_notebook_switch_cb (__attribute__ ((unused)) GtkWidget *wid,
                                     __attribute__ ((unused))
                                     GtkNotebook *notebook,
                                     guint pnum, ApvlvView *view)
{
  view->mCurrTabPos = pnum;

  if (view->crtadoc () && view->crtadoc ()->filename ())
    {
      gchar *base = g_path_get_basename (view->crtadoc ()->filename ());
      view->settitle (base);
      g_free (base);
    }
}

void
ApvlvView::switchtab (long tabPos)
{
  int ntabs = int (mTabList.size ());
  while (tabPos < 0)
    tabPos += ntabs;

  tabPos = tabPos % ntabs;
  switch_tabcontext (tabPos);
  gtk_notebook_set_current_page (GTK_NOTEBOOK (mTabContainer),
                                 gint (mCurrTabPos));
}

void
ApvlvView::windowadded ()
{
  mTabList[mCurrTabPos].mWindowCount++;
  updatetabname ();
}

void
ApvlvView::updatetabname ()
{
  char tagname[26];

  const char *filename = currentWindow ()->getCore ()->filename ();
  gchar *gfilename;

  if (filename == nullptr)
    gfilename = g_strdup ("None");
  else
    gfilename = g_path_get_basename (filename);

  settitle (gfilename);

  if (mTabList[mCurrTabPos].mWindowCount > 1)
    g_snprintf (tagname, sizeof tagname, "[%d] %s",
                mTabList[mCurrTabPos].mWindowCount, gfilename);
  else
    g_snprintf (tagname, sizeof tagname, "%s", gfilename);

  g_free (gfilename);

  GtkWidget *tabname = gtk_label_new (tagname);
  GtkWidget *tabwidget = gtk_notebook_get_nth_page (
      GTK_NOTEBOOK (mTabContainer), gint (mCurrTabPos));
  gtk_notebook_set_tab_label (GTK_NOTEBOOK (mTabContainer), tabwidget,
                              tabname);
  gtk_notebook_set_current_page (GTK_NOTEBOOK (mTabContainer),
                                 gint (mCurrTabPos));
}

void
ApvlvView::append_child (ApvlvView *view)
{
  mChildren.push_back (view);
}

void
ApvlvView::erase_child (ApvlvView *view)
{
  auto itr = mChildren.begin ();
  while (*itr != view && itr != mChildren.end ())
    itr++;

  if (*itr == view)
    {
      mChildren.erase (itr);
    }
}

void
ApvlvView::saveKey (int has, int ct, guint key, bool end)
{
  if (processInLast)
    return;

  if (keyLastEnd)
    {
      keySquence.clear ();
    }

  struct keyNode key_node = { has, ct, key, end };
  keySquence.push_back (key_node);
  keyLastEnd = end;
}

void
ApvlvView::processLastKey ()
{
  processInLast = true;
  for (auto node : keySquence)
    {
      process (node.Has, node.Ct, node.Key);
    }
  processInLast = false;
}
}

// Local Variables:
// mode: c++
// End:

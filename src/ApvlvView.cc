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

#include "ApvlvParams.h"
#include "ApvlvInfo.h"
#include "ApvlvDir.h"
#include "ApvlvCmds.h"
#include "ApvlvView.h"

#ifdef APVLV_WITH_HTML
# include "ApvlvHtm.h"
#endif

#include <glib.h>
#include <glib/gprintf.h>
#include <gio/gio.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <gdk/gdkkeysyms.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include <iostream>
#include <fstream>
#include <sstream>

namespace apvlv
{
  ApvlvView *gView = NULL;

  const int ApvlvView::APVLV_MENU_HEIGHT = 30;
  const int ApvlvView::APVLV_CMD_BAR_HEIGHT = 20;
  const int ApvlvView::APVLV_TABS_HEIGHT = 36;

  ApvlvView::ApvlvView (const char *filename):mCurrTabPos (-1)
  {
    mProCmd = 0;
    mProCmdCnt = 0;

    mCurrHistroy = -1;

    mHasFull = FALSE;

    mMainWindow = gtk_window_new (GTK_WINDOW_TOPLEVEL);

    int w = gParams->valuei ("width");
    int h = gParams->valuei ("height");

    if (gParams->valueb ("fullscreen"))
      {
#if GTK_CHECK_VERSION(3, 0, 0)
        GdkRectangle rect[1];
        GdkDisplay *display = gdk_display_get_default ();
        GdkMonitor *monitor = gdk_display_get_primary_monitor (display);
        gdk_monitor_get_geometry (monitor, rect);
        mWidth = rect->width;
	mHeight = rect->height;
#else
        GdkScreen *scr = gdk_screen_get_default ();
        mWidth = gdk_screen_get_width (scr);
        mHeight = gdk_screen_get_height (scr);
#endif
        debug ("get screen size: %dx%d", mWidth, mHeight);
	fullscreen ();
      }
    else
      {
	mWidth = w;
	mHeight = h;
	gtk_window_set_default_size (GTK_WINDOW (mMainWindow),
				     w > 1 ? w : 800, h > 1 ? h : 600);
      }

#if GTK_CHECK_VERSION(3, 0, 0)
    mViewBox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
#else
    mViewBox = gtk_vbox_new (FALSE, 0);
#endif
    gtk_container_add (GTK_CONTAINER (mMainWindow), mViewBox);

    mMenu = new ApvlvMenu ();

    if (strchr (gParams->values ("guioptions"), 'm') != NULL)
      {
	mMenu->setsize (mWidth, APVLV_MENU_HEIGHT - 1);
	gtk_box_pack_start (GTK_BOX (mViewBox), mMenu->widget (), FALSE, FALSE,
			    0);
      }

    mTabContainer = gtk_notebook_new ();
    gtk_notebook_set_show_tabs (GTK_NOTEBOOK (mTabContainer), FALSE);
    gtk_notebook_set_scrollable (GTK_NOTEBOOK (mTabContainer), TRUE);
    gtk_box_pack_start (GTK_BOX (mViewBox), mTabContainer, TRUE, TRUE, 0);

    if (newtab (filename) == false)
      {
	bool ret = false;
	gchar *rpath = absolutepath (helppdf.c_str ());
	if (rpath)
	  {
	    ret = newtab (rpath);
	    g_free (rpath);
	  }

	if (ret == false)
	  {
	    exit (1);
	  }
      }

    mCommandBar = gtk_entry_new ();
    gtk_box_pack_end (GTK_BOX (mViewBox), mCommandBar, FALSE, FALSE, 0);

    g_signal_connect (G_OBJECT (mMainWindow), "size-allocate",
		      G_CALLBACK (apvlv_view_resized_cb), this);

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

    gtk_widget_show_all (mMainWindow);

    cmd_hide ();
  }

  ApvlvView::~ApvlvView ()
  {
    size_t i;

    delete mMenu;

    for (int i = 0; i < (int) mTabList.size (); i++)
      {
	delete mTabList[i].root;
      }
    mTabList.clear ();

    mCmdHistroy.clear ();

    for (i = 0; i < mDocs.size (); ++i)
      {
	ApvlvCore *core = (ApvlvCore *) mDocs[i];
	delete core;
      }
  }

  void ApvlvView::show ()
  {
    gtk_main ();
  }

  GtkWidget *ApvlvView::widget ()
  {
    return mMainWindow;
  }

  ApvlvWindow *ApvlvView::currentWindow ()
  {
    return ApvlvWindow::currentWindow ();
  }

  void ApvlvView::delcurrentWindow ()
  {
    ApvlvWindow::delcurrentWindow ();
    mTabList[mCurrTabPos].numwindows--;
    updatetabname ();
  }

  void ApvlvView::open ()
  {
    gchar *dirname;

    GtkWidget *dia = gtk_file_chooser_dialog_new ("Open ...",
						  GTK_WINDOW (mMainWindow),
						  GTK_FILE_CHOOSER_ACTION_SAVE,
						  ("_Cancel"),
						  GTK_RESPONSE_CANCEL,
						  ("_OK"),
						  GTK_RESPONSE_ACCEPT,
						  NULL);
    infofile *fp = gInfo->file (0);
    dirname =
      fp ? g_dirname (fp->file.c_str ()) :
      g_strdup (gParams->values ("defaultdir"));
    debug ("lastfile: [%s], dirname: [%s]", fp ? fp->file.c_str () : "",
	   dirname);
    gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (dia), dirname);
    g_free (dirname);

    GtkFileFilter *filter = gtk_file_filter_new ();
    gtk_file_filter_add_mime_type (filter, "PDF File");
    gtk_file_filter_add_pattern (filter, "*.pdf");
    gtk_file_filter_add_pattern (filter, "*.PDF");
#ifdef APVLV_WITH_DJVU
    gtk_file_filter_add_mime_type (filter, "DJVU File");
    gtk_file_filter_add_pattern (filter, "*.DJV");
    gtk_file_filter_add_pattern (filter, "*.djv");
    gtk_file_filter_add_pattern (filter, "*.DJVU");
    gtk_file_filter_add_pattern (filter, "*.djvu");
#endif
#ifdef APVLV_WITH_TXT
    gtk_file_filter_add_mime_type (filter, "TXT File");
    gtk_file_filter_add_pattern (filter, "*.TXT");
    gtk_file_filter_add_pattern (filter, "*.txt");
#endif
#ifdef APVLV_WITH_UMD
    gtk_file_filter_add_mime_type (filter, "UMD File");
    gtk_file_filter_add_pattern (filter, "*.UMD");
    gtk_file_filter_add_pattern (filter, "*.umd");
#endif
#ifdef APVLV_WITH_HTML
    gtk_file_filter_add_mime_type (filter, "HTML File");
    gtk_file_filter_add_pattern (filter, "*.HTM");
    gtk_file_filter_add_pattern (filter, "*.htm");
    gtk_file_filter_add_pattern (filter, "*.HTML");
    gtk_file_filter_add_pattern (filter, "*.html");
#endif
    gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (dia), filter);

    gint ret = gtk_dialog_run (GTK_DIALOG (dia));
    if (ret == GTK_RESPONSE_ACCEPT)
      {
	gchar *filename =
	  gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dia));

#ifdef APVLV_WITH_HTML
	if (g_ascii_strcasecmp (filename + strlen (filename) - 4,
				".htm") == 0
	    || g_ascii_strcasecmp (filename + strlen (filename) - 5,
				   ".html") == 0)
	  {
	    loadhtml (filename);
	  }
	else
#endif
	  {
	    loadfile (filename);
	  }
	g_free (filename);
      }
    gtk_widget_destroy (dia);
  }

  void ApvlvView::opendir ()
  {
    gchar *dirname;

    GtkWidget *dia = gtk_file_chooser_dialog_new ("",
						  GTK_WINDOW (mMainWindow),
						  GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
						  ("_Cancel"),
						  GTK_RESPONSE_CANCEL,
						  ("_OK"),
						  GTK_RESPONSE_ACCEPT,
						  NULL);
    infofile *fp = gInfo->file (0);
    dirname = fp ? g_dirname (fp->file.c_str ()) :
      g_strdup (gParams->values ("defaultdir"));
    gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (dia), dirname);
    g_free (dirname);

    gint ret = gtk_dialog_run (GTK_DIALOG (dia));
    if (ret == GTK_RESPONSE_ACCEPT)
      {
	gchar *filename =
	  gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dia));

	loaddir (filename);
	g_free (filename);
      }
    gtk_widget_destroy (dia);
  }

  bool ApvlvView::loaddir (const char *path)
  {
    ApvlvCore *ndoc = hasloaded (path, CORE_DIR);
    if (ndoc == NULL)
      {
	int w, h;
	currentWindow ()->getsize (&w, &h);
	ndoc = new ApvlvDir (w, h);
	if (!ndoc->loadfile (path))
	  {
	    delete ndoc;
	    return false;
	  }
	regloaded (ndoc);
      }

    currentWindow ()->setCore (ndoc);
    updatetabname ();
    return true;
  }

#ifdef APVLV_WITH_HTML
  bool ApvlvView::loadhtml (const char *path)
  {
    ApvlvCore *ndoc = hasloaded (path, CORE_DIR);
    if (ndoc == NULL)
      {
	int w, h;
	currentWindow ()->getsize (&w, &h);
	ndoc = new ApvlvHTML (w, h);
	if (!ndoc->loadfile (path))
	  {
	    delete ndoc;
	    return false;
	  }
	regloaded (ndoc);
      }

    currentWindow ()->setCore (ndoc);
    updatetabname ();
    return true;
  }
#endif

  bool ApvlvView::loadfile (string file)
  {
    return loadfile (file.c_str ());
  }

  void ApvlvView::quit ()
  {
    if ((int) mTabList.size () == 1)
      {
	mTabList.clear ();
	apvlv_view_delete_cb (NULL, NULL, this);
	return;
      }

    int p = mCurrTabPos;
    if (mCurrTabPos < (int) mTabList.size () - 1)
      switch_tabcontext (mCurrTabPos + 1);
    else
      switch_tabcontext (mCurrTabPos - 1);

    gtk_notebook_set_current_page (GTK_NOTEBOOK (mTabContainer), mCurrTabPos);
    mRootWindow->setsize (mWidth, adjheight ());

    gtk_notebook_remove_page (GTK_NOTEBOOK (mTabContainer), p);
    delete_tabcontext (p);

    if (mCurrTabPos > p)
      --mCurrTabPos;

    mRootWindow->setsize (mWidth, adjheight ());
  }

  bool ApvlvView::newtab (const char *filename)
  {
    ApvlvCore *ndoc;
    ndoc = hasloaded (filename,
		      gParams->valueb ("content") ? CORE_CONTENT : CORE_DOC);

    if (ndoc == NULL)
      {
	if (gParams->valueb ("content"))
	  {
	    ndoc = new ApvlvDir (mWidth, adjheight ());
	    if (!ndoc->loadfile (filename))
	      {
		delete ndoc;
		ndoc = NULL;
	      }
	  }

	if (ndoc == NULL)
	  {
	    ndoc =
	      new ApvlvDoc (mWidth, mHeight, gParams->values ("zoom"), false);
	    if (!ndoc->loadfile (filename))
	      {
		delete ndoc;
		ndoc = NULL;
	      }
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

  bool ApvlvView::newtab (ApvlvCore * core)
  {
    int pos = new_tabcontext (core, true);

    switch_tabcontext (pos);
    mRootWindow->setsize (mWidth, adjheight ());

    gchar *base =
      core->filename ()? g_path_get_basename (core->filename ()) :
      g_strdup ("NONE");
    GtkWidget *tabname = gtk_label_new (base);
    g_free (base);

#if GTK_CHECK_VERSION (3, 0, 0)
    GtkWidget *parentbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
#else
    GtkWidget *parentbox = gtk_vbox_new (FALSE, 0);
#endif
    gtk_container_add (GTK_CONTAINER (parentbox),
		       mTabList[mCurrTabPos].root->widget ());

    gtk_notebook_insert_page (GTK_NOTEBOOK (mTabContainer), parentbox,
			      tabname, pos);

    gtk_widget_show_all (parentbox);
    gtk_notebook_set_current_page (GTK_NOTEBOOK (mTabContainer), mCurrTabPos);
    return true;
  }

  int ApvlvView::new_tabcontext (ApvlvCore * core, bool insertAfterCurr)
  {
    ApvlvWindow *troot = new ApvlvWindow (core);
    TabEntry context (troot, troot, 1);
    if (!insertAfterCurr || mCurrTabPos == -1)
      {
	mTabList.push_back (context);
	return mTabList.size () - 1;
      }

    std::vector < TabEntry >::iterator
      insPos = mTabList.begin () + mCurrTabPos + 1;
    mTabList.insert (insPos, context);

    if (mTabList.size () > 1)
      {
	gtk_notebook_set_show_tabs (GTK_NOTEBOOK (mTabContainer), TRUE);
	gtk_widget_set_size_request (mTabContainer, mWidth - 12, 12);
      }

    return mCurrTabPos + 1;
  }

  void ApvlvView::delete_tabcontext (int tabPos)
  {
    asst (tabPos >= 0 && tabPos < (int) mTabList.size ());

    std::vector < TabEntry >::iterator remPos = mTabList.begin () + tabPos;

    if (remPos->root != NULL)
      {
	delete remPos->root;
	remPos->root = NULL;
      }

    int c = mTabList.size ();
    mTabList.erase (remPos);
    if (c == (int) mTabList.size ())
      {
	gView->errormessage ("erase failed to remove context");
      }

    if ((int) mTabList.size () <= 1)
      {
	gtk_notebook_set_show_tabs (GTK_NOTEBOOK (mTabContainer), FALSE);
      }
  }

  void ApvlvView::switch_tabcontext (int tabPos)
  {
    asst (tabPos >= 0 && tabPos < (int) mTabList.size ());

    if (mCurrTabPos != -1)
      mTabList[mCurrTabPos].curr = currentWindow ();

    mCurrTabPos = tabPos;
    mRootWindow = mTabList[tabPos].root;
    ApvlvWindow::setcurrentWindow (NULL, mTabList[tabPos].curr);

    if (crtadoc () && crtadoc ()->filename ())
      {
	gchar *base = g_path_get_basename (crtadoc ()->filename ());
	settitle (base);
	g_free (base);
      }
  }

  bool ApvlvView::loadfile (const char *filename)
  {
    if (filename == NULL || *filename == '\0')
      {
	return false;
      }

    char *abpath = absolutepath (filename);
    if (abpath == NULL)
      {
	return false;
      }

    ApvlvWindow *win = ApvlvWindow::currentWindow ();
    ApvlvCore *ndoc;

    ndoc =
      hasloaded (abpath,
		 gParams->valueb ("content") ? CORE_CONTENT : CORE_DOC);

    if (ndoc == NULL)
      {
	int w, h;
	win->getsize (&w, &h);
	if (gParams->valueb ("content"))
	  {
	    ndoc = new ApvlvDir (w, h);
	    if (!ndoc->loadfile (abpath))
	      {
		debug ("can't load");
		delete ndoc;
		ndoc = NULL;
	      }
	  }

	if (ndoc == NULL)
	  {
	    ndoc = new ApvlvDoc (w, h, gParams->values ("zoom"), false);
	    if (!ndoc->loadfile (filename))
	      {
		delete ndoc;
		ndoc = NULL;
	      }
	  }

	if (ndoc != NULL)
	  {
	    regloaded (ndoc);
	  }
      }

    if (ndoc != NULL)
      {
	win->setCore (ndoc);
	updatetabname ();
      }
    g_free (abpath);

    return ndoc != NULL ? true : false;
  }

  ApvlvCore *ApvlvView::hasloaded (const char *abpath, int type)
  {
    ApvlvCore *core;
    size_t i;
    for (i = 0; i < mDocs.size (); ++i)
      {
	core = (ApvlvCore *) mDocs[i];
	if (core->inuse () == false
	    && core->type () == type
	    && strcmp (core->filename (), abpath) == 0)
	  {
	    return core;
	  }
      }
    return NULL;
  }

  void ApvlvView::regloaded (ApvlvCore * core)
  {
    if (gParams->valuei ("pdfcache") < 2)
      {
	gParams->push ("pdfcache", "2");
      }

    if (mDocs.size () >= (size_t) gParams->valuei ("pdfcache"))
      {
	std::vector < ApvlvCore * >::iterator itr = mDocs.begin ();
	debug ("to pdf cache size: %d, remove first: %p\n",
	       gParams->valuei ("pdfcache"), *itr);
	mDocs.erase (itr);
      }
    mDocs.push_back (core);
  }

  ApvlvCompletion *ApvlvView::filecompleteinit (const char *path)
  {
    ApvlvCompletion *comp = new ApvlvCompletion;
    GList *list = g_list_alloc ();
    gchar *dname, *bname;
    const gchar *name;

    dname = g_path_get_dirname (path);
    GDir *dir = g_dir_open ((const char *) dname, 0, NULL);
    if (dir != NULL)
      {
	bname = g_path_get_basename (path);
	size_t len = strlen (bname);
	while ((name = g_dir_read_name (dir)) != NULL)
	  {
	    gchar *fname = g_locale_from_utf8 (name, -1, NULL, NULL, NULL);

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
		    list->data = g_strjoin ("", dname, fname, NULL);
		  }
		else
		  {
		    list->data = g_strjoin (PATH_SEP_S, dname, fname, NULL);
		  }
	      }

	    g_free (fname);

	    debug ("add a item: %s", (char *) list->data);
	    comp->add_items (list);
	  }
	g_free (bname);
	g_dir_close (dir);
      }
    g_free (dname);

    g_list_free (list);

    return comp;
  }

  void ApvlvView::promptcommand (char ch)
  {
    char s[2] = { 0 };
    s[0] = ch;
    gtk_entry_set_text (GTK_ENTRY (mCommandBar), s);
    cmd_show (CMD_CMD);
  }

  void ApvlvView::promptcommand (const char *s)
  {
    gtk_entry_set_text (GTK_ENTRY (mCommandBar), s);
    cmd_show (CMD_CMD);
  }

  void ApvlvView::errormessage (const char *str, ...)
  {
    gchar estr[512];
    gint pos;
    va_list vap;
    va_start (vap, str);
    vsnprintf (estr, sizeof estr, str, vap);
    va_end (vap);
    gtk_entry_set_text (GTK_ENTRY (mCommandBar), "ERROR: ");
    gtk_editable_set_position(GTK_EDITABLE (mCommandBar), -1);
    pos = gtk_editable_get_position (GTK_EDITABLE (mCommandBar));
    gtk_editable_insert_text(GTK_EDITABLE (mCommandBar), estr, -1, &pos);
    cmd_show (CMD_MESSAGE);
  }

  void ApvlvView::infomessage (const char *str, ...)
  {
    gchar estr[512];
    gint pos;
    va_list vap;
    va_start (vap, str);
    vsnprintf (estr, sizeof estr, str, vap);
    va_end (vap);
    gtk_entry_set_text (GTK_ENTRY (mCommandBar), "INFO: ");
    gtk_editable_set_position(GTK_EDITABLE (mCommandBar), -1);
    pos = gtk_editable_get_position (GTK_EDITABLE (mCommandBar));
    gtk_editable_insert_text(GTK_EDITABLE (mCommandBar), estr, -1, &pos);
    cmd_show (CMD_MESSAGE);
  }

  void ApvlvView::cmd_show (int cmdtype)
  {
    if (mMainWindow == NULL)
      return;

    mCmdType = cmdtype;

    mRootWindow->setsize (mWidth, adjheight ());

    gtk_widget_show (mCommandBar);
    gtk_widget_grab_focus (mCommandBar);
    gtk_editable_set_position (GTK_EDITABLE (mCommandBar), -1);
  }

  void ApvlvView::cmd_hide ()
  {
    if (mMainWindow == NULL)
      return;
    mCmdType = CMD_NONE;

    gtk_widget_hide (mCommandBar);
    mRootWindow->setsize (mWidth, adjheight ());

    gtk_widget_grab_focus (mMainWindow);
  }

  void ApvlvView::cmd_auto (const char *ps)
  {
    ApvlvCompletion *comp = NULL;

    stringstream ss (ps);
    string cmd, np, argu;
    ss >> cmd >> np >> argu;

    if (np[np.length () - 1] == '\\')
      {
	np.replace (np.length () - 1, 1, 1, ' ');
	np += argu;
	ss >> argu;
      }

    if (cmd == "" || np == "")
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
	    list->data = g_strdup (((ApvlvCore *) mDocs[i])->filename ());
	    comp->add_items (list);
	  }
	g_list_free (list);
      }

    if (comp != NULL)
      {
	char *comtext = NULL;
	debug ("find match: %s", np.c_str ());
	comp->complete (np.c_str (), &comtext);
	if (comtext != NULL)
	  {
	    char text[PATH_MAX];

	    debug ("get a match: %s", comtext);
	    gchar **v;

	    v = g_strsplit (comtext, " ", -1);
	    if (v != NULL)
	      {
		gchar *comstr = g_strjoinv ("\\ ", v);
		g_snprintf (text, sizeof text, ":%s %s", cmd.c_str (),
			    comstr);
		g_free (comstr);
		g_strfreev (v);
	      }
	    else
	      {
		g_snprintf (text, sizeof text, ":%s %s", cmd.c_str (),
			    comtext);
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

  void ApvlvView::fullscreen ()
  {
    if (mHasFull == false)
      {
	gtk_window_fullscreen (GTK_WINDOW (mMainWindow));
	mHasFull = true;
      }
    else
      {
	gtk_window_unfullscreen (GTK_WINDOW (mMainWindow));
	mHasFull = false;
      }
  }

  ApvlvCore *ApvlvView::crtadoc ()
  {
    return currentWindow ()->getCore ();
  }

  returnType ApvlvView::subprocess (int ct, guint key)
  {
    guint procmd = mProCmd;
    mProCmd = 0;
    mProCmdCnt = 0;
    switch (procmd)
      {
      case CTRL ('w'):
	if (key == 'q' || key == CTRL ('Q'))
	  {
	    if (currentWindow ()->istop ())
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
	if (key == 't'){
	  if (ct == 0)
	    switchtab (mCurrTabPos+1);
	  else
	    switchtab (ct-1);
        }else if (key == 'T'){
	  if (ct == 0)
	    switchtab (mCurrTabPos-1);
	  else
	    switchtab (ct-1);
        }else if (key == 'g'){
	  if (ct == 0)
	    ct = 1;
	  crtadoc ()->showpage (ct-1);
        }
	break;

      default:
	return NO_MATCH;
	break;
      }

    return MATCH;
  }

  returnType ApvlvView::process (int has, int ct, guint key)
  {
    if (mProCmd != 0)
      {
	return subprocess (mProCmdCnt, key);
      }

    switch (key)
      {
      case CTRL ('w'):
	mProCmd = CTRL ('w');
        mProCmdCnt = has ? ct : 0;
	return NEED_MORE;
	break;
      case 'q':
	quit ();
	break;
      case 'f':
	fullscreen ();
	break;
      case 'g':
	mProCmd = 'g';
        mProCmdCnt = has ? ct : 0;
	return NEED_MORE;
      default:
	return crtadoc ()->process (has, ct, key);
	break;
      }

    return MATCH;
  }

  bool ApvlvView::run (const char *str)
  {
    bool ret;

    switch (*str)
      {
      case SEARCH:
	crtadoc ()->markposition ('\'');
	ret = crtadoc ()->search (str + 1);
	break;

      case BACKSEARCH:
	crtadoc ()->markposition ('\'');
	ret = crtadoc ()->search (str + 1, true);
	break;

      case COMMANDMODE:
	ret = runcmd (str + 1);
	break;

      default:
	ret = false;
	break;
      }

    return ret;
  }

  void ApvlvView::settitle (const char *title)
  {
    gtk_window_set_title (GTK_WINDOW (mMainWindow), title);
  }

  bool ApvlvView::runcmd (const char *str)
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
		crtadoc ()->setskip (atoi (argu.c_str ()));
	      }
	    else
	      {
		gParams->push (subcmd, argu);
	      }
	  }
	else if (cmd == "map" && subcmd != "")
	  {
	    gCmds->buildmap (subcmd.c_str (), argu.c_str ());
	  }
	else if ((cmd == "o"
		  || cmd == "open" || cmd == "doc") && subcmd != "")
	  {
            char *home = NULL;

            if(subcmd[0] == '~') {
              home = getenv("HOME");

              if(home) {
                subcmd.replace(0, 1, home);
              }
            }

	    if (g_file_test (subcmd.c_str (), G_FILE_TEST_IS_DIR))
	      {
		ret = loaddir (subcmd.c_str ());
	      }
#ifdef APVLV_WITH_HTML
	    else if (g_ascii_strncasecmp (subcmd.c_str (), "http://", 7) == 0
		     || g_ascii_strcasecmp (subcmd.c_str () + subcmd.length () - 4,
					    ".htm") == 0
		     || g_ascii_strcasecmp (subcmd.c_str () + subcmd.length () - 5,
					    ".html") == 0)
	      {
		ret = loadhtml (subcmd.c_str ());
	      }
#endif
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
	else if (cmd == "TOtext" && subcmd != "")
	  {
	    crtadoc ()->totext (subcmd.c_str ());
	  }
	else if ((cmd == "pr" || cmd == "print"))
	  {
	    crtadoc ()->print (subcmd == "" ? -1 : atoi (subcmd.c_str ()));
	  }
	else if (cmd == "sp")
	  {
	    currentWindow ()->birth (false);
	    windowadded ();
	  }
	else if (cmd == "vsp")
	  {
	    currentWindow ()->birth (true);
	    windowadded ();
	  }
	else if ((cmd == "zoom" || cmd == "z") && subcmd != "")
	  {
	    crtadoc ()->setzoom (subcmd.c_str ());
	  }
	else if (cmd == "forwardpage" || cmd == "fp")
	  {
	    if (subcmd == "")
	      crtadoc ()->nextpage (1);
	    else
	      crtadoc ()->nextpage (atoi (subcmd.c_str ()));
	  }
	else if (cmd == "prewardpage" || cmd == "bp")
	  {
	    if (subcmd == "")
	      crtadoc ()->prepage (1);
	    else
	      crtadoc ()->prepage (atoi (subcmd.c_str ()));
	  }
	else if (cmd == "goto" || cmd == "g")
	  {
	    crtadoc ()->markposition ('\'');
	    int p = atoi (subcmd.c_str ());
	    p += crtadoc ()->getskip ();
	    crtadoc ()->showpage (p - 1);
	  }
	else if ((cmd == "help" || cmd == "h") && subcmd == "info")
	  {
	    loadfile (helppdf);
	    crtadoc ()->showpage (1);
	  }
	else if ((cmd == "help" || cmd == "h") && subcmd == "command")
	  {
	    loadfile (helppdf);
	    crtadoc ()->showpage (3);
	  }
	else if ((cmd == "help" || cmd == "h") && subcmd == "setting")
	  {
	    crtadoc ()->loadfile (helppdf.c_str ());
	    crtadoc ()->showpage (8);
	  }
	else if ((cmd == "help" || cmd == "h") && subcmd == "prompt")
	  {
	    crtadoc ()->loadfile (helppdf.c_str ());
	    crtadoc ()->showpage (8);
	  }
	else if (cmd == "help" || cmd == "h")
	  {
	    loadfile (helppdf);
	  }
	else if (cmd == "q" || cmd == "quit")
	  {
	    if (currentWindow ()->istop ())
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
	    while (!mTabList.empty ())
	      {
		if (currentWindow ()->istop ())
		  {
		    quit ();
		  }
		else
		  {
		    delcurrentWindow ();
		  }
	      }
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
	    crtadoc ()->writefile (subcmd.size () >
				   0 ? subcmd.c_str () : NULL);
	  }
	else if (cmd == "toc")
	  {
	    loadfile (crtadoc ()->filename ());
	  }
	else
	  {
	    bool isn = true;
	    for (size_t i = 0; i < cmd.length (); ++i)
	      {
		if (cmd[i] < '0' || cmd[i] > '9')
		  {
		    isn = false;
		    break;
		  }
	      }
	    if (isn && crtadoc ())
	      {
		int p = atoi (cmd.c_str ());
		p += crtadoc ()->getskip ();
		if (p != crtadoc ()->pagenumber ())
		  {
		    crtadoc ()->showpage (p - 1);
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

  void
  ApvlvView::apvlv_view_resized_cb (GtkWidget * wid, GtkAllocation * al,
				    ApvlvView * view)
  {
    int w, h;
    GtkAllocation allocation;

    gtk_widget_get_allocation(view->mViewBox, &allocation);
    debug ("view resized: %d:%d", allocation.width, allocation.height);
    w = allocation.width - 12;
    h = allocation.height - 12;
    if (w != view->mWidth || h != view->mHeight)
      {
	view->mWidth = w;
	view->mHeight = h;
	view->mRootWindow->setsize (w, view->adjheight () - 10);
      }
  }

  gint
  ApvlvView::apvlv_view_keypress_cb (GtkWidget * wid, GdkEvent * ev,
				     ApvlvView * view)
  {
    if (view->mCmdType == CMD_NONE)
      {
	gCmds->append ((GdkEventKey *) ev);
	return TRUE;
      }

    return FALSE;
  }

  bool
  isalt_escape(GdkEventKey *event)
  {
    /* Grab the default modifier mask... */
    guint modifiers = gtk_accelerator_get_default_mod_mask();

    if(event->keyval == GDK_KEY_bracketleft &&
       /* ...so we can ignore mod keys like numlock and capslock. */
       (event->state & modifiers) == GDK_CONTROL_MASK) {
      return true;
    }

    return false;
  }

  gint
  ApvlvView::apvlv_view_commandbar_cb (GtkWidget * wid, GdkEvent * ev,
				       ApvlvView * view)
  {
    if (view->mCmdType == CMD_CMD)
      {
	view->mInHistroy = false;

	GdkEventKey *gek = (GdkEventKey *) ev;
	if (gek->keyval == GDK_KEY_Return)
	  {
	    gchar *str =
	      (gchar *) gtk_entry_get_text (GTK_ENTRY (view->mCommandBar));
	    if (str && strlen (str) > 0)
	      {
		if (view->run (str) == true)
		  {
		    view->mCmdHistroy.push_back (str);
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
	    gchar *str =
	      (gchar *) gtk_entry_get_text (GTK_ENTRY (view->mCommandBar));
	    if (str && strlen (str) > 0)
	      {
		view->cmd_auto (str + 1);
	      }
	    return TRUE;
	  }
	else if (gek->keyval == GDK_KEY_BackSpace)
	  {
	    gchar *str =
	      (gchar *) gtk_entry_get_text (GTK_ENTRY (view->mCommandBar));
	    if (str == NULL || strlen (str) == 1)
	      {
		view->cmd_hide ();
		view->mCurrHistroy = view->mCmdHistroy.size () - 1;
		return TRUE;
	      }
	  }
	else if (gek->keyval == GDK_KEY_Escape || isalt_escape(gek))
	  {
	    view->cmd_hide ();
	    view->mCurrHistroy = view->mCmdHistroy.size () - 1;
	    return TRUE;
	  }
	else if (gek->keyval == GDK_KEY_Up)
	  {
	    if (view->mCmdHistroy.size () == 0)
	      {
		return TRUE;
	      }

	    view->mInHistroy = true;
	    gtk_entry_set_text (GTK_ENTRY (view->mCommandBar),
				view->mCurrHistroy > 0 ?
				view->mCmdHistroy[view->
						  mCurrHistroy--].c_str () :
				view->mCmdHistroy[0].c_str ());
	    return TRUE;
	  }
	else if (gek->keyval == GDK_KEY_Down)
	  {
	    if (view->mCmdHistroy.size () == 0)
	      {
		return TRUE;
	      }

	    view->mInHistroy = true;
	    gtk_entry_set_text (GTK_ENTRY (view->mCommandBar),
				(size_t) view->mCurrHistroy <
				view->mCmdHistroy.size () -
				1 ? view->mCmdHistroy[++view->
						      mCurrHistroy].c_str () :
				view->mCmdHistroy[view->mCmdHistroy.size () -
						  1].c_str ());
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
  ApvlvView::apvlv_view_delete_cb (GtkWidget * wid, GtkAllocation * al,
				   ApvlvView * view)
  {
    view->mMainWindow = NULL;
    gtk_main_quit ();
  }

  void
  ApvlvView::apvlv_notebook_switch_cb (GtkWidget * wid,
				       GtkNotebook * notebook, guint pnum,
				       ApvlvView * view)
  {
    view->mCurrTabPos = pnum;
    view->mRootWindow = view->mTabList[pnum].root;
    ApvlvWindow::setcurrentWindow (NULL, view->mTabList[pnum].curr);
  }

  int ApvlvView::adjheight ()
  {
    int adj = 0;
    if (gtk_notebook_get_show_tabs (GTK_NOTEBOOK (mTabContainer)))
      {
	adj += APVLV_TABS_HEIGHT;
      }
    if (mCmdType != CMD_NONE)
      {
	adj += APVLV_CMD_BAR_HEIGHT;
      }
    if (strchr (gParams->values ("guioptions"), 'm') != NULL)
      {
	adj += APVLV_MENU_HEIGHT;
      }

    return mHeight - adj;
  }

  void ApvlvView::switchtab (int tabPos)
  {
    int ntabs = mTabList.size ();
    while (tabPos < 0)
      tabPos += ntabs;

    tabPos = tabPos % ntabs;
    switch_tabcontext (tabPos);
    mRootWindow->setsize (mWidth, adjheight ());
    gtk_notebook_set_current_page (GTK_NOTEBOOK (mTabContainer), mCurrTabPos);
  }

  void ApvlvView::windowadded ()
  {
    mTabList[mCurrTabPos].numwindows++;
    updatetabname ();
  }

  void ApvlvView::updatetabname ()
  {
    char tagname[26];

    const char *filename = currentWindow ()->getCore ()->filename ();
    gchar *gfilename;

    if (filename == NULL)
      gfilename = g_strdup ("None");
    else
      gfilename = g_path_get_basename (filename);

    settitle (gfilename);

    if (mTabList[mCurrTabPos].numwindows > 1)
      g_snprintf (tagname, sizeof tagname, "[%d] %s",
		  mTabList[mCurrTabPos].numwindows, gfilename);
    else
      g_snprintf (tagname, sizeof tagname, "%s", gfilename);

    g_free (gfilename);

    GtkWidget *tabname = gtk_label_new (tagname);
    GtkWidget *tabwidget =
      gtk_notebook_get_nth_page (GTK_NOTEBOOK (mTabContainer), mCurrTabPos);
    gtk_notebook_set_tab_label (GTK_NOTEBOOK (mTabContainer), tabwidget,
				tabname);
    gtk_notebook_set_current_page (GTK_NOTEBOOK (mTabContainer), mCurrTabPos);
  }
}

// Local Variables:
// mode: c++
// End:

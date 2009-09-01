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
/* @CPPFILE ApvlvView.cpp
 *
 *  Author: Alf <naihe2010@gmail.com>
 */
/* @date Created: 2008/09/30 00:00:00 Alf */

#include "ApvlvParams.hpp"
#include "ApvlvDir.hpp"
#include "ApvlvCmds.hpp"
#include "ApvlvView.hpp"

#include <stdlib.h>
#include <string.h>

#include <glib.h>
#include <glib/gprintf.h>
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

  const int ApvlvView::APVLV_CMD_BAR_HEIGHT = 20;
  const int ApvlvView::APVLV_TABS_HEIGHT = 30;

    ApvlvView::ApvlvView (const char *filename):mHasTabs (false),
    mCurrTabPos (-1)
  {
    mProCmd = 0;

    mHasFull = FALSE;

    mMainWindow = gtk_window_new (GTK_WINDOW_TOPLEVEL);

    int w = gParams->valuei ("width");
    int h = gParams->valuei ("height");

    if (gParams->valueb ("fullscreen"))
      {
	fullscreen ();
      }
    else
      {
	gtk_window_set_default_size (GTK_WINDOW (mMainWindow),
				     w > 1 ? w : 800, h > 1 ? h : 600);
      }

    mWidth = w;
    mHeight = h;

    GtkWidget *vbox = gtk_vbox_new (FALSE, 2);
    gtk_container_add (GTK_CONTAINER (mMainWindow), vbox);

    mTabContainer = gtk_notebook_new ();
    gtk_notebook_set_show_tabs (GTK_NOTEBOOK (mTabContainer), FALSE);
    gtk_notebook_set_scrollable (GTK_NOTEBOOK (mTabContainer), TRUE);
    gtk_box_pack_start (GTK_BOX (vbox), mTabContainer, FALSE, FALSE, 0);

    newtab (filename);

    mCommandBar = gtk_entry_new ();
    gtk_box_pack_end (GTK_BOX (vbox), mCommandBar, FALSE, FALSE, 0);

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
    for (i = 0; i < mDocs.size (); ++i)
      {
	ApvlvCore *core = (ApvlvCore *) mDocs[i];
	delete core;
      }

    for (int i = 0; i < (int) mTabList.size (); i++)
      {
	delete mTabList[i].root;
      }
    mTabList.clear ();

    mCmdHistroy.clear ();
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
    const char *lastfile = NULL;
    gchar *dirname;

    char *path = absolutepath (sessionfile.c_str ());
    ifstream os (path, ios::in);
    g_free (path);

    string line, files;
    if (os.is_open ())
      {

	while ((getline (os, line)) != NULL)
	  {
	    const char *p = line.c_str ();

	    if (*p == '>')
	      {
		stringstream ss (++p);
		ss >> files;
		lastfile = files.c_str ();
	      }
	  }
	os.close ();
      }

    GtkWidget *dia = gtk_file_chooser_dialog_new ("",
						  GTK_WINDOW (mMainWindow),
						  GTK_FILE_CHOOSER_ACTION_SAVE,
						  GTK_STOCK_CANCEL,
						  GTK_RESPONSE_CANCEL,
						  GTK_STOCK_OK,
						  GTK_RESPONSE_ACCEPT,
						  NULL);
    dirname =
      lastfile ? g_dirname (lastfile) : g_strdup (gParams->
						  values ("defaultdir"));
    gchar *realdir = absolutepath (dirname);
    g_free (dirname);
    debug ("lastfile: [%s], dirname: [%s]", lastfile, realdir);
    gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (dia), realdir);
    g_free (realdir);

    GtkFileFilter *filter = gtk_file_filter_new ();
    gtk_file_filter_add_mime_type (filter, "PDF File");
    gtk_file_filter_add_pattern (filter, "*.pdf");
    gtk_file_filter_add_pattern (filter, "*.PDF");
    gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (dia), filter);

    gint ret = gtk_dialog_run (GTK_DIALOG (dia));
    if (ret == GTK_RESPONSE_ACCEPT)
      {
	gchar *filename =
	  gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dia));

	loadfile (filename);
	g_free (filename);
      }
    gtk_widget_destroy (dia);
  }

  void ApvlvView::opendir ()
  {
    const char *lastfile = NULL;
    gchar *dirname;

    char *path = absolutepath (sessionfile.c_str ());
    ifstream os (path, ios::in);
    g_free (path);

    string line, files;
    if (os.is_open ())
      {

	while ((getline (os, line)) != NULL)
	  {
	    const char *p = line.c_str ();

	    if (*p == '>')
	      {
		stringstream ss (++p);
		ss >> files;
		lastfile = files.c_str ();
	      }
	  }
	os.close ();
      }

    GtkWidget *dia = gtk_file_chooser_dialog_new ("",
						  GTK_WINDOW (mMainWindow),
						  GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
						  GTK_STOCK_CANCEL,
						  GTK_RESPONSE_CANCEL,
						  GTK_STOCK_OK,
						  GTK_RESPONSE_ACCEPT,
						  NULL);
    dirname =
      lastfile ? g_dirname (lastfile) : g_strdup (gParams->
						  values ("defaultdir"));
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

  void ApvlvView::newtab (const char *filename)
  {
    ApvlvDoc *ndoc = new ApvlvDoc (mWidth, mHeight, gParams->values ("zoom"),
				   gParams->valueb ("cache"));
    if (ndoc->loadfile (filename))
      {
	regloaded (ndoc);
      }
    newtab (ndoc);
  }

  void ApvlvView::newtab (ApvlvCore * core)
  {
    int pos = new_tabcontext (core, true);

    switch_tabcontext (pos);
    mRootWindow->setsize (mWidth, adjheight ());

    gchar *base =
      core->filename ()? g_path_get_basename (core->
					      filename ()) :
      g_strdup ("NONE");
    GtkWidget *tabname = gtk_label_new (base);
    settitle (base);
    g_free (base);

    GtkWidget *parentbox = gtk_vbox_new (false, 0);
    gtk_container_add (GTK_CONTAINER (parentbox),
		       mTabList[mCurrTabPos].root->widget ());

    gtk_notebook_insert_page (GTK_NOTEBOOK (mTabContainer), parentbox,
			      tabname, pos);

    gtk_widget_show_all (parentbox);
    gtk_notebook_set_current_page (GTK_NOTEBOOK (mTabContainer), mCurrTabPos);
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
	mHasTabs = true;
	gtk_notebook_set_show_tabs (GTK_NOTEBOOK (mTabContainer), TRUE);
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
	warnp ("erase failed to remove context");
      }

    if ((int) mTabList.size () <= 1)
      {
	gtk_notebook_set_show_tabs (GTK_NOTEBOOK (mTabContainer), FALSE);
	mHasTabs = false;
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
  }

  bool ApvlvView::loadfile (const char *filename)
  {
    if (filename == NULL || *filename == '\0')
      {
	return false;
      }

    char *abpath = absolutepath (filename);
    if (abpath != NULL)
      {
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

	    if (ndoc)
	      {
		regloaded (ndoc);
	      }
	  }

	if (ndoc != NULL)
	  {
	    win->setCore (ndoc);
	  }
	g_free (abpath);
      }

    updatetabname ();
    return true;
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
    if (mDocs.size () >= (size_t) gParams->valuei ("pdfcache"))
      {
	std::vector < ApvlvCore * >::iterator itr = mDocs.begin ();
	debug ("to pdf cache size: %d, remove first: %p\n",
	       gParams->valuei ("pdfcache"), *itr);
	mDocs.erase (itr);
      }
    mDocs.push_back (core);
  }

  GCompletion *ApvlvView::filecompleteinit (const char *path)
  {
    GCompletion *gcomp = g_completion_new (NULL);
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
	    g_completion_add_items (gcomp, list);
	  }
	g_free (bname);
	g_dir_close (dir);
      }
    g_free (dname);

    g_list_free (list);

    return gcomp;
  }

  void ApvlvView::promptcommand (char ch)
  {
    char s[2] = { 0 };
    s[0] = ch;
    gtk_entry_set_text (GTK_ENTRY (mCommandBar), s);
    cmd_show ();
  }

  void ApvlvView::promptcommand (const char *s)
  {
    gtk_entry_set_text (GTK_ENTRY (mCommandBar), s);
    cmd_show ();
  }

  void ApvlvView::cmd_show ()
  {
    if (mMainWindow == NULL)
      return;

    mHasCmd = TRUE;

    gtk_widget_set_usize (mCommandBar, mWidth, APVLV_CMD_BAR_HEIGHT);
    mRootWindow->setsize (mWidth, adjheight ());

    gtk_widget_grab_focus (mCommandBar);
    gtk_entry_set_position (GTK_ENTRY (mCommandBar), -1);
  }

  void ApvlvView::cmd_hide ()
  {
    if (mMainWindow == NULL)
      return;
    mHasCmd = FALSE;

    gtk_widget_set_usize (mCommandBar, mWidth, 1);
    mRootWindow->setsize (mWidth, adjheight ());

    gtk_widget_grab_focus (mMainWindow);
  }

  void ApvlvView::cmd_auto (const char *ps)
  {
    GCompletion *gcomp = NULL;

    stringstream ss (ps);
    string cmd, np;
    ss >> cmd >> np;

    if (cmd == "" || np == "")
      {
	return;
      }

    if (cmd == "o" || cmd == "open" || cmd == "TOtext")
      {
	gcomp = filecompleteinit (np.c_str ());
      }
    else if (cmd == "doc")
      {
	gcomp = g_completion_new (NULL);
	GList *list = g_list_alloc ();
	size_t i;
	for (i = 0; i < mDocs.size (); ++i)
	  {
	    list->data = g_strdup (((ApvlvCore *) mDocs[i])->filename ());
	    g_completion_add_items (gcomp, list);
	  }
	g_free (list);
      }

    if (gcomp != NULL)
      {
	char *comtext = NULL;
	debug ("find match: %s", np.c_str ());
	g_completion_complete (gcomp, np.c_str (), &comtext);
	if (comtext != NULL)
	  {
	    debug ("get a match: %s", comtext);
	    char text[0x100];
	    g_snprintf (text, sizeof text, ":%s %s", cmd.c_str (), comtext);
	    g_free (comtext);
	    gtk_entry_set_text (GTK_ENTRY (mCommandBar), text);
	    gtk_editable_set_position (GTK_EDITABLE (mCommandBar), -1);
	  }
	else
	  {
	    debug ("no get match");
	  }

	g_completion_free (gcomp);
      }
  }

  void ApvlvView::fullscreen ()
  {
    if (mHasFull == false)
      {
	gtk_window_maximize (GTK_WINDOW (mMainWindow));
	mHasFull = true;
      }
    else
      {
	gtk_window_unmaximize (GTK_WINDOW (mMainWindow));
	mHasFull = false;
      }
  }

  ApvlvDoc *ApvlvView::crtadoc ()
  {
    return (ApvlvDoc *) currentWindow ()->getCore ();
  }

  returnType ApvlvView::subprocess (int ct, guint key)
  {
    guint procmd = mProCmd;
    mProCmd = 0;
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
	if (ct == 0)
	  ct = 1;

	if (key == 't')
	  switchtab (mCurrTabPos + ct);
	else if (key == 'T')
	  switchtab (mCurrTabPos - ct);
	else if (key == 'g')
	  crtadoc ()->showpage (0);
	break;

      default:
	return NO_MATCH;
	break;
      }

    return MATCH;
  }

  returnType ApvlvView::process (int ct, guint key)
  {
    if (mProCmd != 0)
      {
	return subprocess (ct, key);
      }

    switch (key)
      {
      case CTRL ('w'):
	mProCmd = CTRL ('w');
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
	return NEED_MORE;
      default:
	return crtadoc ()->process (ct, key);
	break;
      }

    return MATCH;
  }

  void ApvlvView::run (const char *str)
  {
    switch (*str)
      {
      case SEARCH:
	crtadoc ()->markposition ('\'');
	crtadoc ()->search (str + 1);
	break;

      case BACKSEARCH:
	crtadoc ()->markposition ('\'');
	crtadoc ()->backsearch (str + 1);
	break;

      case COMMANDMODE:
	runcmd (str + 1);
	break;

      default:
	break;
      }
  }

  void ApvlvView::settitle (const char *title)
  {
    gtk_window_set_title (GTK_WINDOW (mMainWindow), title);
  }

  void ApvlvView::runcmd (const char *str)
  {
    if (*str == '!')
      {
	system (str + 1);
      }
    else
      {
	stringstream ss (str);
	string cmd, subcmd, argu;
	ss >> cmd >> subcmd >> argu;

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
	    loadfile (subcmd.c_str ());
	  }
	else if (cmd == "TOtext" && subcmd != "")
	  {
	    crtadoc ()->totext (subcmd.c_str ());
	  }
	else if ((cmd == "pr" || cmd == "print") && subcmd != "")
	  {
	    crtadoc ()->print (atoi (subcmd.c_str ()));
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
	    crtadoc ()->showpage (atoi (subcmd.c_str ()) - 1);
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
	    crtadoc ()->loadfile (helppdf);
	    crtadoc ()->showpage (7);
	  }
	else if ((cmd == "help" || cmd == "h") && subcmd == "prompt")
	  {
	    crtadoc ()->loadfile (helppdf);
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
	    if (isn && crtadoc ()
		&& atoi (cmd.c_str ()) != crtadoc ()->pagenumber ())
	      {
		crtadoc ()->showpage (atoi (cmd.c_str ()) - 1);
	      }
	  }
      }
  }

  void
    ApvlvView::apvlv_view_resized_cb (GtkWidget * wid, GtkAllocation * al,
				      ApvlvView * view)
  {
    int w, h;

    gtk_window_get_size (GTK_WINDOW (wid), &w, &h);
    w -= 5;
    h -= 5;
    if (w != view->mWidth || h != view->mHeight)
      {
	if (view->mHasCmd)
	  gtk_widget_set_usize (view->mCommandBar, w, 20);
	else
	  gtk_widget_set_usize (view->mCommandBar, w, 1);

	view->mWidth = w;
	view->mHeight = h;
	view->mRootWindow->setsize (w, view->adjheight ());
      }
  }

  gint
    ApvlvView::apvlv_view_keypress_cb (GtkWidget * wid, GdkEvent * ev,
				       ApvlvView * view)
  {
    if (view->mHasCmd == FALSE)
      {
	gCmds->append ((GdkEventKey *) ev);
	return TRUE;
      }

    return FALSE;
  }

  gint
    ApvlvView::apvlv_view_commandbar_cb (GtkWidget * wid, GdkEvent * ev,
					 ApvlvView * view)
  {
    if (view->mHasCmd == true)
      {
	view->mInHistroy = false;

	GdkEventKey *gek = (GdkEventKey *) ev;
	if (gek->keyval == GDK_Return)
	  {
	    gchar *str =
	      (gchar *) gtk_entry_get_text (GTK_ENTRY (view->mCommandBar));
	    if (str && strlen (str) > 0)
	      {
		view->run (str);
		view->mCmdHistroy.push_back (str);
		view->mCurrHistroy = view->mCmdHistroy.size () - 1;
	      }
	    view->cmd_hide ();
	    return TRUE;
	  }
	else if (gek->keyval == GDK_Tab)
	  {
	    gchar *str =
	      (gchar *) gtk_entry_get_text (GTK_ENTRY (view->mCommandBar));
	    if (str && strlen (str) > 0)
	      {
		view->cmd_auto (str + 1);
	      }
	    return TRUE;
	  }
	else if (gek->keyval == GDK_BackSpace)
	  {
	    gchar *str =
	      (gchar *) gtk_entry_get_text (GTK_ENTRY (view->mCommandBar));
	    if (str == NULL || strlen (str) == 1)
	      {
		view->cmd_hide ();
		return TRUE;
	      }
	  }
	else if (gek->keyval == GDK_Escape)
	  {
	    view->cmd_hide ();
	    return TRUE;
	  }
	else if (gek->keyval == GDK_Up)
	  {
	    view->mInHistroy = true;
	    gtk_entry_set_text (GTK_ENTRY (view->mCommandBar),
				view->mCurrHistroy > 0 ?
				view->mCmdHistroy[--view->mCurrHistroy].
				c_str () : view->mCmdHistroy[0].c_str ());
	    return TRUE;
	  }
	else if (gek->keyval == GDK_Down)
	  {
	    view->mInHistroy = true;
	    gtk_entry_set_text (GTK_ENTRY (view->mCommandBar),
				(size_t) view->mCurrHistroy <
				view->mCmdHistroy.size () -
				1 ? view->mCmdHistroy[++view->mCurrHistroy].
				c_str () : view->mCmdHistroy[view->
							     mCmdHistroy.
							     size () -
							     1].c_str ());
	    return TRUE;
	  }


	return FALSE;
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
					 GtkNotebookPage * page, guint pnum,
					 ApvlvView * view)
  {
    view->mCurrTabPos = pnum;
    view->mRootWindow = view->mTabList[pnum].root;
    ApvlvWindow::setcurrentWindow (NULL, view->mTabList[pnum].curr);
  }

  int ApvlvView::adjheight ()
  {
    int adj = 0;
    if (mHasTabs)
      adj += APVLV_TABS_HEIGHT;
    if (mHasCmd)
      adj += APVLV_CMD_BAR_HEIGHT;

    return mHeight - adj - 5;
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

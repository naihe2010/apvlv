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
/* @CFILE ApvlvUtil.cpp
 *
 *  Author: Alf <naihe2010@gmail.com>
 */
/* @date Created: 2008/09/30 00:00:00 Alf */

#include "ApvlvUtil.hpp"

#include <stdlib.h>
#include <sys/stat.h>
#include <gtk/gtk.h>

#ifdef WIN32
# include <windows.h>
#endif

#include <string>
#include <iostream>
#include <fstream>
using namespace std;

namespace apvlv
{

#ifdef WIN32
  string helppdf = "~\\Startup.pdf";
  string iniexam = "~\\apvlvrc.example";
  string iconreg = "~\\reg.png";
  string icondir = "~\\dir.png";
  string iconpdf = "~\\pdf.png";
  string inifile = "~\\_apvlvrc";
  string sessionfile = "~\\_apvlvinfo";
#else
  string helppdf = string (DOCDIR) + "/Startup.pdf";
  string iniexam = string (DOCDIR) + "/apvlvrc.example";
  string iconreg = string (DOCDIR) + "/reg.png";
  string icondir = string (DOCDIR) + "/dir.png";
  string iconpdf = string (DOCDIR) + "/pdf.png";
  string inifile = "~/.apvlvrc";
  string sessionfile = "~/.apvlvinfo";
#endif

  // Converts the path given to a absolute path.
  // Warning: The string is returned a new allocated buffer, NEED TO BE g_free
  char *absolutepath (const char *path)
  {
#ifndef PATH_MAX
#define PATH_MAX 4096
#endif
    char abpath[PATH_MAX];


    if (g_path_is_absolute (path))
      {
	return g_strdup (path);
      }

    if (*path == '~')
      {
#ifdef WIN32
	gchar *home =
	  g_win32_get_package_installation_directory_of_module (NULL);
#else
	char *home = getenv ("HOME");
#endif
	g_snprintf (abpath, sizeof abpath, "%s%s", home, ++path);
      }
    else
      {
#ifdef WIN32
	GetCurrentDirectoryA (sizeof abpath, abpath);
	strcat (abpath, "\\");
	strcat (abpath, path);
#else
	if (realpath (path, abpath) == NULL)
	  {
	    return NULL;
	  }
#endif
      }

    return g_strdup (abpath);
  }

  // Copy a file
  bool filecpy (const char *dst, const char *src)
  {
    gchar *content;
    gchar *s = absolutepath (src);
    gchar *d = absolutepath (dst);
    bool ok = false;

    gboolean ret = g_file_get_contents (s, &content, NULL, NULL);
    if (ret == TRUE)
      {
	ret = g_file_set_contents (d, content, -1, NULL);
	g_free (content);
	ok = ret;
      }

    g_free (s);
    g_free (d);

    return ok;
  }

  // replace a widget with a new widget
  // return the parent widget
  GtkWidget *replace_widget (GtkWidget * owid, GtkWidget * nwid)
  {
    GtkWidget *parent = gtk_widget_get_parent (owid);
    debug ("parent: %p, owid: %p, nwid: %p", parent, owid, nwid);
    gtk_container_remove (GTK_CONTAINER (parent), owid);
    gtk_container_add (GTK_CONTAINER (parent), nwid);
    gtk_widget_show_all (parent);
    return parent;
  }

  // get a PopplerDocument from a given file
  // return the pointer
  PopplerDocument *file_to_popplerdoc (const char *filename)
  {
    static gchar *mRawdata = NULL;
    static guint mRawdatasize = 0;
    gchar *wfilename;

    if (filename == NULL
	|| *filename == '\0'
	|| g_file_test (filename, G_FILE_TEST_IS_REGULAR) == FALSE
	|| (wfilename =
	    g_locale_from_utf8 (filename, -1, NULL, NULL, NULL)) == NULL)
      {
	errp ("filename error: %s", filename ? filename : "No name");
	return NULL;
      }

    size_t filelen;
    struct stat sbuf;
    int rt = stat (wfilename, &sbuf);
    if (rt < 0)
      {
	errp ("Can't stat the PDF file: %s.", filename);
	return NULL;
      }
    filelen = sbuf.st_size;

    if (mRawdata != NULL && mRawdatasize < filelen)
      {
	delete[]mRawdata;
	mRawdata = NULL;
      }

    if (mRawdata == NULL)
      {
	mRawdata = new char[filelen];
	mRawdatasize = filelen;
      }

    ifstream ifs (wfilename, ios::binary);
    if (ifs.is_open ())
      {
	ifs.read (mRawdata, filelen);
	ifs.close ();
      }

    g_free (wfilename);

    PopplerDocument *doc =
      poppler_document_new_from_data (mRawdata, filelen, NULL, NULL);

    if (doc == NULL
	//            && POPPLER_ERROR == POPPLER_ERROR_ENCRYPTED) /* fix this later */
      )
      {
	GtkWidget *dia = gtk_message_dialog_new (NULL,
						 GTK_DIALOG_DESTROY_WITH_PARENT,
						 GTK_MESSAGE_QUESTION,
						 GTK_BUTTONS_OK_CANCEL,
						 "Maybe this PDF file is encrypted, please input a password:");

	GtkWidget *entry = gtk_entry_new ();
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dia)->vbox), entry, TRUE,
			    TRUE, 10);
	gtk_widget_show (entry);

	int ret = gtk_dialog_run (GTK_DIALOG (dia));
	if (ret == GTK_RESPONSE_OK)
	  {
	    gchar *ans = (gchar *) gtk_entry_get_text (GTK_ENTRY (entry));
	    if (ans != NULL)
	      {
		doc =
		  poppler_document_new_from_data (mRawdata, filelen, ans,
						  NULL);
	      }
	  }

	gtk_widget_destroy (dia);
      }

    return doc;
  }

  void
    logv (const char *level, const char *file, int line, const char *func,
	  const char *ms, ...)
  {
    char p[0x1000], temp[0x100];
    va_list vap;

    g_snprintf (temp, sizeof temp, "[%s] %s: %d: %s(): ",
		level, file, line, func);

    va_start (vap, ms);
    vsnprintf (p, sizeof p, ms, vap);
    va_end (vap);

    cerr << temp << p << endl;
  }
}

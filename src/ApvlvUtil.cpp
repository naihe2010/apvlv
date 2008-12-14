/*
* This file is part of the apvlv package
*
* Copyright (C) 2008 Alf.
*
* Contact: YuPengda <naihe2010@gmail.com>
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
/* @CFILE ApvlvUtil.cpp
*
*  Author: Alf <naihe2010@gmail.com>
*/
/* @date Created: 2008/09/30 00:00:00 Alf */

#include "ApvlvUtil.hpp"

#include <stdlib.h>
#include <gtk/gtk.h>

#ifdef WIN32
# include <windows.h>
#endif

#include <string>
#include <iostream>
#include <fstream>
using namespace std;


#ifdef WIN32
#define snprintf _snprintf
#endif

namespace apvlv
{

#ifdef WIN32
  string helppdf = "~\\Startup.pdf";
  string iniexam = "~\\apvlvrc.example";
  string inifile = "~\\_apvlvrc";
  string sessionfile = "~\\_apvlvinfo";
#else
  string helppdf = string (PREFIX) + "/share/doc/apvlv/Startup.pdf";
  string iniexam = string (PREFIX) + "/share/doc/apvlv/apvlvrc.example";
  string inifile = "~/.apvlvrc";
  string sessionfile = "~/.apvlvinfo";
#endif

  // Converts the path given to a absolute path.
  // Warning: The string is returned a new allocated buffer, NEED TO BE g_free
  char *
    absolutepath (const char *path)
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
            gchar *home = g_win32_get_package_installation_directory_of_module (NULL);
#else
            char *home = getenv ("HOME");
#endif
            snprintf (abpath, sizeof abpath, "%s%s",
                      home,
                      ++ path);
          }
        else
          {
#ifdef WIN32
            GetCurrentDirectoryA (sizeof abpath, abpath);
            strcat (abpath, "\\");
            strcat (abpath, path);
#else
            realpath (path, abpath);
#endif
          }

        return g_strdup (abpath);
      }

  // Copy a file
  bool
    filecpy (const char *dst, const char *src)
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

  // remove a widget from its parent
  // return the parent widget
  GtkWidget *
    remove_widget (GtkWidget *wid, widremoveType remove)
      {
        if (remove == WR_REF)
          {
            g_object_ref (G_OBJECT (wid));
          }
        else if (remove == WR_REF_CHILDREN)
          {
            GList *children = gtk_container_get_children (GTK_CONTAINER (wid));
            GList *child = children;
            while (child != NULL)
              {
                g_object_ref (G_OBJECT (child->data));
                child = g_list_next (child);
              }
            g_list_free (children);
          }

        GtkWidget *parent = gtk_widget_get_parent (wid);
        gtk_container_remove (GTK_CONTAINER (parent), wid);
        return parent;
      }

  // replace a widget with a new widget
  // return the parent widget
  GtkWidget *
    replace_widget (GtkWidget *owid, GtkWidget *nwid, widremoveType remove)
      {
        GtkWidget *parent = remove_widget (owid, remove);
        gtk_container_add (GTK_CONTAINER (parent), nwid);
        return parent;
      }

  void
    logv (const char *level, const char *file, int line, const char *func, const char *ms, ...)
      {
        char p[0x1000], temp[0x100];
        va_list vap;

        snprintf (temp, sizeof temp, "[%s] %s: %d: %s(): ",
                  level, file, line, func);

        va_start (vap, ms);
        vsnprintf (p, sizeof p, ms, vap);
        va_end (vap);

        cerr << temp << p << endl;
      }
}

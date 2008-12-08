/****************************************************************************
 * Copyright (c) 1998-2005,2006 Free Software Foundation, Inc.              
 *                                                                          
 * Permission is hereby granted, free of charge, to any person obtaining a  
 * copy of this software and associated documentation files (the            
 * "Software"), to deal in the Software without restriction, including      
 * without limitation the rights to use, copy, modify, merge, publish,      
 * distribute, distribute with modifications, sublicense, and/or sell       
 * copies of the Software, and to permit persons to whom the Software is    
 * furnished to do so, subject to the following conditions:                 
 *                                                                          
 * The above copyright notice and this permission notice shall be included  
 * in all copies or substantial portions of the Software.                   
 *                                                                          
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS  
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF               
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.   
 * IN NO EVENT SHALL THE ABOVE COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,   
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR    
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR    
 * THE USE OR OTHER DEALINGS IN THE SOFTWARE.                               
 *                                                                          
 * Except as contained in this notice, the name(s) of the above copyright   
 * holders shall not be used in advertising or otherwise to promote the     
 * sale, use or other dealings in this Software without prior written       
 * authorization.                                                           
****************************************************************************/

/****************************************************************************
 *  Author:    YuPengda
 *  AuthorRef: Alf <naihe2010@gmail.com>
 *  Blog:      http://naihe2010.cublog.cn
****************************************************************************/
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
#ifdef WIN32
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

  // insert a widget after or before a widget
  void
    gtk_insert_widget_inbox (GtkWidget *prev, bool after, GtkWidget *n)
      {
        GtkWidget *parent = gtk_widget_get_parent (prev);
        gtk_box_pack_start (GTK_BOX (parent), n, TRUE, TRUE, 0);

        gint id = after? 1: 0;
        GList *children = gtk_container_get_children (GTK_CONTAINER (parent));
        for (GList *child = children; child != NULL; child = child->next)
          {
            if (child->data == prev) 
              {
                break;
              }
            else
              {
                id ++;
              }
          }
        g_list_free (children);

        gtk_box_reorder_child (GTK_BOX (parent), n, id);

        gtk_widget_show_all (parent);
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

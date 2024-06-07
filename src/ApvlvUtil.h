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
/* @CPPFILE ApvlvUtil.h
 *
 *  Author: Alf <naihe2010@126.com>
 */
/* @date Created: 2008/09/30 00:00:00 Alf */

#ifndef _APVLV_UTIL_H_
#define _APVLV_UTIL_H_

#include <gtk/gtk.h>

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <libxml/xpath.h>
using namespace std;

namespace apvlv
{
// Global files
extern string helppdf;
extern string mainmenubar_glade;
extern string iniexam;
extern string inifile;
extern string icondir;
extern string iconreg;
extern string iconpdf;
extern string sessionfile;

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

#ifdef WIN32
#define PATH_SEP_C '\\'
#define PATH_SEP_S "\\"
#else
#define PATH_SEP_C '/'
#define PATH_SEP_S "/"
#endif

int apvlv_system (const char *);

char *absolutepath (const char *path);

GtkWidget *replace_widget (GtkWidget *owid, GtkWidget *nwid);

void apvlv_widget_set_background (GtkWidget *wid);

bool apvlv_text_to_pixbuf_buffer (GString *content, int width, int height,
                                  double zoomrate, unsigned char *buffer,
                                  size_t buffer_size, int *o_width,
                                  int *o_height);

xmlNodeSetPtr xmldoc_get_nodeset (xmlDocPtr doc, const char *xpath,
                                  const char *pre, const char *ns);

xmlNodePtr xmldoc_get_node (xmlDocPtr doc, const char *xpath, const char *pre,
                            const char *ns);

string xmlnode_attr_get (xmlNodePtr node, const char *attr);

string filename_ext (const char *filename);

// command type
enum
{
  CMD_NONE,
  CMD_MESSAGE,
  CMD_CMD
};

// function return type
typedef enum
{
  MATCH,
  NEED_MORE,
  NO_MATCH,
} returnType;

// some windows macro
#ifdef WIN32
#include <winbase.h>
#include <wtypes.h>
#define usleep(x) Sleep ((x) / 1000)
#define __func__ __FUNCTION__
#define strcasecmp _strcmpi

#ifndef S_ISDIR
#define S_ISDIR(mode) (((mode) & 0170000) == (0040000))
#endif /*                                                                     \
        */

#endif

// log system
#if defined DEBUG || defined _DEBUG
#define debug(...) logv ("DEBUG", __FILE__, __LINE__, __func__, __VA_ARGS__)
#else
#define debug(...)
#endif
#define errp(...) logv ("ERROR", __FILE__, __LINE__, __func__, __VA_ARGS__)
void logv (const char *, const char *, int, const char *, const char *, ...);

// char macro
// because every unsigned char is < 256, so use this marco to stand for
// Ctrl+char, Shift+char
#define CTRL(c) ((c) + 256)

}
#endif

// Local Variables:
// mode: c++
// mode: c++
// End:

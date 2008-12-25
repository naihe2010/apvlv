/*
* This file is part of the apvlv package
*
* Copyright (C) 2008 Alf.
*
* Contact: Alf <naihe2010@gmail.com>
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
/* @CPPFILE ApvlvUtil.hpp
*
*  Author: Alf <naihe2010@gmail.com>
*/
/* @date Created: 2008/09/30 00:00:00 Alf */

#ifndef _APVLV_UTIL_H_
#define _APVLV_UTIL_H_

#ifdef HAVE_CONFIG_H
# include "config.hpp"
#endif

#include <gtk/gtk.h>
#include <string.h>

#include <iostream>
using namespace std;

namespace apvlv

{
  // Global files
  extern string helppdf;
  extern string iniexam;
  extern string inifile;
  extern string sessionfile;

#ifdef WIN32
#define PATH_SEP_C  '\\'
#define PATH_SEP_S  "\\"
#else
#define PATH_SEP_C  '/'
#define PATH_SEP_S  "/"
#endif

  char *absolutepath (const char *path);

  bool filecpy (const char *dst, const char *src);

  typedef enum
    {
      WR_REMOVE,               // destroy this direct
      WR_REF_CHILDREN,          // destroy this only, but ref its children
      WR_REF                    // only hide, that is, ref it
    } widremoveType;

  GtkWidget *remove_widget (GtkWidget *wid, widremoveType remove);

  GtkWidget *replace_widget (GtkWidget *owid, GtkWidget *nwid, widremoveType remove);

  // function return type
  typedef enum
    {
      MATCH,
      NEED_MORE,
      NO_MATCH,
    } returnType;

// some windows macro
#ifdef WIN32
#define snprintf _snprintf
#endif

#ifdef WIN32
#define __func__ ""
#endif

// log system
#if defined DEBUG || defined _DEBUG
#define debug(...)      logv ("DEBUG", __FILE__, __LINE__, __func__, __VA_ARGS__)
#define asst(s)         do {                    \
    if (!(s))                                   \
      {                                         \
        debug ("(%s) is FALSE, exit", #s);      \
        exit (1);                               \
      }                                         \
} while (0)
#else
#define debug(...)
#define asst(s)
#endif
#define info(...)       logv ("INFO", __FILE__, __LINE__, __func__, __VA_ARGS__)
#define warn(...)       logv ("WARNNING", __FILE__, __LINE__, __func__, __VA_ARGS__)
#define err(...)        logv ("ERROR", __FILE__, __LINE__, __func__, __VA_ARGS__)
  void logv (const char *, const char *, int, const char *, const char *, ...);

// char macro
// because every unsigned char is < 256, so use this marco to stand for Ctrl+char, Shift+char
#define VALIDCHAR(c)            ((c) < 256)
#define CTRL(c)                 ((c) + 256)
#define SHIFT(c)                (CTRL(c) + 256)

}
#endif

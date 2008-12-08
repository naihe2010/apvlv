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

  struct eqint
    {
      bool operator() (int a, int b) const
        {
          return a == b;
        }
    };

  struct eqstr
    {
      bool operator() (const char *sa, const char *sb) const
        {
          return strcmp (sa, sb) == 0;
        }
    };

  char *absolutepath (const char *path);

  bool filecpy (const char *dst, const char *src);

  void gtk_insert_widget_inbox (GtkWidget *prev, bool after, GtkWidget *n);

  // function return type
  typedef enum
    {
      MATCH,
      NEED_MORE,
      NO_MATCH,
    } returnType;

  // log system
#ifdef WIN32
#define __func__ ""
#endif

#ifdef DEBUG
#define debug(...)      logv ("DEBUG", __FILE__, __LINE__, __func__, __VA_ARGS__)
#else
#ifdef _DEBUG
#define debug(...)      logv ("DEBUG", __FILE__, __LINE__, __func__, __VA_ARGS__)
#else
#define debug(...)
#endif
#endif
#define info(...)       logv ("INFO", __FILE__, __LINE__, __func__, __VA_ARGS__)
#define warn(...)       logv ("WARNNING", __FILE__, __LINE__, __func__, __VA_ARGS__)
#define err(...)        logv ("ERROR", __FILE__, __LINE__, __func__, __VA_ARGS__)
  void logv (const char *, const char *, int, const char *, const char *, ...);

  // char macro
  // because every unsigned char is < 256, so use this marco to standrd for Ctrl-c
#define CTRL(c)         ((c) + 256)
}
#endif

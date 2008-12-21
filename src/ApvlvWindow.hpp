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
/* @CPPFILE ApvlvWindow.hpp
*
*  Author: Alf <naihe2010@gmail.com>
*/
/* @date Created: 2008/09/30 00:00:00 Alf */

#ifndef _APVLV_WINDOW_H_
#define _APVLV_WINDOW_H_

#ifdef HAVE_CONFIG_H
# include "config.hpp"
#endif

#include "ApvlvDoc.hpp"

#include <gtk/gtk.h>

#include <iostream>
using namespace std;

namespace apvlv
{
  class ApvlvDoc;

  class ApvlvWindow
    {
  public:
      ApvlvWindow (ApvlvDoc *doc);
      ~ApvlvWindow ();

      /* WE operate the AW_DOC window
       * Any AW_SP, AW_VSP are a virtual window, just for contain the AW_DOC window
       * AW_NONE is a empty window, need free
       * So, ANY user interface function can only get the AW_DOC window
       * */
      enum windowType { AW_SP, AW_VSP, AW_DOC, AW_NONE } type;

      ApvlvWindow *birth (bool vsp, ApvlvDoc *doc = NULL);

      ApvlvWindow *unbirth (ApvlvWindow *, ApvlvWindow *);

      bool istop ();

      void runcommand (int times, const char *, int argu);

      GtkWidget *widget ();

      ApvlvDoc *loadDoc (const char *filename);

      void setDoc (ApvlvDoc *doc);

      ApvlvDoc *getDoc (bool remove = false);

      void setsize (int wid, int hei);

      void smaller (int times = 1);
      void bigger (int times = 1);

      ApvlvWindow *getneighbor (int count, guint key);

      ApvlvWindow *getnext (int num);

      returnType process (int times, guint keyval);

      static void setcurrentWindow (ApvlvWindow *pre, ApvlvWindow *win);

      static void delcurrentWindow ();

      static ApvlvWindow *currentWindow ();

      ApvlvWindow *m_parent, *m_son, *m_daughter;

  private:
      inline ApvlvWindow *getkj (int num, bool next);
      inline ApvlvWindow *gethl (int num, bool next);

      inline gboolean resize_children ();

      static gboolean apvlv_window_resize_children_cb (gpointer data);

      static gboolean apvlv_window_paned_resized_cb (GtkWidget      *wid,
                                                     GdkEventButton *event,
                                                     ApvlvWindow    *win);

      static ApvlvWindow *m_curWindow;

      ApvlvDoc *mDoc;
      GtkWidget *mPaned;

      int mWidth, mHeight;
    };

}

#endif

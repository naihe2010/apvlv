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
/* @CPPFILE ApvlvWindow.h
 *
 *  Author: Alf <naihe2010@126.com>
 */
/* @date Created: 2008/09/30 00:00:00 Alf */

#ifndef _APVLV_WINDOW_H_
#define _APVLV_WINDOW_H_

#include "ApvlvCore.h"
#include "ApvlvDoc.h"

#include <gtk/gtk.h>

#include <iostream>
using namespace std;

namespace apvlv
{
    class ApvlvCore;

    class ApvlvWindow {
     public:
      explicit ApvlvWindow (ApvlvCore *core, ApvlvView *view);
      ~ApvlvWindow ();

      /* WE operate the AW_DOC window
       * Any AW_SP, AW_VSP are a virtual window, just for contain the AW_DOC window
       * AW_NONE is a empty window, need free
       * So, ANY user interface function can only get the AW_DOC window
       * */
      enum WindowType {
          AW_SP, AW_VSP, AW_CORE
      } mType;

      ApvlvWindow *birth (WindowType type, ApvlvCore *core);

      ApvlvWindow *unbirth ();

      GtkWidget *widget ();

      ApvlvCore *getCore ();

      ApvlvWindow *activeCoreWindow ();

      ApvlvWindow *getAncestor ();

      bool isAncestor () const;

      void setCore (ApvlvCore *core);

      void smaller (int times = 1);
      void bigger (int times = 1);

      void setcurrentWindow (ApvlvWindow *win);

      ApvlvWindow *getneighbor (int count, guint key);

      ApvlvWindow *getnext (int num);

      returnType process (int times, guint keyval);

      ApvlvWindow *m_parent, *m_child_1, *m_child_2;

     private:

      inline ApvlvWindow *getkj (__attribute__((unused)) int num, bool next);
      inline ApvlvWindow *gethl (__attribute__((unused)) int num, bool next);

      GtkWidget *mPaned;

      ApvlvCore *mCore;

      ApvlvView *mView;

      ApvlvWindow *mActiveWindow;
    };

}

#endif

/* Local Variables: */
/* mode: c++ */
/* End: */

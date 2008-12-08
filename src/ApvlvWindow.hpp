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

      ApvlvWindow *birth (ApvlvDoc *doc = NULL);

      ApvlvWindow *unbirth ();

      ApvlvWindow *insertafter (ApvlvWindow *bwin);

      ApvlvWindow *insertbefore (ApvlvWindow *awin);

      void runcommand (int times, const char *, int argu);

      GtkWidget *widget ();

      ApvlvDoc *loadDoc (const char *filename);

      void setDoc (ApvlvDoc *doc);

      ApvlvDoc *getDoc () { return m_Doc; }

      void setsize (int wid, int hei);

      ApvlvWindow * separate (bool vsp = false);

      void smaller (int times = 1);
      void bigger (int times = 1);

      ApvlvWindow *getneighbor (int count, guint key);

      returnType process (int times, guint keyval);

      static void setcurrentWindow (ApvlvWindow *pre, ApvlvWindow *win);

      static ApvlvWindow *currentWindow () { return m_curWindow; }

      enum windowType { AW_SP, AW_VSP, AW_DOC } type;

      ApvlvWindow *m_prev, *m_next, *m_parent, *m_child;

  private:
      inline ApvlvWindow *getkj (int num, bool next);
      inline ApvlvWindow *gethl (int num, bool next);
      inline ApvlvWindow *getnext (int num);

      inline void resize_children ();

      static gboolean apvlv_window_resize_children_cb (gpointer data);

      static gboolean apvlv_window_paned_resized_cb (GtkWidget      *wid, 
                                                     GdkEventButton *event,
                                                     ApvlvWindow    *win);

      static ApvlvWindow *m_curWindow;
      static int times;

      ApvlvDoc *m_Doc;

      GtkWidget *m_paned;

      int m_width, m_height;
    };

}

#endif

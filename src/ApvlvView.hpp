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
#ifndef _APVLV_VIEW_H_
#define _APVLV_VIEW_H_

#ifdef HAVE_CONFIG_H
# include "config.hpp"
#endif

#include "ApvlvDoc.hpp"
#include "ApvlvWindow.hpp"
#include "ApvlvCmds.hpp"
#include "ApvlvParams.hpp"

#include <iostream>

#include <gtk/gtk.h>

namespace apvlv
{
  class ApvlvDoc;
  class ApvlvWindow;

  class ApvlvView
    {
  public:
    ApvlvView (int argc, char *argv[]);
    ~ApvlvView ();

    void show ();

    GtkWidget *widget () { return mainwindow; }

    ApvlvWindow *getWindow () { return window; }

    void promptsearch ();
    void promptbacksearch ();
    void promptcommand ();

    void run (const char *str);

    bool loadfile (string file) { return loadfile (file.c_str ()); }
    bool loadfile (const char *filename);

    void open ();

    void quit () { apvlv_view_delete_cb (NULL, NULL, this); }

    void fullscreen ();

    void markposition (const char p) { crtadoc ()->markposition (p); }
    void jump (const char p) { crtadoc ()->jump (p); }

    bool reload () { return crtadoc ()->reload (); }

    void showpage (int p) { crtadoc ()->showpage (p - 1); }
    void nextpage (int times = 1) { crtadoc ()->nextpage (times); }
    void prepage (int times = 1) { crtadoc ()->prepage (times); }
    void halfnextpage (int times = 1) { crtadoc ()->halfnextpage (times); }
    void halfprepage (int times = 1) { crtadoc ()->halfprepage (times); }

    void scrollup (int times = 1) { crtadoc ()->scrollup (times); }
    void scrolldown (int times = 1) { crtadoc ()->scrolldown (times); }
    void scrollleft (int times = 1) { crtadoc ()->scrollleft (times); }
    void scrollright (int times = 1) { crtadoc ()->scrollright (times); }

    void setzoom (const char *s) { crtadoc ()->setzoom (s); }
    void setzoom (double d) { crtadoc ()->setzoom (d); }
    void zoomin () { crtadoc ()->zoomin (); }
    void zoomout () { crtadoc ()->zoomout (); }

    void dowindow (const char *s);

  private:
    ApvlvDoc *crtadoc () { return getWindow ()->getDoc (); }

    void parse_cmd (GdkEventKey * gek);

    void refresh ();

    void cmd_show ();

    void status_show ();

    enum 
      {
        SEARCH,
        BACKSEARCH,
        COMMANDMODE
      } cmd_mode;

    bool destroy;

    void runcmd (const char *cmd);

    gboolean cmd_has;

    GtkWidget *statusbar;
    GtkWidget *mainwindow;

    gboolean full_has;
    int width, height;

    static void apvlv_view_delete_cb (GtkWidget * wid, GtkAllocation * al,
                                      ApvlvView * view);
    static void apvlv_view_resized_cb (GtkWidget * wid, GtkAllocation * al,
                                       ApvlvView * view);
    static gint apvlv_view_keypress_cb (GtkWidget * wid, GdkEvent * ev);

    static gint apvlv_view_statusbar_cb (GtkWidget * wid, GdkEvent * ev);

    ApvlvWindow *window;

    map <string, ApvlvDoc *> m_Docs;
    vector <ApvlvWindow *> m_Windows;
    };

  extern ApvlvView *gView;
}

#endif

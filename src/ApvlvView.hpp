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
/* @CPPFILE ApvlvView.hpp
*
*  Author: Alf <naihe2010@gmail.com>
*/
/* @date Created: 2008/09/30 00:00:00 Alf */

#ifndef _APVLV_VIEW_H_
#define _APVLV_VIEW_H_

#ifdef HAVE_CONFIG_H
# include "config.hpp"
#endif

#include "ApvlvDoc.hpp"
#include "ApvlvWindow.hpp"

#include <iostream>

#include <gtk/gtk.h>

#include <list>

namespace apvlv
{
  typedef enum
    {
      SEARCH = '/',
      BACKSEARCH = '?',
      COMMANDMODE = ':'
    } cmd_mode_type;

  class ApvlvDoc;
  class ApvlvWindow;

  class ApvlvView
    {
  public:
    ApvlvView (int *argc, char ***argv);

    ~ApvlvView ();

    void show ();

    GtkWidget *widget ();

    ApvlvWindow *currentWindow ();

    void delcurrentWindow ();

    void promptcommand (char ch);

    void promptcommand (const char *str);

    void run (const char *str);

    bool loadfile (string file);

    bool loadfile (const char *filename);

    void open ();
    void close ();

    void quit ();

    void fullscreen ();

    returnType process (int times, guint keyval);

    returnType subprocess (int times, guint keyval);

    void cmd_show ();

    void cmd_hide ();

    void cmd_auto (const char *);

  private:
    ApvlvDoc *crtadoc ();

    void refresh ();

    bool destroy;

    GCompletion *filecompleteinit (const char *s);

    void runcmd (const char *cmd);

    void newtab ();

    int new_tabcontext (bool insertAfterCurr);

    void delete_tabcontext (int tabPos);

    void switch_tabcontext (int tabPos);

    // Caclulate number of pixels that the document should be.
    //  This figure accounts for decorations like (mCmdBar and mHaveTabs).
    // Returns a nonnegative number.
    int adjheight ();
  
    void switchtab (int tabPos);

    // Update the tab's context and update tab label.
    void windowadded ();
    
    void updatetabname ();
    bool mHasCmd, mHasTabs;

    guint mProCmd;

    GtkWidget *mCommandBar;
    GtkWidget *mMainWindow;

    GtkWidget *mTabContainer;

    struct TabEntry {
      ApvlvWindow *root;
      ApvlvWindow *curr;
      
      int numwindows;
      TabEntry (ApvlvWindow *_r, ApvlvWindow *_c, int _n) 
	: root(_r), curr(_c), numwindows(_n)
      { }
    };
    // possibly use GArray instead
    std::vector<TabEntry> mTabList;
    int mCurrTabPos;

    gboolean mHasFull;
    int mWidth, mHeight;


    static void apvlv_view_delete_cb (GtkWidget * wid, GtkAllocation * al,
                                      ApvlvView * view);
    static void apvlv_view_resized_cb (GtkWidget * wid, GtkAllocation * al,
                                       ApvlvView * view);
    static gint apvlv_view_keypress_cb (GtkWidget * wid, GdkEvent * ev);

    static gint apvlv_view_commandbar_cb (GtkWidget * wid, GdkEvent * ev);

    ApvlvWindow *mRootWindow, *mCurWindow;


    static const int APVLV_CMD_BAR_HEIGHT, APVLV_TABS_HEIGHT;
    };

  extern ApvlvView *gView;
}

#endif

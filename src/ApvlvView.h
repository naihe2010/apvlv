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
/* @CPPFILE ApvlvView.h
 *
 *  Author: Alf <naihe2010@126.com>
 */
/* @date Created: 2008/09/30 00:00:00 Alf */

#ifndef _APVLV_VIEW_H_
#define _APVLV_VIEW_H_

#include "ApvlvCmds.h"
#include "ApvlvCompletion.h"
#include "ApvlvDoc.h"
#include "ApvlvMenu.h"
#include "ApvlvWindow.h"

#include <gtk/gtk.h>

#include <iostream>
#include <list>

namespace apvlv
{
typedef enum
{
  SEARCH = '/',
  BACKSEARCH = '?',
  COMMANDMODE = ':',
  FIND = 'F'
} cmd_mode_type;

class ApvlvDoc;
class ApvlvWindow;

class ApvlvView
{
public:
  explicit ApvlvView (ApvlvView *);

  ~ApvlvView ();

  GtkWidget *widget ();

  ApvlvWindow *currentWindow ();

  void delcurrentWindow ();

  bool newtab (const char *filename, bool disable_content = false);

  bool newtab (ApvlvCore *core);

  __attribute__ ((unused)) bool newview (const char *filename, gint pn,
                                         bool disable_content = false);

  void promptcommand (char ch);

  void promptcommand (const char *str);

  void errormessage (const char *str, ...);

  void infomessage (const char *str, ...);

  static gchar *input (const char *str, int w = 400, int h = 150,
                       string content = "");

  bool run (const char *str);

  bool loadfile (const string &file);

  bool loadfile (const char *filename);

  bool loaddir (const char *path);

  ApvlvCore *hasloaded (const char *filename);

  void regloaded (ApvlvCore *);

  void open ();

  void opendir ();

  void quit ();

  void fullscreen ();

  returnType process (int hastimes, int times, guint keyval);

  returnType subprocess (int times, guint keyval);

  void cmd_show (int ct);

  void cmd_hide ();

  void cmd_auto (const char *);

  void settitle (const char *);

  ApvlvCore *crtadoc ();

  void append_child (ApvlvView *);

  void erase_child (ApvlvView *);

private:
  static ApvlvCompletion *filecompleteinit (const char *s);

  bool runcmd (const char *cmd);

  long new_tabcontext (ApvlvCore *core);

  void delete_tabcontext (long tabPos);

  void switch_tabcontext (long tabPos);

  void switchtab (long tabPos);

  // Update the tab's context and update tab label.
  void windowadded ();

  void updatetabname ();

  int mCmdType;

  guint mProCmd;
  int mProCmdCnt;

  GtkWidget *mMainWindow;

  ApvlvMenu *mMenu;

  GtkWidget *mViewBox;

  GtkWidget *mTabContainer;
  GtkWidget *mCommandBar;

  struct TabEntry
  {
    ApvlvWindow *mRootWindow;

    int mWindowCount;

    TabEntry (ApvlvWindow *_r, int _n) : mRootWindow (_r), mWindowCount (_n) {}
  };
  std::vector<TabEntry> mTabList;
  long mCurrTabPos;

  gboolean mHasFull;

  struct keyNode
  {
    ;
    int Has;
    int Ct;
    guint Key;
    bool End;
  };
  bool keyLastEnd;
  bool processInLast;
  vector<keyNode> keySquence;

  void saveKey (int has, int ct, guint key, bool end);

  void processLastKey ();

  static void apvlv_view_delete_cb (__attribute__ ((unused)) GtkWidget *wid,
                                    __attribute__ ((unused)) GtkAllocation *al,
                                    ApvlvView *view);
  static gint apvlv_view_keypress_cb (__attribute__ ((unused)) GtkWidget *wid,
                                      GdkEvent *ev, ApvlvView *view);

  static gint apvlv_view_commandbar_cb (__attribute__ ((unused))
                                        GtkWidget *wid,
                                        GdkEvent *ev, ApvlvView *view);

  static void apvlv_notebook_switch_cb (__attribute__ ((unused))
                                        GtkWidget *wid,
                                        __attribute__ ((unused))
                                        GtkNotebook *notebook,
                                        guint num, ApvlvView *view);

  ApvlvCmds mCmds;

  std::vector<ApvlvCore *> mDocs;

  std::vector<string> mCmdHistroy;
  size_t mCurrHistroy;

  ApvlvView *mParent;
  std::vector<ApvlvView *> mChildren;
};

}

#endif

/* Local Variables: */
/* mode: c++ */
/* End: */

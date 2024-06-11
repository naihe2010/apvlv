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

#include <QBoxLayout>
#include <QMainWindow>
#include <QTabWidget>
#include <iostream>
#include <list>

#include "ApvlvCmds.h"
#include "ApvlvCompletion.h"
#include "ApvlvDoc.h"
#include "ApvlvMenuAndTool.h"
#include "ApvlvWindow.h"

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

class ApvlvView : public QMainWindow
{
  Q_OBJECT
public:
  explicit ApvlvView (ApvlvView *view = nullptr);

  virtual ~ApvlvView ();

  ApvlvWindow *currentWindow ();

  void delCurrentWindow ();

  bool newtab (const string &filename, bool disable_content = false);

  bool newtab (ApvlvCore *core);

  void promptcommand (char ch);

  void promptcommand (const char *str);

  void errormessage (const char *str, ...);

  void infomessage (const char *str, ...);

  static char *input (const char *str, int w = 400, int h = 150,
                      string content = "");

  bool run (const char *str);

  bool loadfile (const string &filename);

  bool loaddir (const string &path);

  optional<ApvlvCore *> hasloaded (const string &abpath);

  void regloaded (ApvlvCore *);

  void open ();

  void opendir ();

  void quit ();

  void fullscreen ();

  returnType process (int hastimes, int times, uint keyval);

  returnType subprocess (int times, uint keyval);

  void cmd_show (int ct);

  void cmd_hide ();

  void cmd_auto (const char *);

  void settitle (const string &title);

  ApvlvCore *crtadoc ();

  void append_child (ApvlvView *);

  void erase_child (ApvlvView *);

private:
  static ApvlvCompletion *filecompleteinit (const char *s);

  bool runcmd (const char *cmd);

  int new_tabcontext (ApvlvCore *core);

  void delete_tabcontext (int tabPos);

  void switchtab (long tabPos);

  // Update the tab's context and update tab label.
  void windowadded ();

  void updatetabname ();

  int mCmdType;
  chrono::time_point<chrono::steady_clock> mCmdTime;

  uint mProCmd;
  int mProCmdCnt;

  unique_ptr<ApvlvMenuAndTool> mMenu;

  QFrame *mCentral;

  QTabWidget *mTabContainer;
  QLineEdit *mCommandBar;

  std::vector<ApvlvWindowContext *> mTabList;

  bool mHasFull;

  struct keyNode
  {
    ;
    int Has;
    int Ct;
    uint Key;
    bool End;
  };
  bool keyLastEnd;
  bool processInLast;
  vector<keyNode> keySquence;

  void saveKey (int has, int ct, uint key, bool end);

  void processLastKey ();

  void closeEvent (QCloseEvent *evt) override;
  void keyPressEvent (QKeyEvent *evt) override;

  ApvlvCmds mCmds;

  std::vector<unique_ptr<ApvlvCore> > mDocs;

  std::vector<string> mCmdHistroy;
  size_t mCurrHistroy;

  ApvlvView *mParent;
  std::vector<ApvlvView *> mChildren;

private slots:
  void commandbar_edit_cb (const QString &str);
  void commandbar_return_cb ();
  void notebook_switch_cb (int ind);
  void notebook_close_cb (int ind);
};

}

#endif

/* Local Variables: */
/* mode: c++ */
/* End: */

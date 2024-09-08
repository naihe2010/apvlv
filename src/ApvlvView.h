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

#ifndef _APVLV_VIEW_H_
#define _APVLV_VIEW_H_

#include <QBoxLayout>
#include <QLineEdit>
#include <QMainWindow>
#include <QTabWidget>
#include <iostream>
#include <list>

#include "ApvlvCmds.h"
#include "ApvlvCompletion.h"
#include "ApvlvFrame.h"
#include "ApvlvWindow.h"

namespace apvlv
{
enum CmdModeType
{
  SEARCH = '/',
  BACKSEARCH = '?',
  COMMANDMODE = ':',
  FIND = 'F'
};

class ApvlvFrame;
class ApvlvWindow;

class ApvlvCommandBar : public QLineEdit
{
  Q_OBJECT
public:
  ApvlvCommandBar () { installEventFilter (this); };

protected:
  void keyPressEvent (QKeyEvent *evt) override;
  bool eventFilter (QObject *obj, QEvent *event) override;

signals:
  void keyPressed (QKeyEvent *evt);
};

class ApvlvView : public QMainWindow
{
  Q_OBJECT
public:
  explicit ApvlvView (ApvlvView *view = nullptr);

  virtual ~ApvlvView ();

  ApvlvWindow *currentWindow ();

  void delCurrentWindow ();

  bool newtab (const string &filename, bool disable_content = false);

  bool newtab (ApvlvFrame *core);

  void promptcommand (char ch);

  void promptcommand (const char *str);

  void errormessage (const char *str, ...);

  void infomessage (const char *str, ...);

  static char *input (const char *str, int width = 400, int height = 150,
                      const string &content = "");

  bool run (const char *str);

  bool loadfile (const string &filename);

  bool loaddir (const string &path);

  optional<ApvlvFrame *> hasloaded (const string &abpath);

  void regloaded (ApvlvFrame *);

  ReturnType process (int hastimes, int times, uint keyval);

  ReturnType subprocess (int times, uint keyval);

  void cmd_show (int ct);

  void cmd_hide ();

  void cmd_auto (const char *);

  void settitle (const string &title);

  ApvlvFrame *crtadoc ();

  void append_child (ApvlvView *);

  void erase_child (ApvlvView *);

public slots:
  void loadFileOnPage (const string &filename, int pn);

  void open ();

  void opendir ();

  void quit (bool only_tab = true);

  void search ();

  void backSearch ();

  void advancedSearch ();

  void fullscreen ();

  void nextPage ();

  void previousPage ();

  void toggleContent ();

  void toggleToolBar ();

  void newtab ();

  void closetab ();

  void hsplit ();

  void vsplit ();

  void unbirth ();

private:
  void setupMenuBar ();

  void setupToolBar ();

  static ApvlvCompletion *getFileCompleteItems (const char *s);

  bool runcmd (const char *cmd);

  int new_tabcontext (ApvlvFrame *core);

  void delete_tabcontext (int tabPos);

  void switchtab (long tabPos);

  // Update the tab's context and update tab label.
  void windowadded ();

  void updatetabname ();

  int mCmdType;
  chrono::time_point<chrono::steady_clock> mCmdTime;

  uint mProCmd;
  int mProCmdCnt;

  QFrame *mCentral;

  QTabWidget *mTabContainer;
  ApvlvCommandBar *mCommandBar;

  QMenuBar *mMenuBar;
  QToolBar *mToolBar;

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

  std::vector<unique_ptr<ApvlvFrame> > mDocs;

  std::vector<string> mCmdHistroy;
  size_t mCurrHistroy;

  ApvlvView *mParent;
  std::vector<ApvlvView *> mChildren;

private slots:
  void commandbar_edit_cb (const QString &str);
  void commandbar_return_cb ();
  void commandbar_keypress_cb (QKeyEvent *gek);
  void notebook_switch_cb (int ind);
  void notebook_close_cb (int ind);
};

}

#endif

/* Local Variables: */
/* mode: c++ */
/* End: */

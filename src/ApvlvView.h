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

#include <QLineEdit>
#include <QMainWindow>
#include <QMenuBar>
#include <QTabWidget>
#include <QVBoxLayout>
#include <iosfwd>
#include <iostream>
#include <sstream>
#include <string_view>

#include "ApvlvCmds.h"
#include "ApvlvCompletion.h"
#include "ApvlvFrame.h"
#include "ApvlvSearchDialog.h"
#include "ApvlvWindow.h"

namespace apvlv
{

namespace CommandModeType
{
const char SEARCH = '/';
const char BACKSEARCH = '?';
const char COMMANDMODE = ':';
const char FIND = 'F';
}

class ApvlvFrame;
class ApvlvWindow;

class ApvlvCommandBar : public QLineEdit
{
  Q_OBJECT
public:
  ApvlvCommandBar ()
  {
    installEventFilter (this);
  };

protected:
  void keyPressEvent (QKeyEvent *evt) override;
  bool eventFilter (QObject *obj, QEvent *event) override;

signals:
  void keyPressed (QKeyEvent *evt);
};

class ApvlvView final : public QMainWindow
{
  Q_OBJECT
public:
  explicit ApvlvView (ApvlvView *view = nullptr);

  ~ApvlvView () override;

  ApvlvWindow *currentWindow ();

  void delCurrentWindow ();

  bool newTab (const std::string &filename);

  bool newTab (ApvlvFrame *core);

  void promptCommand (char ch);

  void promptCommand (const char *str);

  template <typename... T>
  void
  errorMessage (T... args)
  {
    std::stringstream msg;
    msg << "ERROR: ";
    msg << (... + args);
    mCommandBar.setText (QString::fromLocal8Bit (msg.str ()));
    cmdShow (CmdStatusType::CMD_MESSAGE);
  }

  static char *input (const char *str, int width = 400, int height = 150,
                      const std::string &content = "");

  bool run (const char *str);

  bool loadFile (const std::string &filename);

  bool loadDir (const std::string &path);

  std::optional<ApvlvFrame *> hasLoaded (std::string_view abpath);

  void regLoaded (ApvlvFrame *doc);

  CmdReturn process (int hastimes, int times, uint keyval);

  CmdReturn subProcess (int times, uint keyval);

  void cmdShow (CmdStatusType cmdtype);

  void cmdHide ();

  void cmdAuto (const char *str);

  void setTitle (const std::string &title);

  ApvlvFrame *currentFrame ();

  void appendChild (ApvlvView *view);

  void eraseChild (ApvlvView *view);

public slots:
  void loadFileOnPage (const std::string &filename, int pn);

  void open ();

  void openDir ();

  void openRrl ();

  void quit (bool only_tab = true);

  void search ();

  void backSearch ();

  void advancedSearch ();

  void dired ();

  void fullScreen ();

  void nextPage ();

  void previousPage ();

  void toggleDirectory ();

  void toggleToolBar ();

  void toggleStatus ();

  void newTab ();

  void closeTab ();

  void horizontalSplit ();

  void verticalSplit ();

  void unBirth ();

private:
  void setupMenuBar (const std::string &guiopt);

  void setupToolBar ();

  bool runCommand (const char *cmd);

  void switchTab (int tabPos);

  // Update the tab's context and update tab label.
  void windowAdded ();

  void updateTabName ();

  CmdStatusType mCmdType;
  std::chrono::time_point<std::chrono::steady_clock> mCmdTime;

  uint mProCmd;
  int mProCmdCnt;

  QFrame mCentral;
  QVBoxLayout mVBoxLayout;

  QTabWidget mTabContainer;
  ApvlvCommandBar mCommandBar;

  QMenuBar mMenuBar;
  QToolBar mToolBar;

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
  std::vector<keyNode> keySquence;

  void saveKey (int has, int ct, uint key, bool end);

  void processLastKey ();

  void closeEvent (QCloseEvent *evt) override;
  void keyPressEvent (QKeyEvent *evt) override;

  ApvlvCmds mCmds;

  SearchDialog mSearchDialog;

  std::vector<std::unique_ptr<ApvlvFrame>> mDocs;

  std::vector<std::string> mCmdHistroy;
  size_t mCurrHistroy;

  ApvlvView *mParent;
  std::vector<ApvlvView *> mChildren;

private slots:
  void commandbarEdited (const QString &str);
  void commandbarReturn ();
  void commandbarKeyPressed (QKeyEvent *gek);
  void tabSwitched (int ind);
};

}

#endif

/* Local Variables: */
/* mode: c++ */
/* End: */

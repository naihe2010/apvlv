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
/* @CPPFILE ApvlvCmds.h
 *
 *  Author: Alf <naihe2010@126.com>
 */

#ifndef _APVLV_CMDS_H_
#define _APVLV_CMDS_H_

#include <QKeyEvent>
#include <QTimer>
#include <map>
#include <vector>

using namespace std;

namespace apvlv
{
enum CmdType
{
  CT_CMD,
  CT_STRING,
  CT_STRING_RETURN
};

// command type
enum
{
  CMD_NONE,
  CMD_MESSAGE,
  CMD_CMD
};

// function return type
enum ReturnType
{
  MATCH,
  NEED_MORE,
  NO_MATCH,
};

// char macro
// because every unsigned char is < 256, so use this marco to stand for
// Ctrl+char, Shift+char
#define CTRL(c) ((c) + 256)

using StringKeyMap = map<string, int>;

class Command;
using CommandKeyList = vector<int>;
using CommandMap = map<CommandKeyList, Command *>;

class ApvlvView;
class Command
{
public:
  Command ();

  ~Command ();

  void process (ApvlvView *);

  void push (string_view s, CmdType type = CT_CMD);

  bool append (QKeyEvent *key);

  const char *append (const char *s);

  void type (CmdType type);

  CmdType type ();

  const char *c_str ();

  CommandKeyList *keyvalv_p ();

  CommandKeyList keyvalv ();

  void setPreCount (int precount);

  [[nodiscard]] int preCount () const;

  int keyval (uint id);

  Command *next ();

  void origin (Command *cmd);

  Command *origin ();

private:
  // command type
  CmdType mType;

  // if it has count
  bool mHasPreCount;

  // how to describe this command in .apvlvrc
  // like <C-d><C-w>, <S-b>s, or :run, :vsp, ...
  string mStrCommand;

  // key's value list
  CommandKeyList mKeyVals;

  // cmd's pre count
  int mPreCount;

  // next command
  Command *mNext;

  // when a key is map to other, this is the origin cmd.
  // after a mapped key was processed, return to this cmds
  Command *mOrigin;
};

class ApvlvCmds : public QObject
{
  Q_OBJECT
public:
  explicit ApvlvCmds (ApvlvView *view);

  ~ApvlvCmds () override;

  void append (QKeyEvent *gev);

  static void buildCommandMap (string_view os, string_view ms);

private:
  Command *process (Command *cmd);

  static ReturnType isMapCommand (CommandKeyList *ack);

  static Command *getMapCommand (Command *cmd);

  Command *mCmdHead;

  // command view
  ApvlvView *mView;

  enum cmdState
  {
    GETTING_COUNT,
    GETTING_CMD,
    CMD_OK,
  } mState;

  unique_ptr<QTimer> mTimeoutTimer;

  string mCountString;

private slots:
  void timeout_cb ();
};
}

#endif

// Local Variables:
// mode: c++
// End:

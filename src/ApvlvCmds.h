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

#include "ApvlvUtil.h"

using namespace std;

namespace apvlv
{
enum CmdType
{
  CT_CMD,
  CT_STRING,
  CT_STRING_RETURN
};

typedef map<const char *, int> StringKeyMap;

class ApvlvCmd;
typedef vector<int> ApvlvCmdKeyv;
typedef map<ApvlvCmdKeyv, ApvlvCmd *> ApvlvCmdMap;

class ApvlvView;
class ApvlvCmd
{
public:
  ApvlvCmd ();

  ~ApvlvCmd ();

  void process (ApvlvView *);

  void push (const char *s, CmdType type = CT_CMD);

  bool append (QKeyEvent *key);

  const char *append (const char *s);

  void type (CmdType type);

  CmdType type ();

  const char *c_str ();

  ApvlvCmdKeyv *keyvalv_p ();

  ApvlvCmdKeyv keyvalv ();

  void precount (int precount);

  [[nodiscard]] int precount () const;

  int keyval (uint id);

  ApvlvCmd *next ();

  void origin (ApvlvCmd *cmd);

  ApvlvCmd *origin ();

private:
  // command type
  CmdType mType;

  // if it has count
  bool mHasPreCount;

  // how to describe this command in .apvlvrc
  // like <C-d><C-w>, <S-b>s, or :run, :vsp, ...
  string mStrCommand;

  // key's value list
  ApvlvCmdKeyv mKeyVals;

  // cmd's pre count
  int mPreCount;

  // next command
  ApvlvCmd *mNext;

  // when a key is map to other, this is the origin cmd.
  // after a mapped key was processed, return to this cmds
  ApvlvCmd *mOrigin;
};

class ApvlvCmds : public QObject
{
  Q_OBJECT
public:
  explicit ApvlvCmds (ApvlvView *view);

  ~ApvlvCmds () override;

  void append (QKeyEvent *gev);

  static void buildmap (const char *os, const char *ms);

private:
  ApvlvCmd *process (ApvlvCmd *cmd);

  static ReturnType ismap (ApvlvCmdKeyv *ack);

  static ApvlvCmd *getmap (ApvlvCmd *cmd);

  ApvlvCmd *mCmdHead;

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

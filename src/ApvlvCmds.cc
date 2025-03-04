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
/* @CPPFILE ApvlvCmds.cc
 *
 *  Author: Alf <naihe2010@126.com>
 */

#include <Qt>
#include <cctype>
#include <cstring>

#include "ApvlvCmds.h"
#include "ApvlvView.h"

namespace apvlv
{
using namespace std;
using namespace Qt;

StringKeyMap Command::mKeyMap = {
  { "<BS>", Key_Backspace },      { "<Tab>", Key_Tab },
  { "<CR>", Key_Return },         { "<Esc>", Key_Escape },
  { "<Space>", Key_Space },       { "<lt>", Key_Less },
  { "<Bslash>", Key_Backslash },  { "<Bar>", Key_Bar },
  { "<Del>", Key_Delete },        { "<Up>", Key_Up },
  { "<Down>", Key_Down },         { "<Left>", Key_Left },
  { "<Right>", Key_Right },       { "<Help>", Key_Help },
  { "<Insert>", Key_Insert },     { "<Home>", Key_Home },
  { "<End>", Key_End },           { "<PageUp>", Key_PageUp },
  { "<PageDown>", Key_PageDown }, { "<KP_Up>", Key_Up },
  { "<KP_Down>", Key_Down },      { "<KP_Left>", Key_Left },
  { "<KP_Right>", Key_Right },    { "<KP_Prior>", Key_MediaPrevious },
  { "<KP_Next>", Key_MediaNext }, { "<KP_Home>", Key_Home },
  { "<KP_End>", Key_End },
};

CommandMap ApvlvCmds::mMaps;

static int
keyToControlChar (QKeyEvent *key)
{
  int char_key = key->key ();
  if (key->modifiers () & Qt::ShiftModifier)
    {
      char_key = toupper (char_key);
    }
  else
    {
      char_key = tolower (char_key);
    }
  if (key->modifiers () & ControlModifier)
    char_key = ctrlValue (char_key);

  return char_key;
}

constexpr static bool
isModifierKey (uint k)
{
  switch (k)
    {
    case Key_Shift:
    case Key_CapsLock:
    case Key_Meta:
    case Key_Alt:
    case Key_Super_L:
    case Key_Super_R:
    case Key_Hyper_L:
    case Key_Hyper_R:
    case Key_Control:
      return true;
    default:
      return false;
    }
}

Command::Command ()
    : mType (CmdType::CT_CMD), mHasPreCount (false), mPreCount (1),
      mOrigin (nullptr)
{
}

void
Command::type (CmdType type)
{
  mType = type;
}

CmdType
Command::type ()
{
  return mType;
}

void
Command::push (string_view sv, CmdType type)
{
  mType = type;

  mHasPreCount = false;
  mPreCount = 1;

  auto s = sv.data ();
  if (isdigit (*s))
    {
      mHasPreCount = true;
      mPreCount = (signed int)strtol (s, nullptr, 10);
      do
        {
          s++;
        }
      while (isdigit (*s));
    }

  if (*s == ':' || *s == '/' || *s == '?')
    {
      mStrCommand = s;
      mType = CmdType::CT_STRING;

      size_t off = mStrCommand.find ("<CR>");
      if (off != string::npos)
        {
          mStrCommand.erase (off, mStrCommand.length () - off);
          mType = CmdType::CT_STRING_RETURN;
          mNext = make_unique<Command> ();
          mNext->push (s + off + 4);
        }
      qDebug () << "set string type command: [" << mStrCommand << "]";
      return;
    }

  while (*s != '\0')
    {
      s = append (s);
    }
}

void
Command::process (ApvlvView *view)
{
  if (type () == CmdType::CT_STRING)
    {
      view->promptCommand (mStrCommand.c_str ());
    }
  else if (type () == CmdType::CT_STRING_RETURN)
    {
      view->run (mStrCommand.c_str ());
    }
  else
    {
      for (uint k = 0; k < keyVals ()->size (); ++k)
        {
          int key = keyval (k);
          if (key > 0)
            view->process (mHasPreCount, preCount (), keyval (k));
        }
    }

  if (next () != nullptr)
    {
      next ()->process (view);
    }
}

bool
Command::append (QKeyEvent *key)
{
  if (isModifierKey (key->key ()))
    return false;

  auto char_key = keyToControlChar (key);
  mKeyVals.push_back (char_key);
  return true;
}

const char *
Command::append (const char *s)
{
  size_t len;
  const char *e = strchr (s, '>');

  len = strlen (s);

  if (len >= 4 && *s == '<' && (*e != '\0' && *(s + 2) != '-'))
    {
      e++;
      for (const auto &it : mKeyMap)
        {
          if (it.first.compare (0, e - s, s) == 0)
            {
              mKeyVals.push_back (it.second);
              return e;
            }
        }
    }

  if (len >= 5 && s[0] == '<' && s[2] == '-' && s[4] == '>')
    {
      if (s[1] == 'C')
        {
          mKeyVals.push_back (ctrlValue (s[3]));
        }
      else
        {
          qWarning () << "Can't recognize the symbol: " << s;
        }
      return s + 5;
    }
  else
    {
      mKeyVals.push_back (s[0]);
      return s + 1;
    }
}

void
Command::setPreCount (int precount)
{
  mPreCount = precount;
  mHasPreCount = true;
}

int
Command::preCount () const
{
  return mPreCount;
}

void
Command::origin (Command *ori)
{
  mOrigin = ori;
}

Command *
Command::origin ()
{
  return mOrigin;
}

CommandKeyList *
Command::keyVals ()
{
  return &mKeyVals;
}

CommandKeyList
Command::keyvalv ()
{
  return mKeyVals;
}

Command *
Command::next ()
{
  return mNext.get ();
}

int
Command::keyval (uint id)
{
  return id >= mKeyVals.size () ? -1 : mKeyVals[id];
}

void
ApvlvCmds::buildCommandMap (string_view os, string_view ms)
{
  Command fir;
  fir.push (os);

  auto *secp = new Command ();
  secp->push (ms);

  for (auto &mMap : mMaps)
    {
      if (mMap.first == fir.keyvalv ())
        {
          delete mMap.second;
          mMap.second = secp;
          return;
        }
    }

  mMaps[fir.keyvalv ()] = secp;
}

ApvlvCmds::ApvlvCmds (ApvlvView *view)
{
  mView = view;

  mState = CmdState::CMD_OK;

  mTimeoutTimer = make_unique<QTimer> (this);
  QObject::connect (mTimeoutTimer.get (), SIGNAL (timeout ()), this,
                    SLOT (timeoutCallback ()));
}

ApvlvCmds::~ApvlvCmds ()
{
  if (mTimeoutTimer->isActive ())
    {
      mTimeoutTimer->stop ();
    }
}

void
ApvlvCmds::append (QKeyEvent *gev)
{
  if (mTimeoutTimer->isActive ())
    {
      mTimeoutTimer->stop ();
    }

  if (mState == CmdState::GETTING_CMD)
    {
      CommandKeyList v = mCmdHead->keyvalv ();
      v.push_back (keyToControlChar (gev));
      CmdReturn r = isMapCommand (&v);
      if (r == CmdReturn::NO_MATCH)
        {
          process (mCmdHead.get ());
          mCmdHead.release ();
          mState = CmdState::CMD_OK;
        }
    }

  if (mCmdHead == nullptr)
    mCmdHead = make_unique<Command> ();

  if (mState == CmdState::CMD_OK)
    {
      if (isdigit (int (gev->key ())) && gev->key () != '0')
        {
          auto c = char (gev->key ());
          mCountString += c;
          mState = CmdState::GETTING_COUNT;
          mTimeoutTimer->start (3000);
          return;
        }
    }

  else if (mState == CmdState::GETTING_COUNT)
    {
      if (isdigit (int (gev->key ())))
        {
          auto c = char (gev->key ());
          mCountString += c;
          mTimeoutTimer->start (3000);
          return;
        }
      else
        {
          if (!mCountString.empty ())
            {
              mCmdHead->setPreCount (
                  int (strtol (mCountString.c_str (), nullptr, 10)));
              mCountString = "";
            }
        }
    }

  bool valid = mCmdHead->append (gev);
  if (!valid)
    {
      mTimeoutTimer->start (3000);
      return;
    }

  mState = CmdState::GETTING_CMD;
  CmdReturn ret = isMapCommand (mCmdHead->keyVals ());
  if (ret == CmdReturn::NEED_MORE)
    {
      mTimeoutTimer->start (3000);
      return;
    }

  Command *pcmd;
  if (ret == CmdReturn::MATCH)
    {
      pcmd = getMapCommand (mCmdHead.get ());
      pcmd->origin (mCmdHead.get ());
      process (pcmd);
      pcmd->origin (nullptr);
      pcmd = nullptr;
    }
  else
    {
      pcmd = process (mCmdHead.get ());
    }

  mCmdHead.reset (pcmd);
  mState = CmdState::CMD_OK;
}

Command *
ApvlvCmds::process (Command *cmd)
{
  uint times = 1;
  Command *orig = cmd->origin ();
  if (orig != nullptr)
    {
      times = orig->preCount ();
    }

  for (uint i = 0; i < times; ++i)
    {
      cmd->process (mView);
    }
  return orig;
}

CmdReturn
ApvlvCmds::isMapCommand (CommandKeyList *ack)
{
  for (auto &mMap : mMaps)
    {
      if (*ack == mMap.first)
        {
          return CmdReturn::MATCH;
        }
      else
        {
          uint i;
          for (i = 0; i < ack->size (); ++i)
            {
              if ((*ack)[i] != mMap.first[i])
                break;
            }

          if (i == ack->size ())
            {
              return CmdReturn::NEED_MORE;
            }
        }
    }

  return CmdReturn::NO_MATCH;
}

Command *
ApvlvCmds::getMapCommand (Command *cmd)
{
  auto it = mMaps.find (*cmd->keyVals ());
  return it != mMaps.end () ? it->second : nullptr;
}

void
ApvlvCmds::timeoutCallback ()
{
  if (mCmdHead != nullptr)
    {
      process (mCmdHead.get ());
      mCmdHead.release ();
    }
  mState = CmdState::CMD_OK;
}
}

// Local Variables:
// mode: c++
// End:

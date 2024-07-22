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
/* @date Created: 2008/09/30 00:00:00 Alf */

#include <Qt>

#include "ApvlvCmds.h"
#include "ApvlvView.h"

namespace apvlv
{
using namespace Qt;

StringKeyMap SK;

static ApvlvCmdMap mMaps;

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
    char_key = CTRL (char_key);

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

ApvlvCmd::ApvlvCmd ()
{
  mType = CT_CMD;

  mHasPreCount = false;

  mPreCount = 1;

  mNext = nullptr;

  mOrigin = nullptr;
}

void
ApvlvCmd::type (CmdType type)
{
  mType = type;
}

CmdType
ApvlvCmd::type ()
{
  return mType;
}

void
ApvlvCmd::push (const char *s, CmdType type)
{
  mType = type;

  mHasPreCount = false;
  mPreCount = 1;

  mNext = nullptr;

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
      mType = CT_STRING;

      size_t off = mStrCommand.find ("<CR>");
      if (off != string::npos)
        {
          mStrCommand.erase (off, mStrCommand.length () - off);
          mType = CT_STRING_RETURN;
          mNext = new ApvlvCmd ();
          mNext->push (s + off + 4);
        }
      qDebug ("set string type command: [%s]", mStrCommand.c_str ());
      return;
    }

  while (*s != '\0')
    {
      s = append (s);
    }
}

ApvlvCmd::~ApvlvCmd () { delete mNext; }

void
ApvlvCmd::process (ApvlvView *view)
{
  if (type () == CT_STRING)
    {
      view->promptcommand (c_str ());
    }
  else if (type () == CT_STRING_RETURN)
    {
      view->run (c_str ());
    }
  else
    {
      for (uint k = 0; k < keyvalv_p ()->size (); ++k)
        {
          int key = keyval (k);
          if (key > 0)
            view->process (mHasPreCount, precount (), keyval (k));
        }
    }

  if (next () != nullptr)
    {
      next ()->process (view);
    }
}

bool
ApvlvCmd::append (QKeyEvent *key)
{
  if (isModifierKey (key->key ()))
    return false;

  auto char_key = keyToControlChar (key);
  mKeyVals.push_back (char_key);
  return true;
}

const char *
ApvlvCmd::append (const char *s)
{
  size_t len;
  char *e = strchr ((char *)s, '>');

  len = strlen (s);

  if (len >= 4 && *s == '<' && (*e != '\0' && *(s + 2) != '-'))
    {
      e++;
      for (auto &it : SK)
        {
          if (strncmp (it.first, s, e - s) == 0)
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
          mKeyVals.push_back (CTRL (s[3]));
        }
      /* commet as above
         else if (s[1] == 'S')
         {
         mKeyVals.push_back (SHIFT (s[3]));
         } */
      else
        {
          char ts[6];
          snprintf (ts, 6, "%s", s);
          qCritical ("Can't recognize the symbol: %s", ts);
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
ApvlvCmd::precount (int precount)
{
  mPreCount = precount;
  mHasPreCount = true;
}

int
ApvlvCmd::precount () const
{
  return mPreCount;
}

void
ApvlvCmd::origin (ApvlvCmd *ori)
{
  mOrigin = ori;
}

ApvlvCmd *
ApvlvCmd::origin ()
{
  return mOrigin;
}

const char *
ApvlvCmd::c_str ()
{
  return mStrCommand.c_str ();
}

ApvlvCmdKeyv *
ApvlvCmd::keyvalv_p ()
{
  return &mKeyVals;
}

ApvlvCmdKeyv
ApvlvCmd::keyvalv ()
{
  return mKeyVals;
}

ApvlvCmd *
ApvlvCmd::next ()
{
  return mNext;
}

int
ApvlvCmd::keyval (uint id)
{
  return id >= mKeyVals.size () ? -1 : mKeyVals[id];
}

void
ApvlvCmds::buildmap (const char *os, const char *ms)
{
  ApvlvCmd fir;
  fir.push (os);

  auto *secp = new ApvlvCmd ();
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

  mState = CMD_OK;

  mCmdHead = nullptr;

  if (SK.empty ())
    {
      SK["<BS>"] = Key_Backspace;
      SK["<Tab>"] = Key_Tab;
      SK["<CR>"] = Key_Return;
      SK["<Esc>"] = Key_Escape;
      SK["<Space>"] = Key_Space;
      SK["<lt>"] = Key_Less;
      SK["<Bslash>"] = Key_Backslash;
      SK["<Bar>"] = Key_Bar;
      SK["<Del>"] = Key_Delete;
      SK["<Up>"] = Key_Up;
      SK["<Down>"] = Key_Down;
      SK["<Left>"] = Key_Left;
      SK["<Right>"] = Key_Right;
      SK["<Help>"] = Key_Help;
      SK["<Insert>"] = Key_Insert;
      SK["<Home>"] = Key_Home;
      SK["<End>"] = Key_End;
      SK["<PageUp>"] = Key_PageUp;
      SK["<PageDown>"] = Key_PageDown;
      SK["<KP_Up>"] = Key_Up;
      SK["<KP_Down>"] = Key_Down;
      SK["<KP_Left>"] = Key_Left;
      SK["<KP_Right>"] = Key_Right;
      SK["<KP_Prior>"] = Key_MediaPrevious;
      SK["<KP_Next>"] = Key_MediaNext;
      SK["<KP_Home>"] = Key_Home;
      SK["<KP_End>"] = Key_End;
    }

  mTimeoutTimer = make_unique<QTimer> (this);
  QObject::connect (mTimeoutTimer.get (), SIGNAL (timeout ()), this,
                    SLOT (timeout_cb ()));
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

  if (mState == GETTING_CMD)
    {
      ApvlvCmdKeyv v = mCmdHead->keyvalv ();
      v.push_back (keyToControlChar (gev));
      ReturnType r = ismap (&v);
      if (r == NO_MATCH)
        {
          process (mCmdHead);
          delete mCmdHead;
          mCmdHead = nullptr;
          mState = CMD_OK;
        }
    }

  if (mCmdHead == nullptr)
    mCmdHead = new ApvlvCmd;

  if (mState == CMD_OK)
    {
      if (isdigit (int (gev->key ())) && gev->key () != '0')
        {
          char s[2] = { 0 };
          s[0] = char (gev->key ());
          mCountString += s;
          mState = GETTING_COUNT;
          mTimeoutTimer->start (3000);
          return;
        }
    }

  else if (mState == GETTING_COUNT)
    {
      if (isdigit (int (gev->key ())))
        {
          char s[2] = { 0 };
          s[0] = char (gev->key ());
          mCountString += s;
          mTimeoutTimer->start (3000);
          return;
        }
      else
        {
          if (!mCountString.empty ())
            {
              mCmdHead->precount (
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

  mState = GETTING_CMD;
  ReturnType ret = ismap (mCmdHead->keyvalv_p ());
  if (ret == NEED_MORE)
    {
      mTimeoutTimer->start (3000);
      return;
    }

  ApvlvCmd *pcmd;
  if (ret == MATCH)
    {
      pcmd = getmap (mCmdHead);
      pcmd->origin (mCmdHead);
      process (pcmd);
      pcmd->origin (nullptr);
      pcmd = nullptr;
    }
  else
    {
      pcmd = process (mCmdHead);
    }

  delete mCmdHead;
  mCmdHead = pcmd;
  mState = CMD_OK;
}

ApvlvCmd *
ApvlvCmds::process (ApvlvCmd *cmd)
{
  uint times = 1;
  ApvlvCmd *orig = cmd->origin ();
  if (orig != nullptr)
    {
      times = orig->precount ();
    }

  for (uint i = 0; i < times; ++i)
    {
      cmd->process (mView);
    }
  return orig;
}

ReturnType
ApvlvCmds::ismap (ApvlvCmdKeyv *cvp)
{
  for (auto &mMap : mMaps)
    {
      if (*cvp == mMap.first)
        {
          return MATCH;
        }
      else
        {
          uint i;
          for (i = 0; i < cvp->size (); ++i)
            {
              if ((*cvp)[i] != mMap.first[i])
                break;
            }

          if (i == cvp->size ())
            {
              return NEED_MORE;
            }
        }
    }

  return NO_MATCH;
}

ApvlvCmd *
ApvlvCmds::getmap (ApvlvCmd *cmd)
{
  auto it = mMaps.find (*cmd->keyvalv_p ());
  return it != mMaps.end () ? it->second : nullptr;
}

void
ApvlvCmds::timeout_cb ()
{
  if (mCmdHead != nullptr)
    {
      process (mCmdHead);
      delete mCmdHead;
      mCmdHead = nullptr;
    }
  mState = CMD_OK;
}
}

// Local Variables:
// mode: c++
// End:

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

#include "ApvlvCmds.h"
#include "ApvlvParams.h"
#include "ApvlvUtil.h"
#include "ApvlvView.h"

#include <gdk/gdkkeysyms.h>
#ifdef __linux__
#include <gdk/gdkx.h>
#endif

#include <cstdlib>
#include <sstream>

namespace apvlv
{
StringKeyMap SK;

static ApvlvCmdMap mMaps;

#define gek2guint(g)                                                          \
  ((g)->state == GDK_CONTROL_MASK ? CTRL ((g)->keyval) : (g)->keyval)

static inline bool
modifierKey (guint k)
{
  switch (k)
    {
    case GDK_KEY_Shift_L:
    case GDK_KEY_Shift_R:
    case GDK_KEY_Shift_Lock:
    case GDK_KEY_Meta_L:
    case GDK_KEY_Meta_R:
    case GDK_KEY_Alt_L:
    case GDK_KEY_Alt_R:
    case GDK_KEY_Super_L:
    case GDK_KEY_Super_R:
    case GDK_KEY_Hyper_L:
    case GDK_KEY_Hyper_R:
    case GDK_KEY_Control_L:
    case GDK_KEY_Control_R:
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
ApvlvCmd::type (cmdType type)
{
  mType = type;
}

cmdType
ApvlvCmd::type ()
{
  return mType;
}

void
ApvlvCmd::push (const char *s, cmdType type)
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
      debug ("set string type command: [%s]", mStrCommand.c_str ());
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
      for (guint k = 0; k < keyvalv_p ()->size (); ++k)
        {
          gint key = keyval (k);
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
ApvlvCmd::append (GdkEventKey *gek)
{
  if (modifierKey (gek->keyval))
    return false;

  if (gek->state & GDK_CONTROL_MASK)
    {
      mKeyVals.push_back (CTRL (gek->keyval));
    }
  else
    {
      mKeyVals.push_back (int (gek->keyval));
    }
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
          g_snprintf (ts, 6, "%s", s);
          errp ("Can't recognize the symbol: %s", ts);
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
ApvlvCmd::precount (gint precount)
{
  mPreCount = precount;
  mHasPreCount = true;
}

gint
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

gint
ApvlvCmd::keyval (guint id)
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

  mTimeoutTimer = 0;
  mState = CMD_OK;

  mCmdHead = nullptr;

  if (SK.empty ())
    {
      SK["<BS>"] = GDK_KEY_BackSpace;
      SK["<Tab>"] = GDK_KEY_Tab;
      SK["<CR>"] = GDK_KEY_Return;
      SK["<Esc>"] = GDK_KEY_Escape;
      SK["<Space>"] = GDK_KEY_space;
      SK["<lt>"] = GDK_KEY_less;
      SK["<Bslash>"] = GDK_KEY_backslash;
      SK["<Bar>"] = GDK_KEY_bar;
      SK["<Del>"] = GDK_KEY_Delete;
      SK["<Up>"] = GDK_KEY_Up;
      SK["<Down>"] = GDK_KEY_Down;
      SK["<Left>"] = GDK_KEY_Left;
      SK["<Right>"] = GDK_KEY_Right;
      SK["<Help>"] = GDK_KEY_Help;
      SK["<Insert>"] = GDK_KEY_Insert;
      SK["<Home>"] = GDK_KEY_Home;
      SK["<End>"] = GDK_KEY_End;
      SK["<PageUp>"] = GDK_KEY_Page_Up;
      SK["<PageDown>"] = GDK_KEY_Page_Down;
      SK["<KP_Up>"] = GDK_KEY_KP_Up;
      SK["<KP_Down>"] = GDK_KEY_KP_Down;
      SK["<KP_Left>"] = GDK_KEY_KP_Left;
      SK["<KP_Right>"] = GDK_KEY_KP_Right;
      SK["<KP_Prior>"] = GDK_KEY_KP_Prior;
      SK["<KP_Next>"] = GDK_KEY_KP_Next;
      SK["<KP_Home>"] = GDK_KEY_KP_Home;
      SK["<KP_End>"] = GDK_KEY_KP_End;
    }
}

ApvlvCmds::~ApvlvCmds ()
{
  if (mTimeoutTimer > 0)
    {
      g_source_remove (mTimeoutTimer);
      mTimeoutTimer = 0;
    }
}

void
ApvlvCmds::append (GdkEventKey *gev)
{
  if (mTimeoutTimer > 0)
    {
      g_source_remove (mTimeoutTimer);
      mTimeoutTimer = 0;
    }

  if (mState == GETTING_CMD)
    {
      ApvlvCmdKeyv v = mCmdHead->keyvalv ();
      v.push_back (gek2guint (gev));
      returnType r = ismap (&v);
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
      if (isdigit (int (gev->keyval)) && gev->keyval != '0')
        {
          char s[2] = { 0 };
          s[0] = char (gev->keyval);
          mCountString += s;
          mState = GETTING_COUNT;
          mTimeoutTimer = g_timeout_add (3000, apvlv_cmds_timeout_cb, this);
          return;
        }
    }

  else if (mState == GETTING_COUNT)
    {
      if (isdigit (int (gev->keyval)))
        {
          char s[2] = { 0 };
          s[0] = char (gev->keyval);
          mCountString += s;
          mTimeoutTimer = g_timeout_add (3000, apvlv_cmds_timeout_cb, this);
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
      mTimeoutTimer = g_timeout_add (3000, apvlv_cmds_timeout_cb, this);
      return;
    }

  mState = GETTING_CMD;
  returnType ret = ismap (mCmdHead->keyvalv_p ());
  if (ret == NEED_MORE)
    {
      mTimeoutTimer = g_timeout_add (3000, apvlv_cmds_timeout_cb, this);
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
  guint times = 1;
  ApvlvCmd *orig = cmd->origin ();
  if (orig != nullptr)
    {
      times = orig->precount ();
    }

  for (guint i = 0; i < times; ++i)
    {
      cmd->process (mView);
    }
  return orig;
}

returnType
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
          guint i;
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

gboolean
ApvlvCmds::apvlv_cmds_timeout_cb (gpointer data)
{
  auto *cmds = (ApvlvCmds *)data;
  if (cmds->mCmdHead != nullptr)
    {
      cmds->process (cmds->mCmdHead);
      delete cmds->mCmdHead;
      cmds->mCmdHead = nullptr;
    }
  cmds->mState = CMD_OK;
  return FALSE;
}
}

// Local Variables:
// mode: c++
// End:

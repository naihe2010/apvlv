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
*/

/* @CPPFILE ApvlvCmds.cpp
*
*  Author: Alf <naihe2010@gmail.com>
*/
/* @date Created: 2008/09/30 00:00:00 Alf */

#include "ApvlvParams.hpp"
#include "ApvlvView.hpp"
#include "ApvlvUtil.hpp"
#include "ApvlvCmds.hpp"

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#ifndef WIN32
#include <gdk/gdkx.h>
#endif

#include <sstream>

namespace apvlv
{
  ApvlvCmds *gCmds = NULL;

  StringKeyMap SK;

#define showv(v) do \
    {\
      debug ("%s's size: %d", #v, (v).size ()); \
      for (unsigned int i=0; i<(v).size (); ++i) \
        {       \
          debug ("%s's %dth elm: %c[%d]", #v, i, (v)[i], (v)[i]); \
        } \
    } \
  while (0)

#define gek2guint(g)    ((g)->state == GDK_CONTROL_MASK? CTRL ((g)->keyval): (g)->keyval)

  ApvlvCmd::ApvlvCmd ()
    {
      mType = CT_CMD;

      mBeMap = false;
      mCanMap = true;

      mHasPreCount = false;

      mStrCommand[0] = '\0';

      mPreCount = 1;

      mNext = NULL;

      mOrigin = NULL;
    }

  ApvlvCmd::ApvlvCmd (ApvlvCmd &cmd)
    {
      mNext = new ApvlvCmd (*cmd.next ());
      if (cmd.canmap () == false)
        {
        }
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
      asst (s);

      mType = type;

      mHasPreCount = false;
      mPreCount = 1;

      if (*s == '-'
          || isdigit (*s))
        {
          mHasPreCount = true;
          mPreCount = atoi (s);
          do 
            {
              s ++;
            } 
          while (isdigit (*s));
        }

      if (*s == ':'
          || *s == '/'
          || *s == '?')
        {
          mStrCommand.push_back (*s);
          mKeyVals.push_back (*s);
          mNext = new ApvlvCmd;
          mNext->push (++ s, CT_STRING);
          return;
        }

      if (type == CT_STRING)
        {
          mStrCommand = s;
          char *p = strstr (mStrCommand.c_str (), "<CR>");
          if (p != NULL)
            {
              *p = '\0';
              mType = CT_STRING_RETURN;
            }
          debug ("set return string type command: [%s]", mStrCommand.c_str ());
          return;
        }

      while (*s != '\0')
        {
          s = append (s);
        }
    }

  ApvlvCmd::~ApvlvCmd ()
    {
      delete mNext;
    }

  void
    ApvlvCmd::append (GdkEventKey *gek)
      {
        if (gek->state == GDK_CONTROL_MASK)
          {
            mKeyVals.push_back (CTRL (gek->keyval));
          }
        /*
           else if (gek->state == GDK_SHIFT_MASK 
           && (! isupper (gek->keyval)))
           {
        // hack, if a char is not a Uppercase chracter, we use S- mark
        mKeyVals.push_back (SHIFT (gek->keyval));
        } 
        */
        else
          {
            mKeyVals.push_back (gek->keyval);
          }
      }

  const char *
    ApvlvCmd::append (const char *s)
      {
      size_t len = strlen (s);

      asst (len);
      for (unsigned int i=1; i<len; ++i)
        {
          string ss (s, i);
          StringKeyMap::iterator it;
          it = SK.find (ss.c_str ());
          if (it != SK.end ())
            {
              mKeyVals.push_back (it->second);
              return s + i;
            }
        }

      if (len >= 5
               && s[0] == '<'
               && s[2] == '-'
               && s[4] == '>'
      )
        {
          if (s[1] == 'C')
            {
              mKeyVals.push_back (CTRL (s[3]));
            }
          else if (s[1] == 'S')
            {
              mKeyVals.push_back (SHIFT (s[3]));
            }
          else 
            {
              asst (0);
            }
          return s + 5;
        }
      else if (len >= 1)
        {
          mKeyVals.push_back (s[0]);
          return s + 1;
        }

      asst (0);
      }

  void 
    ApvlvCmd::bemap (bool bemap)
      {
        mBeMap = bemap;
      }

    bool 
      ApvlvCmd::bemap ()
        {
          return mBeMap;
        }

    void 
      ApvlvCmd::canmap (bool canmap)
        {
          mCanMap = canmap;
        }

    bool 
      ApvlvCmd::canmap ()
        {
          return mCanMap;
        }

    void 
      ApvlvCmd::hascount (bool hascount)
        {
          mHasPreCount = hascount;
        }

    bool 
      ApvlvCmd::hascount ()
        {
          return mHasPreCount;
        }

    void 
      ApvlvCmd::precount (gint precount)
        {
          mPreCount = precount;
        }

    gint
      ApvlvCmd::precount ()
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

  guint
    ApvlvCmd::keyval (guint id)
      {
        return id >= mKeyVals.size ()? -1: mKeyVals[id];
      }

  bool
    ApvlvCmds::buildmap (const char *os, const char *ms)
      {
        ApvlvCmd fir;
        fir.push (os);

        ApvlvCmd *secp = new ApvlvCmd;
        secp->push (ms);

        ApvlvCmdMap::iterator it;
        for (it = mMaps.begin (); it != mMaps.end (); ++ it)
          {
            if (it->first == fir.keyvalv ())
              {
                break;
              }
          }

        if (it != mMaps.end ())
          {
            delete it->second;
            it->second = secp;
          }
        else
          {
            mMaps[fir.keyvalv ()] = secp;
          }
        return true;
      }

  bool
    ApvlvCmds::buildevent (const char *p)
      {
      m_keys.clear ();

      string ss (p);
      while (! ss.empty ())
        {
          GdkEventKey gk;

          if (ss[0] == '<')
            {
              size_t p = ss.find ('>');
              if (p != string::npos)
                {
                  string ss2 (ss.c_str (), p + 1);
                  int r = translate (ss2, &gk);
                  if (r > 0)
                    {
                      ss.erase (0, p + 1);
                      goto deft;
                    }
                }
            }

          gk.state = 0;
          gk.keyval = gdk_unicode_to_keyval (ss[0]);
          ss.erase (0, 1);

deft:
          gk.type = GDK_KEY_PRESS;
          gk.window = gView->widget ()->window;
          gk.send_event = FALSE;
          gk.time = GDK_CURRENT_TIME;
          gk.length = 0;
          gk.string = "";
#ifndef WIN32
          gk.hardware_keycode = XKeysymToKeycode (GDK_WINDOW_XDISPLAY (gk.window), gk.keyval);
#endif
          gk.group = 0;
          gk.is_modifier = 0;

          m_keys.push_back (gk);
        }
      return true;
    }

  void
    ApvlvCmds::destroyevent ()
      {
        if (sendtimer > 0)
          g_source_remove (sendtimer);
        m_keys.clear ();
      }

  void
    ApvlvCmds::sendevent ()
      {
        if (! m_keys.empty ())
          {
            GdkEventKey *gev = (GdkEventKey *) &(m_keys[0]);
            gdk_event_put ((GdkEvent *) gev);

            gev->type = GDK_KEY_RELEASE;
            gdk_event_put ((GdkEvent *) gev);

            vector <GdkEventKey>::iterator it = m_keys.begin ();
            m_keys.erase (it);
          }

        if (! m_keys.empty ())
          sendtimer = g_timeout_add (100, ae_send_next, this);
      }

  gboolean
    ApvlvCmds::ae_send_next (void *data)
      {
        ApvlvCmds *ac = (ApvlvCmds *) data;
        ac->sendevent ();
        return TRUE;
      }

  ApvlvCmds::ApvlvCmds ()
    {
      timeouttimer = -1;
      state = CMD_OK;

      mCmdHead = NULL;

      if (SK.empty ())
        {
          SK["<BS>"] = GDK_BackSpace;
          SK["<Tab>"] = GDK_Tab;
          SK["<CR>"] = GDK_Return;
          SK["<Esc>"] = GDK_Escape;
          SK["<Space>"] = GDK_space;
          SK["<lt>"] = GDK_less;
          SK["<Bslash>"] = GDK_backslash;
          SK["<Bar>"] = GDK_bar;
          SK["<Del>"] = GDK_Delete;
          SK["<Up>"] = GDK_Up;
          SK["<Down>"] = GDK_Down;
          SK["<Left>"] = GDK_Left;
          SK["<Right>"] = GDK_Right;
          SK["<Help>"] = GDK_Help;
          SK["<Insert>"] = GDK_Insert;
          SK["<Home>"] = GDK_Home;
          SK["<End>"] = GDK_End;
          SK["<PageUp>"] = GDK_Page_Up;
          SK["<PageDown>"] = GDK_Page_Down;
        }

      if (KS.empty ())
        {
          KS[GDK_BackSpace] = "<BS>";
          KS[GDK_Tab] = "<Tab>";
          KS[GDK_Return] = "<CR>";
          KS[GDK_Escape] = "<Esc>";
          KS[GDK_space] = "<Space>";
          KS[GDK_less] = "<lt>";
          KS[GDK_backslash] = "<Bslash>";
          KS[GDK_bar] = "<Bar>";
          KS[GDK_Delete] = "<Del>";
          KS[GDK_Up] = "<Up>";
          KS[GDK_Down] = "<Down>";
          KS[GDK_Left] = "<Left>";
          KS[GDK_Right] = "<Right>";
          KS[GDK_Help] = "<Help>";
          KS[GDK_Insert] = "<Insert>";
          KS[GDK_Home] = "<Home>";
          KS[GDK_End] = "<End>";
          KS[GDK_Page_Up] = "<PageUp>";
          KS[GDK_Page_Down] = "<PageDown>";
        }
    }

  ApvlvCmds::~ApvlvCmds ()
    {
      if (timeouttimer > 0)
        {
          g_source_remove (timeouttimer);
          timeouttimer = -1;
        }

      destroyevent ();
    }

  void
    ApvlvCmds::append (GdkEventKey *gev)
      {
        if (timeouttimer > 0)
          {
            g_source_remove (timeouttimer);
            timeouttimer = -1;
          }

        if (state == GETTING_CMD) 
          {
            asst (mCmdHead);
            ApvlvCmdKeyv v = mCmdHead->keyvalv ();
            v.push_back (gek2guint (gev));
            returnType r = ismap (&v);
            if (r == NO_MATCH)
              {
                process (mCmdHead);
                delete mCmdHead;
                mCmdHead = NULL;
                state = CMD_OK;
              }
          }

        if (mCmdHead == NULL)
          mCmdHead = new ApvlvCmd;

        if (state == CMD_OK)
          {
            if (gev->keyval == '-'
                || (isdigit (gev->keyval)))
              {
                char s[2] = { 0 };
                s[0] = gev->keyval;
                count += s;
                state = GETTING_COUNT;
                timeouttimer = g_timeout_add (3000, apvlv_cmds_timeout_cb, this);
                return;
              }
          }

        else if (state == GETTING_COUNT)
          {
            if (gev->keyval == '-'
                || (isdigit (gev->keyval)))
              {
                char s[2] = { 0 };
                s[0] = gev->keyval;
                count += s;
                timeouttimer = g_timeout_add (3000, apvlv_cmds_timeout_cb, this);
                return;
              }
            else
              {
                if (count.size () > 0)
                  {
                    mCmdHead->precount (atoi (count.c_str ()));
                    count = "";
                  }
              }
          }

        state = GETTING_CMD;
        mCmdHead->append (gev);
        returnType ret = ismap (mCmdHead->keyvalv_p ());
        if (ret == NEED_MORE)
          {
            timeouttimer = g_timeout_add (3000, apvlv_cmds_timeout_cb, this);
            return;
          }

        ApvlvCmd *pcmd = NULL;
        if (ret == MATCH)
          {
            ApvlvCmd *pcmd = getmap (mCmdHead);
            process (pcmd);
            pcmd = NULL;
          }
        else if (ret == NO_MATCH)
          {
            pcmd = process (mCmdHead);
          }
        else
          {
            asst (0);
          }

        delete mCmdHead;
        mCmdHead = pcmd;
        state = CMD_OK;
      }

  ApvlvCmd *
    ApvlvCmds::process (ApvlvCmd *cmd)
      {
        guint times = 1;
        ApvlvCmd *orig = cmd->origin ();
        if (orig != NULL)
          {
            times = orig->precount ();
          }

        for (guint i=0; i<times; ++ i)
          {
            ApvlvCmd *ncmd = cmd->next ();
            if (ncmd != NULL)
              {
                if (ncmd->type () != CT_CMD)
                  {
                    if (cmd->keyval (0) == ':'
                        || cmd->keyval (0) == '/'
                        || cmd->keyval (0) == '?')
                      {
                        debug ("run this");
                        gView->run (cmd_mode_type (cmd->keyval (0)), ncmd->c_str ());
                        return orig;
                      }
                  }

                debug ("Can't run this map command, as default");
                for (guint k=0; k<cmd->keyvalv_p()->size (); ++k)
                  gView->process (cmd->precount (), cmd->keyval (k));
                for (guint k=0; k<ncmd->keyvalv_p()->size (); ++k)
                  gView->process (ncmd->precount (), ncmd->keyval (k));
              }
            else
              {
                for (guint k=0; k<cmd->keyvalv_p()->size (); ++k)
                  gView->process (cmd->precount (), cmd->keyval (k));
              }
          }
        return orig;
      }

  returnType 
    ApvlvCmds::ismap (ApvlvCmdKeyv *cvp)
      {
        ApvlvCmdMap::iterator it;

        for (it = mMaps.begin (); it != mMaps.end (); ++ it)
          {
            if (*cvp == it->first)
              {
                return MATCH;
              }
            else
              {
                guint i;
                for (i=0; i<cvp->size (); ++i)
                  {
                    if ((*cvp)[i] != it->first[i])
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
    ApvlvCmds::getmap (const char *os)
      {
        ApvlvCmd cmd;
        cmd.push (os);
        return getmap (&cmd);
      }

  ApvlvCmd *
    ApvlvCmds::getmap (ApvlvCmd *cmd)
      {
        ApvlvCmdMap::iterator it;
        it = mMaps.find (*cmd->keyvalv_p ());
        return it != mMaps.end ()? it->second: NULL;
      }

  void
    ApvlvCmds::translate (GdkEventKey *gev, string *s)
      {
        if (gev->state == GDK_CONTROL_MASK)
          {
            guint ret = gdk_keyval_to_unicode (gev->keyval);
            if (isprint (ret))
              {
                s->append ("<C-");
                s->push_back (ret);
                s->append (">");
              }
          }
        else
          {
            if (KS[gev->keyval] != NULL)
              {
                s->append (KS[gev->keyval]);
              }
            else
              {
                guint ret = gdk_keyval_to_unicode (gev->keyval);
                if (isprint (ret))
                  {
                    s->push_back (ret);
                  }
              }
          }
      }

  bool
    ApvlvCmds::translate (string &s, GdkEventKey *gev)
      {
        if (s.size () > 4
            && s[1] == 'C'
            && s[2] == '-'
        )
          {
            gev->state = GDK_CONTROL_MASK;
            gev->keyval = gdk_unicode_to_keyval (s[3]);
            return true;
          }

        KeyStringMap::iterator it;
        for (it = KS.begin (); it != KS.end (); ++ it)
          {
            if (it->second == NULL) continue;
            if (strcmp (it->second, s.c_str ()) == 0)
              {
                gev->state = 0;
                gev->keyval = it->first;
                return true;
              }
          }

        return false;
      }

  void
    ApvlvCmds::sendmapkey (const char *s)
      {
        bool ret = buildevent (s);
        if (ret)
          {
            sendevent ();
          }
      }

  gboolean
    ApvlvCmds::apvlv_cmds_timeout_cb (gpointer data)
      {
        ApvlvCmds *cmds = (ApvlvCmds *) data;
        if (cmds->mCmdHead != NULL)
          {
            cmds->process (cmds->mCmdHead);
            delete cmds->mCmdHead;
            cmds->mCmdHead = NULL;
          }
        cmds->state = CMD_OK;
        return FALSE;
      }
}

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
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#ifndef WIN32
#include <gdk/gdkx.h>
#endif

#include <sstream>

namespace apvlv
{
  ApvlvCmds *gCmds = NULL;

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
      queue = "";
      state = CMD_OK;

      hasop = false;

      getall = false;

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
        translate (gev, &queue);
        run ();
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

        map <guint, const char *>::iterator it;
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

  bool
    ApvlvCmds::run ()
      {
        bool bret = false;
        while (! queue.empty ())
          {
            if (timeouttimer > 0)
              {
                g_source_remove (timeouttimer);
                timeouttimer = -1;
              }

            bool ret;

            switch (state)
              {
              case CMD_OK:
                hasop = false;
                count = 1;
              case GETTING_COUNT:
                ret = getcount ();
                if (ret == false)
                  {
                    timeouttimer = g_timeout_add (3000, apvlv_cmds_timeout_cb, this);
                    return false;
                  }
              case GETTING_CMD:
                ret = getcmd ();
                if (ret)
                  {
                    returnType type = gView->process (count, cmd);
                    if (type == MATCH)
                      {
                        state = CMD_OK;
                        bret = true;
                      }
                    else if (type == NO_MATCH)
                      {
                        state = CMD_OK;
                        bret = false;
                      }
                    else if (type == NEED_MORE)
                      {
                        timeouttimer = g_timeout_add (3000, apvlv_cmds_timeout_cb, this);
                        state = GETTING_CMD;
                        bret = false;
                      }
                  }
                else
                  {
                    return false;
                  }
                break;
              default:
                break;
              }
          }

        return bret;
      }

  bool
    ApvlvCmds::getcount ()
      {
        if (queue[0] == '-'
             || isdigit (queue[0])
        )
          {
            // If the 1st is '-' or a number and only this, I can't get the right count.
            // So, let's return.
            if (queue.size () < 2)
            return false;
          }
        else
          {
            state = GETTING_CMD;
            return true;
          }

        bool found = false;
        for (unsigned int i=1; i<queue.size (); ++i)
          {
            if (! isdigit (queue[i]))
              {
                found = true;
                break;
              }
          }

        // If can't find a nor number, return and get next time.
        if (! found)
          return false;

        if (queue[0] == '-')
          {
            hasop = true;
            queue.erase (0, 1);
          }

        istringstream is (queue);
        is >> count;

        if (hasop)
          {
            count = 0 - count;
          }

        while (isdigit (queue[0]))
          {
            queue.erase (0, 1);
          }

        state = GETTING_CMD;
        return true;
      }

  bool
    ApvlvCmds::getcmd ()
      {
        if (queue.empty ())
          return false;

        returnType ret = getmap ();
        if (ret == MATCH)
          {
            sendmapkey (mapcmd);
          }
        else if (ret == NEED_MORE)
          {
            return false;
          }

        if (queue.empty ())
          {
            return false;
          }

        if (queue[0] == '<')
          {
            size_t p = queue.find ('>');
            if (p != string::npos)
              {
                string ss (queue.c_str (), p + 1);
                GdkEventKey gk;
                bool r = translate (ss, &gk);
                if (r)
                  {
                    cmd = gk.keyval;
                    if (gk.state == GDK_CONTROL_MASK)
                      {
                        cmd = CTRL (cmd);
                      }
                    queue.erase (0, p + 1);
                    return true;
                  }
              }
          }
        else
          {
            cmd = queue[0];
            queue.erase (0, 1);
            return true;
          }
        return false;
      }

  returnType
    ApvlvCmds::getmap ()
      {
        returnType ret = NO_MATCH;
        for (unsigned int i=1; i<=queue.size (); ++i)
          {
            ret = gParams->getmap (queue.c_str (), i);
            if (ret == MATCH)
              {
                string ss (queue.c_str (), i);
                mapcmd = gParams->mapvalue (ss.c_str ());
                queue.erase (0, i);
                return MATCH;
              }
            else if (ret == NO_MATCH)
              {
                return NO_MATCH;
              }
          }

        return ret;
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
        cmds->queue = "";
        cmds->state = CMD_OK;
        return FALSE;
      }
}

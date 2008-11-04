/****************************************************************************
 * Copyright (c) 1998-2005,2006 Free Software Foundation, Inc.              
 *                                                                          
 * Permission is hereby granted, free of charge, to any person obtaining a  
 * copy of this software and associated documentation files (the            
 * "Software"), to deal in the Software without restriction, including      
 * without limitation the rights to use, copy, modify, merge, publish,      
 * distribute, distribute with modifications, sublicense, and/or sell       
 * copies of the Software, and to permit persons to whom the Software is    
 * furnished to do so, subject to the following conditions:                 
 *                                                                          
 * The above copyright notice and this permission notice shall be included  
 * in all copies or substantial portions of the Software.                   
 *                                                                          
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS  
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF               
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.   
 * IN NO EVENT SHALL THE ABOVE COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,   
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR    
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR    
 * THE USE OR OTHER DEALINGS IN THE SOFTWARE.                               
 *                                                                          
 * Except as contained in this notice, the name(s) of the above copyright   
 * holders shall not be used in advertising or otherwise to promote the     
 * sale, use or other dealings in this Software without prior written       
 * authorization.                                                           
****************************************************************************/

/****************************************************************************
 *  Author:    YuPengda
 *  AuthorRef: Alf <naihe2010@gmail.com>
 *  Blog:      http://naihe2010.cublog.cn
****************************************************************************/
#include "ApvlvParams.hpp"
#include "ApvlvView.hpp"
#include "ApvlvUtil.hpp"
#include "ApvlvCmds.hpp"

#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <gdk/gdkx.h>

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
              int p = ss.find ('>');
              if (p != string::npos)
                {
                  string ss2 (ss.c_str (), p + 1);
                  bool r = translate (ss2, &gk);
                  if (r == true)
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
          gk.string = strndup ("", 0);
          gk.hardware_keycode = XKeysymToKeycode (GDK_WINDOW_XDISPLAY (gk.window), gk.keyval);
          gk.group = 0;
          gk.is_modifier = 0;

          m_keys.push_back (gk);
        }
    }

  void
    ApvlvCmds::destroyevent ()
      {
        if (sendtimer > 0)
          gtk_timeout_remove (sendtimer); 
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
          sendtimer = gtk_timeout_add (100, ae_send_next, this);
      }

  gboolean
    ApvlvCmds::ae_send_next (void *data)
      {
        ApvlvCmds *ac = (ApvlvCmds *) data;
        ac->sendevent ();
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
          gtk_timeout_remove (timeouttimer); 
          timeouttimer = -1;
        }

      destroyevent ();
    }

  void 
    ApvlvCmds::append (GdkEventKey *gev)
      {
        translate (gev, &queue);
        bool ret = run ();
        if (ret == true)
          gView->status_show ();
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
            if (it->second = s.c_str ())
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
        if (timeouttimer > 0) 
          {
            gtk_timeout_remove (timeouttimer); 
            timeouttimer = -1;
          }

        bool ret;
        switch (state)
          {
          case CMD_OK:
            hasop = false;
          case GETTING_COUNT:
            ret = getcount ();
            if (ret)
              {
                return run ();
              }
            return false;
            break;
          case GETTING_CMD:
            ret = getcmd ();
            if (ret)
              {
                returnType type = gView->process (count, gdk_keyval_to_unicode (cmd), cmdstate);
                if (type == MATCH)
                  {
                    state = CMD_OK;
                    return true;
                  }
                else if (type == NO_MATCH)
                  {
                    state = CMD_OK;
                  }
                else if (type == NEED_MORE)
                  {
                    state = GETTING_CMD;
                  }
                else if (type == GET_ALL)
                  {
                    getall = true;
                  }
              }
            return false;
            break;
          default:
            break;
          }
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
            count = 1;
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

        if (hasop) count = 0 - count;

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

        if (queue[0] == '<')
          {
            int p = queue.find ('>');
            if (p != string::npos)
              {
                string ss (queue.c_str (), p + 1);
                GdkEventKey gk;
                bool r = translate (ss, &gk);
                if (r == true)
                  {
                    cmd = gk.keyval;
                    cmdstate = gk.state;
                    queue.erase (0, p + 1);
                    return true;
                  }
              }
          }

        cmd = gdk_unicode_to_keyval (queue[0]);
        cmdstate = 0;
        queue.erase (0, 1);
        return true;
      }

  returnType 
    ApvlvCmds::getmap ()
      {
        returnType ret;
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
        sendevent ();
      }

  gboolean 
    ApvlvCmds::apvlv_cmds_timeout_cb (gpointer data)
      {
        ApvlvCmds *cmds = (ApvlvCmds *) data;
        cmds->queue = "";
        return FALSE;
      }
}

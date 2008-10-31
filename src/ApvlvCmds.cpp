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

namespace apvlv
{
  ApvlvCmds *gCmds = NULL;

  ApvlvEvent::ApvlvEvent (const char *s)
    {
      m_next = NULL;
      gev = gev2 = NULL;

      if (strlen (s) >= 5 && strncmp (s, "<C-", 3) == 0 && s[4] == '>')
        {
          /*
          gev = (GdkEventKey *) g_malloc (sizeof (GdkEventKey));
          gev->type = GDK_KEY_PRESS;
          gev->window = gView->widget ()->window;
          gev->send_event = TRUE;
          gev->time = GDK_CURRENT_TIME;
          gev->state = 0;
          gev->keyval = gdk_unicode_to_keyval (GDK_Control_L);
          gev->length = 0;
          gev->string = strdup ("");
          gev->hardware_keycode = XKeysymToKeycode (GDK_WINDOW_XDISPLAY (gev->window), gev->keyval);
          gev->group = 0;
          gev->is_modifier = 1;*/

          gev2 = (GdkEventKey *) g_malloc (sizeof (GdkEventKey));
          gev2->type = GDK_KEY_PRESS;
          gev2->window = gView->widget ()->window;
          gev2->send_event = TRUE;
          gev2->time = GDK_CURRENT_TIME;
          gev2->state = GDK_CONTROL_MASK;
          gev2->keyval = gdk_unicode_to_keyval (s[3]);
          gev2->length = 1;
          gev2->string = strndup (s + 3, 1);
          gev2->hardware_keycode = XKeysymToKeycode (GDK_WINDOW_XDISPLAY (gev2->window), gev2->keyval);
          gev2->group = 0;
          gev2->is_modifier = 0;

          if (s[5] != '\0')
            {
              m_next = new ApvlvEvent (s + 5);
            }
        }
      else
        {
          gev = (GdkEventKey *) g_malloc (sizeof (GdkEventKey));
          gev->type = GDK_KEY_PRESS;
          gev->window =  gView->widget ()->window;
          gev->send_event = TRUE;
          gev->time = GDK_CURRENT_TIME;
          gev->state = 0;
          gev->keyval = gdk_unicode_to_keyval (s[0]);
          gev->length = 1;
          gev->string = strndup (s, 1);
          gev->hardware_keycode = XKeysymToKeycode (GDK_WINDOW_XDISPLAY (gev->window), gev->keyval);
          gev->group = 0;
          gev->is_modifier = 0;

          if (s[1] != '\0')
            {
              m_next = new ApvlvEvent (s + 1);
            }
        }
    }

  ApvlvEvent::~ApvlvEvent ()
    {
      if (gev != NULL)
        {
          g_free (gev);
        }

      if (gev2 != NULL)
        {
          g_free (gev2);
        }

      for (ApvlvEvent *ae = m_next; ae; ae = ae->m_next)
        {
          delete ae;
        }
    }

  void
    ApvlvEvent::send ()
      {
        if (gev != NULL)
          {
            gdk_event_put ((GdkEvent *) gev);
          }

        if (gev2 != NULL)
          {
            gdk_event_put ((GdkEvent *) gev2);
            gev2->type = GDK_KEY_RELEASE;
            gev2->send_event = FALSE;
            gdk_event_put ((GdkEvent *) gev2);
          }

        if (gev != NULL)
          {
            gev->type = GDK_KEY_RELEASE;
            gev->send_event = FALSE;
            gdk_event_put ((GdkEvent *) gev);
          }

        for (ApvlvEvent *ae = m_next; ae; ae = ae->m_next)
          {
            ae->send ();
          }
      }

  ApvlvCmds::ApvlvCmds () 
    { 
      argu = "";
      queue = ""; 

      timer = -1; 
    }

  ApvlvCmds::~ApvlvCmds () 
    { 
      if (timer > 0) 
        {
          gtk_timeout_remove (timer); 
          timer = -1;
        }
    }

  bool
    ApvlvCmds::tryrun ()
      {
        if (timer > 0) 
          {
            gtk_timeout_remove (timer); 
            timer = -1;
          }

        char *p = (char *) queue.c_str ();

        bool hasop = false; 
        char *op = p;
        bool hastimes = false;
        int times = 0;

        while (*p != '\0')
          {
            if (*p == '-')
              {
                hasop = !hasop;
                op = ++ p;
              }

            if (! isdigit (*p))
              {
                if (hastimes)
                  {
                    if (hasop)
                      {
                        times = 0 - atoi (op);
                      }
                    else
                      {
                        times = atoi (op);
                      }
                  }
                break;
              }
            else
              {
                hastimes = true;
                p ++;
              }
          }

        const char *s = gParams->mapvalue (p);
        if (s != NULL)
          {
            queue.replace (p - queue.c_str (), strlen (p), s);
            return tryrun ();
          }

        bool ret;
        if (argu == "")
          {
            if (hastimes)
              {
                ret = docmd (p, times);
              }
            else
              {
                ret = docmd (p);
              }
          }
        else
          {
            ret = doargu (p);
          }

        if (state != NOT_MATCH)
          {
            queue = "";
          }

        if (state != CMD_OK)
          {
            int cmd_timeout = atoi (gParams->settingvalue ("commandtimeout"));
            timer = gtk_timeout_add (cmd_timeout, apvlv_cmds_timeout_cb, this);
            return false;
          }

        return true;
      }

  bool 
    ApvlvCmds::docmd (const char *s, int times)
      {
        if (strcmp (s, "f") == 0)
          {
            gView->fullscreen ();
          }

        else if (strcmp (s, "q") == 0)
          {
            gView->quit ();
          }

        else if (strcmp (s, "o") == 0)
          {
            gView->open ();
          }

        else if (strcmp (s, "m") == 0)
          {
            if (argu == "m")
              {
                doargu ("m");
              }
            else
              {
                argu = "m";
                state = NEED_ARGUMENT;
              }
          }
        else if (strcmp (s, "'") == 0)
          {
            if (argu == "'")
              {
                doargu ("\'");
              }
            else
              {
                argu = "'";
                state = NEED_ARGUMENT;
              }
          }

        else if (strcmp (s, "R") == 0)
          {
            gView->reload ();
          }

        else if (strcmp (s, "C-w") == 0)
          {
            if (argu == "C-w")
              {
                doargu ("C-w");
              }
            else
              {
                argu = "C-w";
                state = NEED_ARGUMENT;
              }
          }

        else if (strcmp (s, "g") == 0)
          {
            gView->markposition ('\'');
            gView->showpage (times);
          }
        else if (strcmp (s, "C-d") == 0)
          {
            gView->halfnextpage (times);
          }
        else if (strcmp (s, "C-u") == 0)
          {
            gView->halfprepage (times);
          }
        else if (strcmp (s, "C-f") == 0)
          {
            gView->nextpage (times);
          }
        else if (strcmp (s, "C-b") == 0)
          {
            gView->prepage (times);
          }

        else if (strcmp (s, "k") == 0)
          {
            gView->scrollup (times);
          }
        else if (strcmp (s, "j") == 0)
          {
            gView->scrolldown (times);
          }
        else if (strcmp (s, "h") == 0)
          {
            gView->scrollleft (times);
          }
        else if (strcmp (s, "l") == 0)
          {
            gView->scrollright (times);
          }

        else if (strcmp (s, "zi") == 0)
          {
            gView->zoomin ();
          }
        else if (strcmp (s, "zo") == 0)
          {
            gView->zoomout ();
          }

        else if (strcmp (s, "/") == 0)
          {
            gView->markposition ('\'');
            gView->promptsearch ();
          }
        else if (strcmp (s, "?") == 0)
          {
            gView->markposition ('\'');
            gView->promptbacksearch ();
          }
        else if (strcmp (s, ":") == 0)
          {
            gView->promptcommand ();
          }
        else
          {
            state = NOT_MATCH;
            return false;
          }

        state = CMD_OK;
        return true;
      }

  bool
    ApvlvCmds::doargu (const char *s)
      {
        if (argu == "m")
          {
            if ('a' <= *s && *s <= 'z')
              {
                gView->markposition (*s);
              }
          }
        else if (argu == "'")
          {
            gView->jump (*s);
          }
        else if (argu == "C-w")
          {
            gView->dowindow (s);
          }
        else
          {
            return false;
          }

        argu = "";
        return true;
      }

  gboolean 
    ApvlvCmds::apvlv_cmds_timeout_cb (gpointer data)
      {
        ApvlvCmds *cmds = (ApvlvCmds *) data;
        cmds->queue = "";
        return FALSE;
      }
}

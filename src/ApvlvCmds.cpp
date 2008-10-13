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
#include "ApvlvCmds.hpp"

#include <gtk/gtk.h>

namespace apvlv
{
  ApvlvCmds::ApvlvCmds (ApvlvParams *pa) 
    { 
      param = pa;

      argu = "";
      queue = ""; 

      state = CMD_OK;
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

        const char *s = param->mapvalue (p);
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
            ret = doargu (*p);
          }

        if (state != NOT_MATCH)
          {
            queue = "";
          }

        if (state != CMD_OK)
          {
            int cmd_timeout = atoi (param->settingvalue ("commandtimeout"));
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
            fullscreen ();
          }

        else if (strcmp (s, "q") == 0)
          {
            quit ();
          }

        else if (strcmp (s, "o") == 0)
          {
            open ();
          }

        else if (strcmp (s, "m") == 0)
          {
            if (argu == "m")
              {
                doargu ('m');
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
                doargu ('\'');
              }
            else
              {
                argu = "'";
                state = NEED_ARGUMENT;
              }
          }

        else if (strcmp (s, "R") == 0)
          {
            reload ();
          }

        else if (strcmp (s, "g") == 0)
          {
            markposition ('\'');
            showpage (times);
          }
        else if (strcmp (s, "C-d") == 0)
          {
            halfnextpage (times);
          }
        else if (strcmp (s, "C-u") == 0)
          {
            halfprepage (times);
          }
        else if (strcmp (s, "C-f") == 0)
          {
            nextpage (times);
          }
        else if (strcmp (s, "C-b") == 0)
          {
            prepage (times);
          }

        else if (strcmp (s, "k") == 0)
          {
            scrollup (times);
          }
        else if (strcmp (s, "j") == 0)
          {
            scrolldown (times);
          }
        else if (strcmp (s, "h") == 0)
          {
            scrollleft (times);
          }
        else if (strcmp (s, "l") == 0)
          {
            scrollright (times);
          }

        else if (strcmp (s, "zi") == 0)
          {
            zoomin ();
          }
        else if (strcmp (s, "zo") == 0)
          {
            zoomout ();
          }

        else if (strcmp (s, "/") == 0)
          {
            markposition ('\'');
            promptsearch ();
          }
        else if (strcmp (s, "?") == 0)
          {
            markposition ('\'');
            promptbacksearch ();
          }
        else if (strcmp (s, ":") == 0)
          {
            promptcommand ();
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
    ApvlvCmds::doargu (const char s)
      {
        if (argu == "m")
          {
            if ('a' <= s && s <= 'z')
              {
                markposition (s);
              }
          }
        else if (argu == "'")
          {
            jump (s);
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

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

        p = (char *) queue.c_str ();

        while (*p != '\0')
          {
            if (! ('0' <= *p && *p <= '9'))
              {
                break;
              }
            else
              {
                p ++;
              }
          }

        const char *s = param->value (p);
        if (s == NULL)
          {
            int cmd_timeout = atoi (param->value ("commandtimeout"));
            timer = gtk_timeout_add (cmd_timeout, apvlv_cmds_timeout_cb, this);
            return false;
          }

        int times = atoi (queue.c_str ());
        times > 0? docmd (s, times): docmd (s);

        queue = "";
        return true;
      }

  bool 
    ApvlvCmds::docmd (const char *s, int times)
      {
        if (strcmp (s, "fullscreen") == 0)
          {
            fullscreen ();
          }

        else if (strcmp (s, "quit") == 0)
          {
            quit ();
          }

        else if (strcmp (s, "open") == 0)
          {
            open ();
          }

        else if (strcmp (s, "goto") == 0)
          {
            showpage (times);
          }
        else if (strcmp (s, "nextpage") == 0)
          {
            nextpage (times);
          }
        else if (strcmp (s, "prepage") == 0)
          {
            prepage (times);
          }
        //else if (strcmp (s, "nexthalfpage") == 0)
        //  {
        //    nexthalfpage (times);
        //  }
        //else if (strcmp (s, "prehalfpage") == 0)
        //  {
        //    prehalfpage (times);
        //  }

        else if (strcmp (s, "scrollup") == 0)
          {
            scrollup (times);
          }
        else if (strcmp (s, "scrolldown") == 0)
          {
            scrolldown (times);
          }
        else if (strcmp (s, "scrollleft") == 0)
          {
            scrollleft (times);
          }
        else if (strcmp (s, "scrollright") == 0)
          {
            scrollright (times);
          }

        else if (strcmp (s, "zoomin") == 0)
          {
            zoomin ();
          }
        else if (strcmp (s, "zoomout") == 0)
          {
            zoomout ();
          }
        else if (strcmp (s, "search") == 0)
          {
            promptsearch ();
          }
        else if (strcmp (s, "backsearch") == 0)
          {
            promptbacksearch ();
          }
        else if (strcmp (s, "commandmode") == 0)
          {
            promptcommand ();
          }

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

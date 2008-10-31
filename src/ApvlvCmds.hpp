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
#ifndef _APVLV_CMDS_H_
#define _APVLV_CMDS_H_

#ifdef HAVE_CONFIG_H
# include "config.hpp"
#endif

#include <gtk/gtk.h>

#include <iostream>
using namespace std;

namespace apvlv
{
  class ApvlvEvent
    {
  public:
      ApvlvEvent (const char *s);
      ~ApvlvEvent ();

      void send ();

      ApvlvEvent *m_next;

  private:
      GdkEventKey *gev, *gev2;
    };

  class ApvlvCmds
    {
  public:
    ApvlvCmds ();

    ~ApvlvCmds ();

    void push (char c) { char t[2] = { 0 }; t[0] = c; queue.append (t); tryrun (); }

    void push (const char *cmd) { queue.append (cmd); tryrun (); }

  private:
    bool tryrun ();
    bool docmd (const char *s, int times = 1);
    bool doargu (const char *s);

    enum 
      {
        CMD_OK,
        NOT_MATCH,
        NEED_ARGUMENT
      } state;

    static gboolean apvlv_cmds_timeout_cb (gpointer);
    gint timer;

    string queue;
    string argu;
    };

  extern ApvlvCmds *gCmds;
}

#endif

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
#include <vector>
using namespace std;

namespace apvlv
{
  class ApvlvCmds
    {
  public:
    ApvlvCmds ();

    ~ApvlvCmds ();

    void append (GdkEventKey *gev);

  private:
    void translate (GdkEventKey *gev, string *s);

    bool translate (string &s, GdkEventKey *gev);

    bool run ();

    bool getcount ();

    bool getcmd ();

    returnType getmap ();

    void sendmapkey (const char *s);

    enum cmdState
      {
        GETTING_COUNT,
        GETTING_CMD,
        CMD_OK,
      } state;

    static gboolean apvlv_cmds_timeout_cb (gpointer);
    gint timeouttimer;

    string queue;

    bool hasop;
    int count;

    bool getall;

    const char *mapcmd;

    guint cmd;

    guint cmdstate;

    map <guint, const char *> KS;

    /*
    typedef pair <guint, const char *> keystritem;
    vector <keystritem> KS2; */

    bool buildevent (const char *p);

    void destroyevent ();

    void sendevent ();

    int sendtimer;

    static gboolean ae_send_next (void *);

    vector <GdkEventKey> m_keys;

    };

  extern ApvlvCmds *gCmds;
}

#endif

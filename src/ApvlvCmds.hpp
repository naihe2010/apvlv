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
*
*/
/* @CPPFILE ApvlvCmds.hpp
*
*  Author: Alf <naihe2010@gmail.com>
*/
/* @date Created: 2008/09/30 00:00:00 Alf */

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

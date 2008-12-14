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
/* @CPPFILE ApvlvParams.hpp
*
*  Author: Alf <naihe2010@gmail.com>
*/
/* @date Created: 2008/09/30 00:00:00 Alf */

#ifndef _APVLV_PARAMS_H_
#define _APVLV_PARAMS_H_

#ifdef HAVE_CONFIG_H
# include "config.hpp"
#endif

#include "ApvlvUtil.hpp"

#include <iostream>
#include <string>
#include <map>

using namespace std;

namespace apvlv
{
  class ApvlvParams
    {
  public:
    ApvlvParams ();
    ~ApvlvParams ();

    bool loadfile (const char *filename);

    bool mappush (string &cmd1, string &cmd2);

    const char *mapvalue (const char *key);

    returnType getmap (const char *s, int n);

    const char *cmd (const char *key);

    bool settingpush (string &ch, string &str);

    bool settingpush (const char *c, const char *s);

    const char *settingvalue (const char *key);

    //for debug
    void show ()
      {
        map <string, string>::iterator it;

        cerr << "maps" << endl;
        for (it = m_maps.begin (); it != m_maps.end (); ++ it)
          {
            cerr << "first:[" << (*it).first << "], second[" << (*it).second << "]" << endl;
          }
        cerr << endl;

        cerr << "settings" << endl;
        for (it = m_settings.begin (); it != m_settings.end (); ++ it)
          {
            cerr << "first:[" << (*it).first << "], second[" << (*it).second << "]" << endl;
          }
      }

  private:
    map <string, string> m_maps, m_settings;
    };

  extern ApvlvParams *gParams;
}

#endif

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
/* @CPPFILE ApvlvParams.cpp
*
*  Author: Alf <naihe2010@gmail.com>
*/
/* @date Created: 2008/09/30 00:00:00 Alf */

#include "ApvlvUtil.hpp"
#include "ApvlvParams.hpp"

#include <string.h>

#include <iostream>
#include <fstream>
#include <sstream>

namespace apvlv
{
  ApvlvParams *gParams = NULL;

  ApvlvParams::ApvlvParams ()
    {
      settingpush ("fullscreen", "no");
      settingpush ("zoom", "fitwidth");
      settingpush ("width", "800");
      settingpush ("height", "600");
      settingpush ("commandtimeout", "1000");
#ifdef WIN32
      settingpush ("defaultdir", "C:\\");
#else
      settingpush ("defaultdir", "/tmp");
#endif
  }

  ApvlvParams::~ApvlvParams ()
    {
      m_maps.clear ();
      m_settings.clear ();
    }

  bool ApvlvParams::loadfile (const char *filename)
    {
      string str;
      fstream os (filename, ios::in);

      if (! os.is_open ())
        {
          err ("Open configure file %s error", filename);
          return false;
        }

      while ((getline (os, str)) != NULL)
        {
          string argu, data, crap;
          stringstream is (str);
          // avoid commet line, continue next
          is >> crap;
          if (crap[0] == '\"' || crap == "")
            {
              continue;
            }
          // parse the line like "set fullscreen=yes" or set "set zoom=1.5"
          else if (crap == "set")
            {
              is >> argu;
              size_t off = argu.find ('=');
              if (off == string::npos)
                {
                  is >> crap >> data;
                  if (crap == "=" && data.length () > 0)
                    {
                      settingpush (argu.c_str (), data.c_str ());
                      continue;
                    }
                }
              else if (off < 32)
                {
                  char k[32], v[32], *p;
                  memcpy (k, argu.c_str (), off);
                  k[off] = '\0';

                  p = (char *) argu.c_str () + off + 1;
                  while (isspace (*p))
                    p ++;

                  if (*p != '\0')
                    {
                      strncpy (v, argu.c_str () + off + 1, 31);
                      v[31] = '\0';
                      settingpush (k, v);
                      continue;
                    }
                }

              err ("Syntax error: set: %s", str.c_str ());
            }
          // like "map n next-page"
          else if (crap == "map")
            {
              is >> argu;
              getline (is, data);
              while (data[0] == ' ' || data[0] == '\t')
                data.erase (0, 1);
              if (argu.length () > 0 && data.length () > 0)
                {
                  mappush (argu, data);
                }
              else
                {
                  err ("Syntax error: map: %s", str.c_str ());
                }
            }
          else
            {
              err ("Unknown rc command: %s: %s", crap.c_str (), str.c_str ());
            }
        }
      return true;
    }

  bool
    ApvlvParams::mappush (string &ch, string &str)
      {
        m_maps[ch] = str;
        return true;
      }

  bool
    ApvlvParams::settingpush (const char *c, const char *s)
      {
        string cs (c), ss (s);
        m_settings[cs] = ss;
        return true;
      }

  bool
    ApvlvParams::settingpush (string &ch, string &str)
      {
        m_settings[ch] = str;
        return true;
      }

  const char *
    ApvlvParams::mapvalue (const char *s)
      {
        string ss (s);
        map <string, string>::iterator it;
        it = m_maps.find (ss);
        if (it != m_maps.end ())
          {
            return (*it).second.c_str ();
          }
        return NULL;
      }

  returnType
    ApvlvParams::getmap (const char *s, int n)
      {
        map <string, string>::iterator it;
        for (it = m_maps.begin (); it != m_maps.end (); ++ it)
          {
            if (strncmp (it->first.c_str (), s, n) == 0)
              {
                if (it->first.size () == (unsigned int) n)
                  return MATCH;
                else
                  return NEED_MORE;
              }
          }
        return NO_MATCH;
      }

  const char *
    ApvlvParams::settingvalue (const char *s)
      {
        string ss (s);
        map <string, string>::iterator it;
        it = m_settings.find (ss);
        if (it != m_settings.end ())
          {
            return (*it).second.c_str ();
          }
        return NULL;
      }
}

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
#include "ApvlvCmds.hpp"
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
      push ("fullscreen", "no");
      push ("zoom", "fitwidth");
      push ("width", "800");
      push ("height", "600");
      push ("cache", "no");
      push ("commandtimeout", "1000");
#ifdef WIN32
      push ("defaultdir", "C:\\");
#else
      push ("defaultdir", "/tmp");
#endif
    }

  ApvlvParams::~ApvlvParams ()
    {
      mSettings.clear ();
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
                      push (argu.c_str (), data.c_str ());
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
                      strncpy (v, p, 31);
                      v[31] = '\0';
                      push (k, v);
                      continue;
                    }
                }

              err ("Syntax error: set: %s", str.c_str ());
            }
          // like "map n next-page"
          else if (crap == "map")
            {
              is >> argu;

              if (argu.length () == 0)
                {
                  err ("map command not complete");
                  continue;
                }

              getline (is, data);

              while (data.length () > 0 && isspace (data[0]))
                data.erase (0, 1);

              if (argu.length () > 0 && data.length () > 0)
                {
                  gCmds->buildmap (argu.c_str (), data.c_str ());
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
    ApvlvParams::push (const char *c, const char *s)
      {
        string cs (c), ss (s);
        mSettings[cs] = ss;
        return true;
      }

  bool
    ApvlvParams::push (string &ch, string &str)
      {
        mSettings[ch] = str;
        return true;
      }

  const char *
    ApvlvParams::value (const char *s)
      {
        string ss (s);
        map <string, string>::iterator it;
        it = mSettings.find (ss);
        if (it != mSettings.end ())
          {
            return it->second.c_str ();
          }
        return NULL;
      }
}

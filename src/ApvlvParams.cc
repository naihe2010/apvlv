/*
 * This file is part of the apvlv package
 *
 * Copyright (C) 2008 Alf.
 *
 * Contact: Alf <naihe2010@126.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2.0 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */
/* @CPPFILE ApvlvParams.cc
 *
 *  Author: Alf <naihe2010@126.com>
 */

#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>

#include "ApvlvCmds.h"
#include "ApvlvParams.h"
#include "ApvlvUtil.h"

namespace apvlv
{
using namespace std;

ApvlvParams::ApvlvParams ()
{
  push ("inverted", "no");
  push ("fullscreen", "no");
  push ("zoom", "fitwidth");
  push ("continuous", "yes");
  push ("autoscrollpage", "yes");
  push ("autoscrolldoc", "yes");
  push ("noinfo", "no");
  push ("width", "800");
  push ("height", "600");
  push ("fix_width", "0");
  push ("fix_height", "0");
  push ("background", "");
  push ("warpscan", "yes");
  push ("commandtimeout", "1000");
#ifdef WIN32
  push ("defaultdir", "C:\\");
#else
  push ("defaultdir", "/tmp");
#endif
  push ("guioptions", "mTsS");
  push ("autoreload", "3");
  push ("thread_count", "auto");
  push ("lok_path", "/usr/lib64/libreoffice/program");

  push (".pdf:engine", "MuPDF");
  push (".epub:engine", "Web");
  push (".fb2:engine", "Web");
  push (".txt:engine", "MuPDF");

  push ("ocr:lang", "eng+chi_sim");
}

ApvlvParams::~ApvlvParams () = default;

bool
ApvlvParams::loadFile (const std::string &filename)
{
  string str;
  fstream os (filename, ios::in);

  if (!os.is_open ())
    {
      qWarning () << "Open configure file " << filename << " error";
      return false;
    }

  while ((getline (os, str)))
    {
      string argu;
      string data;
      string crap;
      stringstream is (str);

      is >> crap;
      if (crap[0] == '\"' || crap.empty ())
        {
          continue;
        }

      if (crap == "set")
        {
          is >> argu;
          size_t off = argu.find ('=');
          if (off == string::npos)
            {
              is >> crap >> data;
              if (crap == "=")
                {
                  push (argu, data);
                  continue;
                }
            }
          else
            {
              argu[off] = ' ';
              stringstream ass{ argu };
              ass >> argu >> data;
              push (argu, data);
              continue;
            }
        }

      // like "map n next-page"
      else if (crap == "map")
        {
          is >> argu;

          if (argu.empty ())
            {
              qWarning () << "map command not complete";
              continue;
            }

          getline (is, data);

          while (!data.empty () && isspace (data[0]))
            data.erase (0, 1);

          if (!argu.empty () && !data.empty ())
            {
              ApvlvCmds::buildCommandMap (argu, data);
            }
          else
            {
              qWarning () << "Syntax error: map: " << str;
            }
        }
      else
        {
          qWarning () << "Unknown rc command: " << crap << ": " << str;
        }
    }

  return true;
}

bool
ApvlvParams::push (string_view ch, string_view str)
{
  mParamMap[string (ch)] = str;
  return true;
}

string
ApvlvParams::getGroupStringOrDefault (std::string_view entry,
                                      std::string_view key,
                                      const std::string &defs)
{
  auto itr = std::ranges::find_if (
      mParamMap,
      [entry, key] (const pair<string, string> &p) -> bool
        {
          if (p.first.find (':') == string::npos)
            return false;
          else
            {
              auto pos = p.first.find (':');
              auto pentry = p.first.substr (0, pos);
              auto pkey = p.first.substr (pos + 1);
              return pentry == entry && pkey == key;
            }
        });
  if (itr != mParamMap.cend ())
    {
      return itr->second;
    }
  return defs;
}

string
ApvlvParams::getStringOrDefault (string_view key, const string &defs)
{
  auto itr
      = std::ranges::find_if (mParamMap,
                              [key] (const pair<string, string> &p) -> bool
                                {
                                  return p.first == key;
                                });
  if (itr != mParamMap.cend ())
    {
      return itr->second;
    }
  return defs;
}

int
ApvlvParams::getIntOrDefault (string_view key, int defi)
{
  auto values = getStringOrDefault (key, "");
  if (values.empty ())
    return defi;

  return int (strtol (values.c_str (), nullptr, 10));
}

bool
ApvlvParams::getBoolOrDefault (string_view key, bool defb)
{
  auto values = getStringOrDefault (key, "");
  if (values.empty ())
    return defb;

  if (values == "true" || values == "yes" || values == "on" || values == "1")
    {
      return true;
    }
  else
    {
      return false;
    }
}
}

// Local Variables:
// mode: c++
// End:

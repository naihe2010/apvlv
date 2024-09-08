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

#include "ApvlvParams.h"
#include "ApvlvCmds.h"
#include "ApvlvUtil.h"

#include <fstream>
#include <iostream>
#include <sstream>

namespace apvlv
{
ApvlvParams *gParams = nullptr;

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
  push ("guioptions", "mT");
  push ("autoreload", "3");
  push ("thread_count", "auto");
  push ("lok_path", "/usr/lib64/libreoffice/program");
}

ApvlvParams::~ApvlvParams () { mSettings.clear (); }

bool
ApvlvParams::loadfile (const string &filename)
{
  //    debug ("load debug: %s", filename);
  string str;
  fstream os (filename, ios::in);

  if (!os.is_open ())
    {
      qCritical ("Open configure file %s error", filename.c_str ());
      return false;
    }

  while ((getline (os, str)))
    {
      string argu, data, crap;
      stringstream is (str);
      // avoid commet line, continue next
      is >> crap;
      if (crap[0] == '\"' || crap.empty ())
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
              if (crap == "=")
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

              p = (char *)argu.c_str () + off + 1;
              while (isspace (*p))
                {
                  p++;
                }

              snprintf (v, sizeof v, "%s", *p ? p : "");

              p = (char *)v + strlen (v) - 1;
              while (isspace (*p) && p >= v)
                {
                  p--;
                }
              *(p + 1) = '\0';

              push (k, v);
              continue;
            }

          qCritical ("Syntax error: set: %s", str.c_str ());
        }
      // like "map n next-page"
      else if (crap == "map")
        {
          is >> argu;

          if (argu.length () == 0)
            {
              qCritical ("map command not complete");
              continue;
            }

          getline (is, data);

          while (data.length () > 0 && isspace (data[0]))
            data.erase (0, 1);

          if (argu.length () > 0 && data.length () > 0)
            {
              ApvlvCmds::buildmap (argu.c_str (), data.c_str ());
            }
          else
            {
              qCritical ("Syntax error: map: %s", str.c_str ());
            }
        }
      else
        {
          qCritical ("Unknown rc command: %s: %s", crap.c_str (),
                     str.c_str ());
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
ApvlvParams::values (const char *s)
{
  string ss (s);
  auto it = mSettings.find (ss);
  if (it != mSettings.end ())
    {
      return it->second.c_str ();
    }
  return nullptr;
}

int
ApvlvParams::valuei (const char *s)
{
  string ss (s);
  auto it = mSettings.find (ss);
  if (it != mSettings.end ())
    {
      return int (strtol (it->second.c_str (), nullptr, 10));
    }
  return -1;
}

bool
ApvlvParams::valueb (const char *s)
{
  string ss (s);
  auto it = mSettings.find (ss);
  if (it != mSettings.end () && it->second == "yes")
    {
      return true;
    }
  return false;
}
}

// Local Variables:
// mode: c++
// End:

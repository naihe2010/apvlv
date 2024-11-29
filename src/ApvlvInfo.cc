/*
 * This file is part of the apvlv package
 * Copyright (C) <2008>  <Alf>
 *
 * Contact: Alf <naihe2010@126.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */
/* @CPPFILE ApvlvInfo.cc
 *
 *  Author: Alf <naihe2010@126.com>
 */

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <ranges>
#include <sstream>

#include "ApvlvInfo.h"
#include "ApvlvParams.h"

namespace apvlv
{
using namespace std;

void
ApvlvInfo::loadFile (std::string_view file)
{
  mFileName = file;

  ifstream is (mFileName, ios::in);
  if (is.is_open ())
    {
      string line;

      while (getline (is, line))
        {
          auto p = line.c_str ();

          if (*p != '\''              /* the ' */
              || !isdigit (*(p + 1))) /* the digit */
            {
              continue;
            }

          addPosition (p);
        }

      is.close ();
    }
}

bool
ApvlvInfo::update ()
{
  ofstream os (mFileName, ios::out);
  if (!os.is_open ())
    {
      return false;
    }

  int i = 0;
  for (const auto &infofile : mInfoFiles)
    {
      os << "'" << i++ << "\t";
      os << infofile.page << ':' << infofile.skip << "\t";
      os << infofile.rate << "\t";
      os << infofile.file << endl;
    }

  os.close ();
  return true;
}

std::optional<InfoFile *>
ApvlvInfo::lastFile ()
{
  if (mInfoFiles.empty ())
    return nullopt;
  else
    return &*(mInfoFiles.rbegin ());
}

optional<InfoFile *>
ApvlvInfo::file (const string &filename)
{
  auto itr = std::ranges::find_if (
      std::views::reverse (mInfoFiles),
      [filename] (auto const &infofile) { return infofile.file == filename; });
  if (itr != mInfoFiles.rend ())
    {
      return &(*itr);
    }

  return nullopt;
}

bool
ApvlvInfo::updateFile (int page, int skip, double rate, const string &filename)
{
  InfoFile infofile{ page, skip, rate, filename };
  auto optinfofile = file (filename);
  if (optinfofile)
    {
      *optinfofile.value () = infofile;
    }
  else
    {
      mInfoFiles.push_back (infofile);
      if (mInfoFiles.size () > mMaxInfo)
        mInfoFiles.pop_front ();
    }

  return update ();
}

ApvlvInfo::ApvlvInfo ()
{
  mMaxInfo = ApvlvParams::instance ()->getIntOrDefault ("max_info",
                                                        DEFAULT_MAX_INFO);
}

bool
ApvlvInfo::addPosition (const char *str)
{
  const char *p;
  const char *s;

  p = strchr (str + 2, '\t'); /* Skip the ' and the digit */
  if (p == nullptr)
    {
      return false;
    }

  while (*p != '\0' && !isdigit (*p))
    {
      p++;
    }
  int page = int (strtol (p, nullptr, 10));
  int skip;

  s = strchr (p, ':');
  for (; s && p < s; ++p)
    {
      if (!isdigit (*p))
        {
          break;
        }
    }
  if (p == s)
    {
      ++p;
      skip = int (strtol (p, nullptr, 10));
    }
  else
    {
      skip = 0;
    }

  p = strchr (p, '\t');
  if (p == nullptr)
    {
      return false;
    }

  while (*p != '\0' && !isdigit (*p))
    {
      p++;
    }
  double rate = strtod (p, nullptr);

  p = strchr (p, '\t');
  if (p == nullptr)
    {
      return false;
    }

  while (*p != '\0' && isspace (*p))
    {
      p++;
    }
  if (*p == '\0')
    {
      return false;
    }

  auto fp = InfoFile{ page, skip, rate, p };
  mInfoFiles.emplace_back (fp);
  return true;
}
};

// Local Variables:
// mode: c++
// End:

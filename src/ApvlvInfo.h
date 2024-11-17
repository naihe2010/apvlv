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
/* @CPPFILE ApvlvInfo.h
 *
 *  Author: Alf <naihe2010@126.com>
 */

#ifndef _APVLV_INFO_H_
#define _APVLV_INFO_H_

#include <deque>
#include <optional>
#include <string>

namespace apvlv
{

struct InfoFile
{
  int page;
  int skip;
  double rate;
  std::string file;
};

const int DEFAULT_MAX_INFO = 100;

class ApvlvInfo final
{
public:
  ApvlvInfo (const ApvlvInfo &) = delete;
  ApvlvInfo &operator= (const ApvlvInfo &) = delete;
  void loadFile (std::string_view file);
  bool update ();

  std::optional<InfoFile *> file (const std::string &filename);
  bool updateFile (int page, int skip, double rate,
                   const std::string &filename);

  static ApvlvInfo *
  instance ()
  {
    static ApvlvInfo inst;
    return &inst;
  }

private:
  ApvlvInfo ();
  ~ApvlvInfo () = default;

  std::string mFileName{};

  std::deque<InfoFile> mInfoFiles{};
  std::deque<InfoFile>::size_type mMaxInfo{ DEFAULT_MAX_INFO };

  bool addPosition (const char *str);
};
};

#endif

/* Local Variables: */
/* mode: c++ */
/* End: */

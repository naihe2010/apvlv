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
#define _APVLV_INFO_H_ 1

#include <optional>
#include <string>
#include <vector>

namespace apvlv
{
struct InfoFile
{
  int page;
  int skip;
  double rate;
  std::string file;
};

class ApvlvInfo
{
public:
  explicit ApvlvInfo (const std::string &file);
  ~ApvlvInfo () = default;

  bool update ();

  std::optional<InfoFile *> file (int);
  std::optional<InfoFile *> file (const std::string &filename);
  bool updateFile (int page, int skip, double rate,
                   const std::string &filename);

private:
  std::string mFileName;

  std::vector<InfoFile> mInfoFiles;
  int mFileMax;

  bool ini_add_position (const char *);
};

extern ApvlvInfo *gInfo;
};

#endif

/* Local Variables: */
/* mode: c++ */
/* End: */

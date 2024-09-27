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
/* @CPPFILE ApvlvParams.h
 *
 *  Author: Alf <naihe2010@126.com>
 */

#ifndef _APVLV_PARAMS_H_
#define _APVLV_PARAMS_H_

#include <map>
#include <string>
#include <string_view>

namespace apvlv
{

class ApvlvParams final
{
public:
  ApvlvParams ();
  ~ApvlvParams ();

  bool loadfile (const std::string &filename);

  bool push (std::string_view ch, std::string_view str);

  std::string getStringOrDefault (std::string_view key,
                                  const std::string &defs = "");

  int getIntOrDefault (std::string_view key, int defi = 0);

  bool getBoolOrDefault (std::string_view key, bool defb = false);

private:
  std::map<std::string, std::string> mParamMap;
};

extern ApvlvParams *gParams;
}

#endif

/* Local Variables: */
/* mode: c++ */
/* End: */

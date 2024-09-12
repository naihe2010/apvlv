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
#define _APVLV_PARAMS_H_ 1

#include <map>
#include <string>
#include <string_view>

namespace apvlv
{
using namespace std;

class ApvlvParams final
{
public:
  ApvlvParams ();
  ~ApvlvParams ();

  bool loadfile (const string &filename);

  bool push (string_view ch, string_view str);

  string getStringOrDefault (string_view key, const string &defs = "");

  int getIntOrDefault (string_view key, int defi = 0);

  bool getBoolOrDefault (string_view key, bool defb = false);

private:
  map<string, string> mParamMap;
};

extern ApvlvParams *gParams;
}

#endif

/* Local Variables: */
/* mode: c++ */
/* End: */

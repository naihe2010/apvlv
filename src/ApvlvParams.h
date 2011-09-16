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
/* @CPPFILE ApvlvParams.hpp
 *
 *  Author: Alf <naihe2010@126.com>
 */
/* @date Created: 2008/09/30 00:00:00 Alf */

#ifndef _APVLV_PARAMS_H_
#define _APVLV_PARAMS_H_

#include "ApvlvUtil.h"

#include <iostream>
#include <string>
#include <map>

using namespace std;

namespace apvlv
{
typedef map < string, string > ApvlvParam;

class ApvlvParams
{
public:
  ApvlvParams ();
  ~ApvlvParams ();

  bool loadfile (const char *filename);

  bool push (string & ch, string & str);

  bool push (const char *c, const char *s);

  const char *values (const char *key);

  int valuei (const char *key);

  double valuef (const char *key);

  bool valueb (const char *key);

private:
  ApvlvParam mSettings;
};

extern ApvlvParams *gParams;
}

#endif

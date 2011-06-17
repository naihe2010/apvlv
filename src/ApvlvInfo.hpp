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
/* @CFILE ApvlvInfo.hpp
*
*  Author: Alf <naihe2010@126.com>
*/
/* @date Created: 2010/02/23 14:56:21 Alf*/

#ifndef _APVLV_INFO_H_
#define _APVLV_INOF_H_

#ifdef HAVE_CONFIG_H
# include "config.hpp"
#endif

#include <glib.h>
#include <string>

using namespace std;

namespace apvlv
{
struct infofile
{
  int page;
  int skip;
  double rate;
  string file;
};

class ApvlvInfo
{
public:
  ApvlvInfo (const char *file);
  ~ApvlvInfo ();

  bool update ();

  infofile *file (int);
  infofile *file (const char *);
  bool file (int, double, const char *, int);

private:
  string mFileName;

  GSList *mFileHead;
  int mFileMax;

  bool ini_add_position (const char *);
};

extern ApvlvInfo *gInfo;
};

#endif

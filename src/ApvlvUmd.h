/*
* This file is part of the apvlv package
* Copyright (C) <2010>  <Alf>
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
/* @CFILE ApvlvUmd.hpp xxxxxxxxxxxxxxxxxxxxxxxxxx.
*
*  Author: Alf <naihe2010@126.com>
*/
/* @date Created: 2011/09/16 13:55:19 Alf*/

#ifndef _APVLV_UMD_H_
#define _APVLV_UMD_H_

#include "ApvlvFile.h"

namespace apvlv
{
class ApvlvUMD:public ApvlvFile
{
public:
  ApvlvUMD (const char *filename, bool check = true);

  ~ApvlvUMD ();

  bool writefile (const char *filename);

  bool pagesize (int page, int rot, double *x, double *y);

  int pagesum ();

  bool pagetext (int, int, int, int, int, char **);

  bool render (int, int, int, double, int, GdkPixbuf *, char *);

  bool pageselectsearch (int, int, int, double, int, GdkPixbuf *,
                         char *, int, ApvlvPoses *);

  ApvlvPoses *pagesearch (int pn, const char *s, bool reverse = false);

  ApvlvLinks *getlinks (int pn);

  ApvlvFileIndex *new_index ();

  void free_index (ApvlvFileIndex *);

  bool pageprint (int pn, cairo_t * cr);

private:
#ifdef HAVE_LIBUMD
  umd_t *mUmd;
#endif
};

}
#endif

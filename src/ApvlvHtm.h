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
/* @CPPFILE ApvlvHtm.h
 *
 *  Author: Alf <naihe2010@126.com>
 */
/* @date Created: 2011/09/16 13:50:49 Alf*/

#ifndef _APVLV_HTM_H_
#define _APVLV_HTM_H_

#include "ApvlvFile.h"

namespace apvlv
{
const double HTML_DEFAULT_WIDTH = 800;
const double HTML_DEFAULT_HEIGHT = 800;

class ApvlvHTML : public ApvlvFile
{
  FILE_TYPE_DECLARATION (ApvlvHTML);

public:
  explicit ApvlvHTML (const string &filename, bool check = true);

  ~ApvlvHTML () override;

  bool writefile (const char *filename) override;

  bool pagesize (int page, int rot, double *x, double *y) override;

  int pagesum () override;

  bool pagetext (int, double, double, double, double, char **) override;

  bool render (int pn, int ix, int iy, double zm, int rot,
               QWebEngineView *webview) override;

  unique_ptr<ApvlvPoses> pagesearch (int pn, const char *str,
                                     bool reverse) override;

  unique_ptr<ApvlvLinks> getlinks (int pn) override;

  bool pageprint (int pn, QPrinter *cr) override;

  DISPLAY_TYPE
  get_display_type () override { return DISPLAY_TYPE_HTML; }

private:
  string mUri;
};

}

#endif

/* Local Variables: */
/* mode: c++ */
/* End: */

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
/* @CPPFILE ApvlvTxt.h
 *
 *  Author: Alf <naihe2010@126.com>
 */

#ifndef _APVLV_TXT_H_
#define _APVLV_TXT_H_

#include "ApvlvHtm.h"

namespace apvlv
{
class ApvlvTXT : public ApvlvHTML
{
  FILE_TYPE_DECLARATION (ApvlvTXT);

public:
  explicit ApvlvTXT (const string &filename, bool check = true)
      : ApvlvHTML (filename, check){};

  bool pageText (int pn, string &text) override;

  bool pageRender (int pn, int ix, int iy, double zm, int rot,
                   ApvlvWebview *webview) override;
  string
  pathMimeType (const string &path) override
  {
    return "text/plain";
  };
};

}
#endif

/* Local Variables: */
/* mode: c++ */
/* End: */

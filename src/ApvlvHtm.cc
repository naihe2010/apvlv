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
/* @CPPFILE ApvlvHtm.cc
 *
 *  Author: Alf <naihe2010@126.com>
 */
/* @date Created: 2011/09/16 13:51:04 Alf*/

#include "ApvlvHtm.h"
#include "ApvlvUtil.h"
#include <QWebEngineView>

namespace apvlv
{
ApvlvHTML::ApvlvHTML (const string &filename, bool check)
    : ApvlvFile (filename, check), mUri (filename)
{
}

ApvlvHTML::~ApvlvHTML () = default;

bool
ApvlvHTML::writefile (const char *filename)
{
  return false;
}

bool
ApvlvHTML::pagesize (int page, int rot, double *x, double *y)
{
  *x = HTML_DEFAULT_WIDTH;
  *y = HTML_DEFAULT_HEIGHT;
  return true;
}

int
ApvlvHTML::pagesum ()
{
  return 1;
}

bool
ApvlvHTML::pagetext (int, double, double, double, double, char **)
{
  return false;
}

bool
ApvlvHTML::render (int pn, int ix, int iy, double zm, int rot,
                   QWebEngineView *webview)
{
  auto url = QUrl (QString::fromStdString (mUri));
  webview->load (url);
  return false;
}

unique_ptr<ApvlvPoses>
ApvlvHTML::pagesearch (int pn, const char *str, bool reverse)
{
  return nullptr;
}

unique_ptr<ApvlvLinks>
ApvlvHTML::getlinks (int pn)
{
  return nullptr;
}

bool
ApvlvHTML::pageprint (int pn, QPrinter *cr)
{
  return false;
}
}

// Local Variables:
// mode: c++
// End:

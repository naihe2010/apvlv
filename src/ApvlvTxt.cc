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
/* @CPPFILE ApvlvTxt.cc
 *
 *  Author: Alf <naihe2010@126.com>
 */
/* @date Created: 2012/01/16 11:06:35 Alf*/

#include "ApvlvTxt.h"
#include "ApvlvUtil.h"
#include <filesystem>
#include <fstream>

#include <QWebEngineView>

namespace apvlv
{
ApvlvTXT::ApvlvTXT (const string &filename, bool check)
    : ApvlvFile (filename, check)
{
  auto path = filesystem::path (filename);
  auto size = filesystem::file_size (path);
  ifstream ifs (filename, ifstream::binary);
  if (ifs.is_open ())
    {
      mContent = make_unique<char> (size);
      ifs.read (mContent.get (), streamsize (size));
      ifs.close ();
    }
  else
    {
      throw bad_alloc ();
    }
  mLength = size;
}

bool
ApvlvTXT::writefile (const char *filename)
{
  return false;
}

bool
ApvlvTXT::pagesize (int pn, int rot, double *px, double *py)
{
  return false;
}

int
ApvlvTXT::pagesum ()
{
  return 1;
}

unique_ptr<ApvlvPoses>
ApvlvTXT::pagesearch (int pn, const char *str, bool reverse)
{
  return nullptr;
}

unique_ptr<ApvlvLinks>
ApvlvTXT::getlinks (int pn)
{
  return nullptr;
}

bool
ApvlvTXT::pagetext (int pn, double x1, double y1, double x2, double y2,
                    char **out)
{
  return false;
}

bool
ApvlvTXT::render (int, int, int, double, int, QImage *)
{
  return false;
}

bool
ApvlvTXT::render (int pn, int ix, int iy, double zm, int rot,
                  QWebEngineView *webview)
{
  webview->setZoomFactor (zm);
  // need impl
  return true;
}

bool
ApvlvTXT::pageprint (int pn, QPrinter *cr)
{
  return false;
}

optional<QByteArray>
ApvlvTXT::get_ocf_file (const string &path)
{
  QByteArray byte_array{ mContent.get () };
  return byte_array;
}

string
ApvlvTXT::get_ocf_mime_type (const string &path)
{
  return "text/plain";
}

}

// Local Variables:
// mode: c++
// End:

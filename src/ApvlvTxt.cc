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

#include <QFile>
#include <QWebEngineSettings>
#include <QWebEngineView>
#include <filesystem>
#include <fstream>

#include "ApvlvTxt.h"
#include "ApvlvUtil.h"

namespace apvlv
{
FILE_TYPE_DEFINITION (ApvlvTXT, { ".txt", ".text" });

ApvlvTXT::ApvlvTXT (const string &filename, bool check)
    : ApvlvFile (filename, check)
{
  QFile file (QString::fromStdString (filename));
  if (file.open (QIODevice::ReadOnly | QIODevice::Text) == false)
    throw bad_alloc ();

  mContent = file.readAll ();
  file.close ();
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
  auto settings = webview->settings ();
  settings->setDefaultTextEncoding ("UTF-8");
  webview->setZoomFactor (zm);
  QUrl url{ "apvlv:///1" };
  webview->load (url);
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
  return mContent;
}

string
ApvlvTXT::get_ocf_mime_type (const string &path)
{
  return "text/plain";
}
ApvlvSearchMatches
ApvlvTXT::searchPage (int pn, const string &text, bool is_case, bool is_reg)
{
  return ApvlvSearchMatches ();
}

}

// Local Variables:
// mode: c++
// End:

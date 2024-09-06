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

#include <QFile>
#include <QWebEngineSettings>
#include <fstream>

#include "ApvlvTxt.h"
#include "ApvlvWebViewWidget.h"

namespace apvlv
{
FILE_TYPE_DEFINITION (ApvlvTXT, { ".txt", ".text" });

bool
ApvlvTXT::pageText (int pn, const Rectangle &rect, string &text)
{
  QFile file (mUrl.path ());
  if (file.open (QIODeviceBase::ReadOnly) == false)
    return false;

  auto bytes = file.readAll ();
  text.append (bytes.toStdString ());
  file.close ();
  return true;
}

bool
ApvlvTXT::pageRender (int pn, double zm, int rot, WebView *webview)
{
  auto settings = webview->settings ();
  settings->setDefaultTextEncoding ("UTF-8");
  return ApvlvHTML::pageRender (pn, zm, rot, webview);
}

}

// Local Variables:
// mode: c++
// End:

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
/* @CPPFILE ApvlvOffice.cc
 *
 *  Author: Alf <naihe2010@126.com>
 */

#include <LibreOfficeKit/LibreOfficeKit.hxx>
#include <QDir>
#include <QTemporaryFile>

#include "ApvlvLibreOffice.h"

namespace apvlv
{
FILE_TYPE_DEFINITION ("libreOffice", ApvlvOFFICE,
                      { ".doc", ".docx", ".xls", ".xlsx" });

using namespace std;

unique_ptr<lok::Office> ApvlvOFFICE::mOffice;

bool
ApvlvOFFICE::load (const string &filename)
{
  initLokInstance ();

  mDoc
      = unique_ptr<lok::Document>{ mOffice->documentLoad (filename.c_str ()) };
  if (mDoc == nullptr)
    return false;

  mDoc->initializeForRendering ();
  return true;
}

ApvlvOFFICE::~ApvlvOFFICE () { mDoc = nullptr; }

int
ApvlvOFFICE::sum ()
{
  return mDoc->getParts ();
}

bool
ApvlvOFFICE::pageText (int pn, const Rectangle &rect, string &text)
{
  mDoc->setPart (pn);
  auto tmpname = QString ("%1/apvlv.%2.txt")
                     .arg (QDir::temp ().path ())
                     .arg (random ());
  mDoc->saveAs (tmpname.toStdString ().c_str (), "txt");

  QFile file (tmpname);
  if (file.open (QIODeviceBase::ReadOnly) == false)
    return false;

  auto bytes = file.readAll ();
  text.append (bytes.toStdString ());
  file.close ();
  file.remove ();
  return true;
}

bool
ApvlvOFFICE::pageRenderToImage (int pn, double zm, int rot, QImage *pix)
{
  mDoc->setPart (pn);
  QTemporaryFile file;
  if (file.open ())
    {
      mDoc->saveAs (file.fileName ().toStdString ().c_str (), "png");
      *pix = QImage (file.fileName (), "png");
      file.close ();
    }
  return true;
}

void
ApvlvOFFICE::initLokInstance ()
{
  static std::once_flag flag;
  std::call_once (flag, [] () {
    auto lok_path = ApvlvParams::instance ()->getStringOrDefault (
        "lok_path", DEFAULT_LOK_PATH);
    mOffice = unique_ptr<lok::Office>{ lok::lok_cpp_init (lok_path.c_str ()) };
  });
}

}

// Local Variables:
// mode: c++
// End:

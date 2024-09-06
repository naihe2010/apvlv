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
#include <QImage>
#include <QTemporaryFile>

#include "ApvlvLibreOffice.h"

namespace apvlv
{
FILE_TYPE_DEFINITION (ApvlvOFFICE, { ".doc", ".docx", ".xls", ".xlsx" });

unique_ptr<lok::Office> ApvlvOFFICE::mOffice
    = unique_ptr<lok::Office>{ lok::lok_cpp_init (APVLV_LOK_PATH) };

ApvlvOFFICE::ApvlvOFFICE (const string &filename, bool check)
    : File (filename, check)
{
  mDoc
      = unique_ptr<lok::Document>{ mOffice->documentLoad (filename.c_str ()) };
  if (mDoc == nullptr)
    throw bad_alloc ();

  mDoc->initializeForRendering ();
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
ApvlvOFFICE::pageRender (int pn, double zm, int rot, QImage *pix)
{
  /*
  auto width = static_cast<int>(ix * zm);
  auto height = static_cast<int>(iy * zm);
  auto size = static_cast<size_t>(width) * height * 4;
  auto buffer = make_unique<unsigned char>(size);
  mFrame->paintPartTile (buffer.get(), pn, 0, width, height, 0, 0, ix, iy);
  *pix = QImage(buffer.get(), width, height, QImage::Format_ARGB32);
   */
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
}

// Local Variables:
// mode: c++
// End:

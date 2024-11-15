/*
 * This file is part of the apvlv package
 *
 * Copyright (C) 2008 Alf.
 *
 * Contact: Alf <naihe2010@126.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2.0 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */
/* @CPPFILE ApvlvOCR.cc
 *
 *  Author: Alf <naihe2010@126.com>
 */

#include "ApvlvOCR.h"
#include "ApvlvParams.h"

namespace apvlv
{

using namespace std;

OCR::OCR ()
{
  auto lang = ApvlvParams::instance ()->getGroupStringOrDefault (
      "ocr", "lang", "eng+chi_sim");
  mTessBaseAPI.Init (nullptr, lang.c_str ());
}

OCR::~OCR () { mTessBaseAPI.End (); }

std::unique_ptr<TextAreaVector>
OCR::getTextArea (const QPixmap &pixmap)
{
  return nullptr;
}

std::unique_ptr<char>
OCR::getTextFromPixmap (const QPixmap &pixmap, QRect area)
{
  auto image = pixmap.toImage ();
  image = image.convertToFormat (QImage::Format_RGB888);
  mTessBaseAPI.SetImage (image.bits (), image.width (), image.height (), 3,
                         static_cast<int> (image.bytesPerLine ()));
  auto text = mTessBaseAPI.GetUTF8Text ();
  if (area.isValid ())
    {
      mTessBaseAPI.SetRectangle (area.left (), area.top (), area.width (),
                                 area.height ());
    }
  mTessBaseAPI.Clear ();
  return unique_ptr<char> (text);
}

}

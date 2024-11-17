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
/* @CPPFILE ApvlvOCR.h
 *
 *  Author: Alf <naihe2010@126.com>
 */

#ifndef _APVLV_OCR_H_
#define _APVLV_OCR_H_

#include <QPixmap>
#include <QRect>
#include <memory>
#include <tesseract/capi.h>
#include <vector>

namespace apvlv
{

using TextAreaVector = std::vector<QRect>;

class OCR final
{
public:
  OCR ();
  ~OCR ();

  std::unique_ptr<TextAreaVector> getTextArea (const QPixmap &pixmap);
  std::unique_ptr<char> getTextFromPixmap (const QPixmap &pixmap,
                                           QRect area = QRect ());

private:
  TessBaseAPI mTessBaseAPI;
};

}

#endif

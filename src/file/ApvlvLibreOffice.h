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
/* @CPPFILE ApvlvOffice.h
 *
 *  Author: Alf <naihe2010@126.com>
 */

#ifndef _APVLV_OFFICE_H_
#define _APVLV_OFFICE_H_

#include <LibreOfficeKit/LibreOfficeKit.hxx>
#include <memory>

#include "ApvlvFile.h"

namespace apvlv
{

const char *DEFAULT_LOK_PATH = "/usr/lib64/libreoffice/program";

class ApvlvOFFICE : public File
{
  FILE_TYPE_DECLARATION (ApvlvOFFICE);

public:
  bool load (const string &filename) override;
  ~ApvlvOFFICE ();

  int sum () override;

  bool pageText (int pn, const Rectangle &rect, string &text) override;

  bool pageRender (int pn, double zm, int rot, QImage *pix) override;

protected:
  std::unique_ptr<lok::Document> mDoc;

private:
  static std::unique_ptr<lok::Office> mOffice;
  static std::mutex mLokMutex;

  static void initLokInstance ();
};

}

#endif

/* Local Variables: */
/* mode: c++ */
/* End: */

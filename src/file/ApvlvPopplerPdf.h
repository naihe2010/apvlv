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
/* @CPPFILE ApvlvPdf.h
 *
 *  Author: Alf <naihe2010@126.com>
 */

#ifndef _APVLV_POPPLERPDF_H_
#define _APVLV_POPPLERPDF_H_

#include <qt6/poppler-qt6.h>

#include "ApvlvFile.h"

namespace apvlv
{

class ApvlvPDF : public File
{
  FILE_TYPE_DECLARATION (ApvlvPDF);

public:
  bool load (const std::string &filename) override;

  ~ApvlvPDF () override = default;

  SizeF pageSizeF (int page, int rot) override;

  int sum () override;

  bool pageRenderToImage (int pn, double zm, int rot, QImage *img) override;

  std::unique_ptr<WordListRectangle> pageSearch (int pn,
                                                 const char *s) override;

private:
  bool generateIndex ();
  void generateChildrenIndex (FileIndex &root_index,
                              const QVector<Poppler::OutlineItem> &outlines);

  std::unique_ptr<Poppler::Document> mDoc;
};
}
#endif

/* Local Variables: */
/* mode: c++ */
/* End: */

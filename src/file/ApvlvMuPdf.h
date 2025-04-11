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

#ifndef _APVLV_MUPDF_H_
#define _APVLV_MUPDF_H_

#include <mupdf/fitz.h>
#include <vector>

#include "ApvlvFile.h"

namespace apvlv
{

class ApvlvMuPDF : public File
{
  FILE_TYPE_DECLARATION (ApvlvMuPDF);

public:
  ApvlvMuPDF ();
  ~ApvlvMuPDF () override;

  bool load (const std::string &filename) override;

  [[nodiscard]] DISPLAY_TYPE
  getDisplayType () const override
  {
    return DISPLAY_TYPE::IMAGE;
  }

  SizeF pageSizeF (int page, int rot) override;

  int sum () override;

  bool pageIsOnlyImage (int pn) override;

  bool pageRenderToImage (int pn, double zm, int rot, QImage *img) override;

  std::optional<std::vector<Rectangle>>
  pageHighlight (int pn, const ApvlvPoint &pa, const ApvlvPoint &pb) override;

  bool pageText (int pn, const Rectangle &rect, std::string &text) override;

  std::unique_ptr<WordListRectangle> pageSearch (int pn,
                                                 const char *str) override;

private:
  fz_context *mContext;
  fz_document *mDoc;

  void pageRenderComments (int pn, fz_pixmap *pixmap,
                           const std::vector<Comment> &comments,
                           const fz_matrix &mat);
  void generateIndex ();
  void generateIndexRecursively (FileIndex &index, const fz_outline *outline);
};
}
#endif

/* Local Variables: */
/* mode: c++ */
/* End: */

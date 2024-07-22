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
/* @date Created: 2011/09/16 13:50:04 Alf*/

#ifndef _APVLV_POPPLERPDF_H_
#define _APVLV_POPPLERPDF_H_

#include <qt6/poppler-qt6.h>

#include "ApvlvFile.h"

namespace apvlv
{

using namespace Poppler;
class ApvlvPDF : public ApvlvFile
{
  FILE_TYPE_DECLARATION (ApvlvPDF);

public:
  explicit ApvlvPDF (const string &filename, bool check = true);

  ~ApvlvPDF () override = default;

  bool writefile (const char *filename) override;

  bool pagesize (int page, int rot, double *x, double *y) override;

  int pagesum () override;

  bool pagetext (int pn, double x1, double y1, double x2, double y2,
                 char **out) override;

  bool render (int, int, int, double, int, QImage *) override;

  bool annot_underline (int, double, double, double, double) override;

  bool annot_text (int, double, double, double, double,
                   const char *text) override;

  bool annot_update (int pn, ApvlvAnnotText *text) override;

  unique_ptr<ApvlvPoses> pagesearch (int pn, const char *s,
                                     bool reverse) override;

  ApvlvSearchMatches searchPage (int pn, const string &text, bool is_case,
                                 bool is_reg) override;

  ApvlvAnnotTexts getAnnotTexts (int pn) override;

  unique_ptr<ApvlvLinks> getlinks (int pn) override;

  bool pageprint (int pn, QPrinter *cr) override;

private:
  bool pdf_get_index ();
  void pdf_get_children_index (ApvlvFileIndex &root_index,
                               QVector<OutlineItem> &outlines);

  unique_ptr<Document> mDoc;
};
}
#endif

/* Local Variables: */
/* mode: c++ */
/* End: */

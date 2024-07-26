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

#ifndef _APVLV_QTPDF_H_
#define _APVLV_QTPDF_H_ 1

#include <QPdfBookmarkModel>
#include <QPdfDocument>

#include "ApvlvFile.h"

namespace apvlv
{

class ApvlvPDF : public File
{
  FILE_TYPE_DECLARATION (ApvlvPDF);

public:
  explicit ApvlvPDF (const string &filename, bool check = true);

  ~ApvlvPDF () override = default;

  bool pageSize (int page, int rot, double *x, double *y) override;

  int sum () override;

  bool pageRender (int, int, int, double, int, QImage *) override;

  bool pageText (int, string &text) override;

  bool pageAnnotUnderline (int, double, double, double, double) override;

  bool pageAnnotText (int, double, double, double, double,
                      const char *text) override;

  bool pageAnnotUpdate (int pn, ApvlvAnnotText *text) override;

  unique_ptr<ApvlvPoses> pageSearch (int pn, const char *s,
                                     bool reverse) override;

  ApvlvAnnotTexts pageAnnotTexts (int pn) override;

private:
  bool pdf_get_index ();
  void pdf_get_index_iter (FileIndex &, const QPdfBookmarkModel *,
                           const QModelIndex &);

  unique_ptr<QPdfDocument> mDoc;
};
}
#endif

/* Local Variables: */
/* mode: c++ */
/* End: */

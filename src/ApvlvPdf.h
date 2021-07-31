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

#ifndef _APVLV_PDF_H_
#define _APVLV_PDF_H_

#include "ApvlvFile.h"

namespace apvlv
{
class ApvlvPDF : public ApvlvFile
{
public:
  explicit ApvlvPDF (const char *filename, bool check = true);

  ~ApvlvPDF () override;

  bool writefile (const char *filename) override;

  bool pagesize (int page, int rot, double *x, double *y) override;

  int pagesum () override;

  bool pagetext (int pn, gdouble x1, gdouble y1, gdouble x2, gdouble y2,
                 char **out) override;

  bool render (int, int, int, double, int, GdkPixbuf *, char *) override;

  bool annot_underline (int, gdouble, gdouble, gdouble, gdouble) override;

  bool annot_text (int, gdouble, gdouble, gdouble, gdouble,
                   const char *text) override;

  bool pageselectsearch (int, int, int, double, int, GdkPixbuf *, char *, int,
                         ApvlvPoses *) override;

  ApvlvPoses *pagesearch (int pn, const char *s, bool reverse) override;

  ApvlvAnnotTexts *getAnnotTexts (int pn) override;

  ApvlvLinks *getlinks (int pn) override;

  ApvlvFileIndex *new_index () override;

  void free_index (ApvlvFileIndex *) override;

  bool pageprint (int pn, cairo_t *cr) override;

private:
  bool walk_poppler_index_iter (ApvlvFileIndex *root_index,
                                PopplerIndexIter *iter);

  PopplerDocument *mDoc;
};
}
#endif

/* Local Variables: */
/* mode: c++ */
/* End: */

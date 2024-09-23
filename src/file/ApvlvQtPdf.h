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
#include <QPdfSearchModel>

#include "ApvlvFile.h"
#include "ApvlvFileWidget.h"

namespace apvlv
{

class PDFWidget : public FileWidget
{
public:
  QWidget *createWidget () override;
  void showPage (int, double s) override;
  void showPage (int, const std::string &anchor) override;

  void setSearchSelect (int select) override;

  void setZoomrate (double zm) override;
};

class ApvlvPDF : public File
{
  FILE_TYPE_DECLARATION (ApvlvPDF);

public:
  bool load (const std::string &filename) override;

  ~ApvlvPDF () override = default;

  [[nodiscard]] DISPLAY_TYPE
  getDisplayType () const override
  {
    return DISPLAY_TYPE_IMAGE;
  }

  PDFWidget *getWidget () override;

  SizeF pageSizeF (int page, int rot) override;

  int sum () override;

  bool pageRender (int, double, int, QImage *) override;

  bool pageText (int, const Rectangle &rect, std::string &text) override;

  std::unique_ptr<WordListRectangle> pageSearch (int pn,
                                                 const char *s) override;

private:
  bool pdf_get_index ();
  void pdf_get_index_iter (FileIndex &, const QPdfBookmarkModel *,
                           const QModelIndex &);

  std::unique_ptr<QPdfDocument> mDoc;
  std::unique_ptr<QPdfSearchModel> mSearchModel;

  QWidget *mView;

  friend class PDFWidget;
};
}
#endif

/* Local Variables: */
/* mode: c++ */
/* End: */

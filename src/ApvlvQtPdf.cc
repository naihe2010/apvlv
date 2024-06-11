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
/* @CPPFILE ApvlvPdf.cc
 *
 *  Author: Alf <naihe2010@126.com>
 */
/* @date Created: 2011/09/16 13:50:18 Alf*/

#include <QInputDialog>
#include <QMessageBox>
#include <QPdfDocument>
#include <filesystem>
#include <fstream>

#include "ApvlvQtPdf.h"
#include "ApvlvUtil.h"

namespace apvlv
{
using namespace std;

ApvlvPDF::ApvlvPDF (const string &filename, bool check)
    : ApvlvFile (filename, check)
{
  mDoc = make_unique<QPdfDocument> ();
  auto res = mDoc->load (QString::fromStdString (filename));
  if (res == QPdfDocument::Error::IncorrectPassword)
    {
      auto text
          = QInputDialog::getText (nullptr, "password", "input password");
      auto pass = QByteArray::fromStdString (text.toStdString ());
      mDoc->setPassword (pass);
      res = mDoc->load (QString::fromStdString (filename));
    }

  if (res != QPdfDocument::Error::None)
    {
      throw std::bad_alloc ();
    }

  pdf_get_index ();
}

bool
ApvlvPDF::writefile (const char *filename)
{
  debug ("write %p to %s", this, filename);
  auto path = filesystem::absolute (filename);
  if (mDoc)
    {
      // need impl
      // auto ret = mDoc->write();
      // debug ("write pdf: %p to %s, return %d", mDoc, uri, ret);
      return true;
    }
  return false;
}

bool
ApvlvPDF::pagesize (int pn, int rot, double *x, double *y)
{
  if (mDoc == nullptr)
    return false;

  auto size = mDoc->pagePointSize (pn);
  if (rot == 0 || rot == 180)
    {
      *x = size.width ();
      *y = size.height ();
    }
  else
    {
      *x = size.height ();
      *y = size.width ();
    }
  return true;
}

int
ApvlvPDF::pagesum ()
{
  return mDoc ? mDoc->pageCount () : 0;
}

unique_ptr<ApvlvPoses>
ApvlvPDF::pagesearch (int pn, const char *str, bool is_reverse)
{
  if (mDoc == nullptr)
    return nullptr;

  /* auto page = mDoc->search (pn);
  auto results = page->search (str);
  if (is_reverse)
    reverse (results.begin (), results.end ());
  auto poses = make_unique<ApvlvPoses> ();
  for (auto res : results)
    {
      poses->push_back (
          { res.left (), res.bottom (), res.right (), res.top () });
    }*/
  auto poses = make_unique<ApvlvPoses> ();
  return poses;
}

bool
ApvlvPDF::pagetext (int pn, double x1, double y1, double x2, double y2,
                    char **out)
{

  return true;
}

bool
ApvlvPDF::render (int pn, int ix, int iy, double zm, int rot, QImage *pix)
{
  if (mDoc == nullptr)
    return false;

  auto xres = 72.0, yres = 72.0;
  xres *= zm;
  yres *= zm;

  QSize image_size{ int (ix * zm), int (iy * zm) };
  auto prot = QPdfDocumentRenderOptions::Rotation::None;
  if (rot == 90)
    prot = QPdfDocumentRenderOptions::Rotation::Clockwise90;
  if (rot == 180)
    prot = QPdfDocumentRenderOptions::Rotation::Clockwise180;
  if (rot == 270)
    prot = QPdfDocumentRenderOptions::Rotation::Clockwise270;
  QPdfDocumentRenderOptions options{};
  options.setRotation (prot);
  options.setScaledSize (image_size);
  auto image = mDoc->render (pn, image_size, options);
  *pix = std::move (image);
  return true;
}

unique_ptr<ApvlvLinks>
ApvlvPDF::getlinks (int pn)
{
  return make_unique<ApvlvLinks> ();
}

bool
ApvlvPDF::pdf_get_index ()
{
  auto outlines = mDoc->pageModel ();
  if (outlines == nullptr)
    return false;

  mIndex = { "", 0, "", FILE_INDEX_PAGE };
  return true;
}

bool
ApvlvPDF::pageprint (int pn, QPrinter *printer)
{
  return false;
}

bool
ApvlvPDF::annot_underline (int pn, double x1, double y1, double x2, double y2)
{
  return true;
}

bool
ApvlvPDF::annot_text (int pn, double x1, double y1, double x2, double y2,
                      const char *text)
{
  return true;
}

ApvlvAnnotTexts
ApvlvPDF::getAnnotTexts (int pn)
{
  return {};
}

bool
ApvlvPDF::annot_update (int pn, ApvlvAnnotText *text)
{
  return false;
}
}

// Local Variables:
// mode: c++
// End:

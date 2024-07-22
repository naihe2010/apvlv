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

#include <QApplication>
#include <QInputDialog>
#include <QPdfBookmarkModel>
#include <QPdfDocument>
#include <QPdfSearchModel>
#include <QThread>
#include <filesystem>
#include <fstream>

#include "ApvlvQtPdf.h"
#include "ApvlvUtil.h"

namespace apvlv
{
using namespace std;

FILE_TYPE_DEFINITION (ApvlvPDF, { ".pdf" });

ApvlvPDF::ApvlvPDF (const string &filename, bool check)
    : ApvlvFile (filename, check), mDoc (make_unique<QPdfDocument> ())
{
  auto res = mDoc->load (QString::fromStdString (filename));
  if (res == QPdfDocument::Error::IncorrectPassword)
    {
      if (QThread::currentThread () == QApplication::instance ()->thread ())
        {
          auto text
              = QInputDialog::getText (nullptr, "password", "input password");
          if (text.isEmpty ())
            throw bad_alloc ();

          auto pass = QByteArray::fromStdString (text.toStdString ());
          mDoc->setPassword (pass);
          res = mDoc->load (QString::fromStdString (filename));
        }
      else
        {
          qWarning ("file: %s has password, skip !!!", filename.c_str ());
        }
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
  qDebug ("write %p to %s", this, filename);
  auto path = filesystem::absolute (filename);
  if (mDoc)
    {
      // need impl
      // auto ret = mDoc->write();
      // qDebug ("write pdf: %p to %s, return %d", mDoc, uri, ret);
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

  auto qsearch = make_unique<QPdfSearchModel> ();
  qsearch->setDocument (mDoc.get ());
  qsearch->setSearchString (str);
  auto results = qsearch->resultsOnPage (pn);

  if (is_reverse)
    reverse (results.begin (), results.end ());
  auto poses = make_unique<ApvlvPoses> ();
  for (auto const &res : results)
    {
      auto rects = res.rectangles ();
      transform (rects.begin (), rects.end (), poses->begin (),
                 [] (auto const &rect) {
                   return ApvlvPos{ rect.left (), rect.bottom (),
                                    rect.right (), rect.top () };
                 });
    }
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
  auto bookmark_model = make_unique<QPdfBookmarkModel> ();
  bookmark_model->setDocument (mDoc.get ());
  mIndex = { "", 0, getFilename (), FILE_INDEX_FILE };
  pdf_get_index_iter (mIndex, bookmark_model.get (), QModelIndex ());
  return true;
}

void
ApvlvPDF::pdf_get_index_iter (ApvlvFileIndex &file_index,
                              const QPdfBookmarkModel *bookmark_model,
                              const QModelIndex &parent)
{
  for (auto row = 0; row < bookmark_model->rowCount (parent); ++row)
    {
      auto index = bookmark_model->index (row, 0, parent);
      auto title = bookmark_model->data (index, Qt::UserRole);
      auto level = bookmark_model->data (index, 257);
      auto page = bookmark_model->data (index, 258);
      auto location = bookmark_model->data (index, 259);

      ApvlvFileIndex child_index (title.toString ().toStdString (),
                                  page.toInt (), "", FILE_INDEX_PAGE);
      if (bookmark_model->hasChildren (index))
        {
          pdf_get_index_iter (child_index, bookmark_model, index);
        }

      file_index.mChildrenIndex.emplace_back (child_index);
    }
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

ApvlvSearchMatches
ApvlvPDF::searchPage (int pn, const string &text, bool is_case, bool is_reg)
{
  auto selection = mDoc->getAllText (pn);
  auto page_text = selection.text ();
  auto qlines = page_text.split ("\r\n");
  auto matches = ApvlvSearchMatches{};
  for (auto const &qline : qlines)
    {
      auto line = qline.toStdString ();
      if (line.find (text) != string::npos)
        {
          matches.emplace_back (ApvlvSearchMatch{ text, line });
        }
    }
  return matches;
}
}

// Local Variables:
// mode: c++
// End:

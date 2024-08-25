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

#include <QApplication>
#include <QInputDialog>
#include <QPdfBookmarkModel>
#include <QPdfDocument>
#include <QPdfPageNavigator>
#include <QPdfSearchModel>
#include <QThread>
#include <QtPdfWidgets/QPdfView>
#include <filesystem>
#include <fstream>

#include "../ApvlvUtil.h"
#include "ApvlvQtPdf.h"

namespace apvlv
{
using namespace std;

FILE_TYPE_DEFINITION (ApvlvPDF, { ".pdf" });

ApvlvPDF::ApvlvPDF (const string &filename, bool check)
    : File (filename, check), mDoc (make_unique<QPdfDocument> ()),
      mView (nullptr)
{
  auto res = mDoc->load (QString::fromLocal8Bit (filename));
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
          res = mDoc->load (QString::fromLocal8Bit (filename));
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

  mSearchModel = make_unique<QPdfSearchModel> ();
  mSearchModel->setDocument (mDoc.get ());

  pdf_get_index ();
}

QWidget *
ApvlvPDF::getWidget ()
{
  if (mView == nullptr)
    {
      auto view = new QPdfView;
      view->setDocument (mDoc.get ());
      view->setSearchModel (mSearchModel.get ());
      mView = view;
    }
  return mView;
}

bool
ApvlvPDF::widgetGoto (QWidget *widget, int pn)
{
  auto *view = dynamic_cast<QPdfView *> (widget);
  auto nav = view->pageNavigator ();
  nav->jump (pn, { 0, 0 });
  return true;
}

bool
ApvlvPDF::widgetGoto (QWidget *widget, const string &anchor)
{
  auto *view = dynamic_cast<QPdfView *> (widget);
  auto nav = view->pageNavigator ();
  auto link = QPdfLink{};
  nav->jump (link);
  return true;
}

bool
ApvlvPDF::widgetZoom (QWidget *widget, double zm)
{
  auto *view = dynamic_cast<QPdfView *> (widget);
  view->setZoomFactor (zm);
  return true;
}

bool
ApvlvPDF::widgetSearch (QWidget *widget, const string &word)
{
  auto *view = dynamic_cast<QPdfView *> (widget);
  mSearchModel->setSearchString (QString::fromLocal8Bit (word));
  view->setCurrentSearchResultIndex (1);
  return true;
}

bool
ApvlvPDF::pageSize (int pn, int rot, double *x, double *y)
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
ApvlvPDF::sum ()
{
  return mDoc ? mDoc->pageCount () : 0;
}

unique_ptr<WordListRectangle>
ApvlvPDF::pageSearch (int pn, const char *str, bool is_reverse)
{
  if (mDoc == nullptr)
    return nullptr;

  auto qsearch = make_unique<QPdfSearchModel> ();
  qsearch->setDocument (mDoc.get ());
  qsearch->setSearchString (str);
  auto results = qsearch->resultsOnPage (pn);
  if (results.empty ())
    return nullptr;

  if (is_reverse)
    reverse (results.begin (), results.end ());

  auto poses = make_unique<WordListRectangle> ();
  for (auto const &res : results)
    {
      WordRectangle word_rectangle;
      for (auto const &rect : res.rectangles ())
        {
          word_rectangle.push_back (
              { rect.left (), rect.bottom (), rect.right (), rect.top () });
        }
      poses->push_back (word_rectangle);
    }
  return poses;
}

bool
ApvlvPDF::pageRender (int pn, int ix, int iy, double zm, int rot, QImage *pix)
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
  *pix = mDoc->render (pn, image_size, options);
  return true;
}

bool
ApvlvPDF::pageText (int pn, string &text)
{
  if (mDoc == nullptr)
    return false;

  auto selection = mDoc->getAllText (pn);
  text = selection.text ().toStdString ();
  return true;
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
ApvlvPDF::pdf_get_index_iter (FileIndex &file_index,
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

      FileIndex child_index (title.toString ().toStdString (), page.toInt (),
                             "", FILE_INDEX_PAGE);
      if (bookmark_model->hasChildren (index))
        {
          pdf_get_index_iter (child_index, bookmark_model, index);
        }

      file_index.mChildrenIndex.emplace_back (child_index);
    }
}

bool
ApvlvPDF::pageAnnotUnderline (int pn, double x1, double y1, double x2,
                              double y2)
{
  return true;
}

bool
ApvlvPDF::pageAnnotText (int pn, double x1, double y1, double x2, double y2,
                         const char *text)
{
  return true;
}

ApvlvAnnotTexts
ApvlvPDF::pageAnnotTexts (int pn)
{
  return {};
}

bool
ApvlvPDF::pageAnnotUpdate (int pn, ApvlvAnnotText *text)
{
  return false;
}

}

// Local Variables:
// mode: c++
// End:

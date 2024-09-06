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
PDFWidget::createWidget ()
{
  if (mWidget == nullptr)
    {
      auto view = new QPdfView;

      auto pdf = dynamic_cast<ApvlvPDF *> (mFile);
      view->setDocument (pdf->mDoc.get ());
      view->setSearchModel (pdf->mSearchModel.get ());

      mHalScrollBar = view->horizontalScrollBar ();
      mValScrollBar = view->verticalScrollBar ();
      mWidget = view;
    }
  return mWidget;
}

PDFWidget *
ApvlvPDF::getWidget ()
{
  auto wid = new PDFWidget ();
  wid->setFile (this);
  wid->createWidget ();
  return wid;
}

SizeF
ApvlvPDF::pageSizeF (int pn, int rot)
{
  auto qsize = mDoc->pagePointSize (pn);
  if (rot == 0 || rot == 180)
    {
      return { qsize.width (), qsize.height () };
    }
  else
    {
      return { qsize.height (), qsize.width () };
    }
}

int
ApvlvPDF::sum ()
{
  return mDoc ? mDoc->pageCount () : 0;
}

unique_ptr<WordListRectangle>
ApvlvPDF::pageSearch (int pn, const char *str)
{
  if (mDoc == nullptr)
    return nullptr;

  mSearchModel->setSearchString (str);
  auto results = mSearchModel->resultsOnPage (pn);
  if (results.empty ())
    return nullptr;

  auto poses = make_unique<WordListRectangle> ();
  for (auto const &res : results)
    {
      WordRectangle word_rectangle;
      for (auto const &rect : res.rectangles ())
        {
          word_rectangle.rect_list.push_back (
              { rect.left (), rect.bottom (), rect.right (), rect.top () });
        }
      poses->push_back (word_rectangle);
    }
  return poses;
}

bool
ApvlvPDF::pageRender (int pn, double zm, int rot, QImage *pix)
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
ApvlvPDF::pageText (int pn, const Rectangle &rect, string &text)
{
  if (mDoc == nullptr)
    return false;

  auto selection = mDoc->getSelection (pn, { rect.p1x, rect.p1y },
                                       { rect.p2x, rect.p2y });
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

void
PDFWidget::showPage (int p, double s)
{
  auto *view = dynamic_cast<QPdfView *> (mWidget);
  auto nav = view->pageNavigator ();
  nav->jump (p, { 0, 0 });
  scrollTo (s, 0.0);
  mPageNumber = p;
  mScrollValue = s;
}
void
PDFWidget::showPage (int p, const string &anchor)
{
  auto *view = dynamic_cast<QPdfView *> (mWidget);
  auto nav = view->pageNavigator ();
  nav->jump (p, { 0, 0 });
  mPageNumber = p;
  mScrollValue = 0;
}

void
PDFWidget::setSearchSelect (int select)
{
  auto view = dynamic_cast<QPdfView *> (mWidget);
  auto pdf = dynamic_cast<ApvlvPDF *> (mFile);
  auto model = pdf->mSearchModel.get ();

  auto doc_select = 0;
  for (auto pn = 0; pn < mPageNumber; ++pn)
    {
      auto res = model->resultsOnPage (pn);
      doc_select += static_cast<int> (res.size ());
    }
  doc_select += select;
  view->setCurrentSearchResultIndex (doc_select);

  auto link = model->resultAtIndex (doc_select);
  auto scr = static_cast<int> (link.location ().y ());
  mValScrollBar->setValue (scr);

  qDebug ("link: %d,{%f,%f}: select: %d", link.page (), link.location ().x (),
          link.location ().y (), doc_select);
  mSearchSelect = select;
}

void
PDFWidget::setZoomrate (double zm)
{
  mZoomrate = zm;
  auto *view = dynamic_cast<QPdfView *> (mWidget);
  view->setZoomFactor (zm);
}
}

// Local Variables:
// mode: c++
// End:

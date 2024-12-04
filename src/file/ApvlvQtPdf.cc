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
#include <QPainter>
#include <QPdfPageNavigator>
#include <QThread>
#include <QtPdfWidgets/QPdfView>
#include <filesystem>
#include <fstream>

#include "ApvlvQtPdf.h"
#include "ApvlvUtil.h"

namespace apvlv
{
using namespace std;

FILE_TYPE_DEFINITION ("QtPdf", ApvlvPDF, { ".pdf" });

bool
ApvlvPDF::load (const string &filename)
{
  mDoc = make_unique<QPdfDocument> ();
  mView = nullptr;
  auto res = mDoc->load (QString::fromLocal8Bit (filename));
  if (res == QPdfDocument::Error::IncorrectPassword)
    {
      if (QThread::currentThread () == QApplication::instance ()->thread ())
        {
          auto text
              = QInputDialog::getText (nullptr, "password", "input password");
          if (text.isEmpty ())
            return false;

          auto pass = QByteArray::fromStdString (text.toStdString ());
          mDoc->setPassword (pass);
          res = mDoc->load (QString::fromLocal8Bit (filename));
        }
      else
        {
          qWarning () << "file: " << filename << " has password, skip !!!";
        }
    }

  if (res != QPdfDocument::Error::None)
    {
      return false;
    }

  mSearchModel = make_unique<QPdfSearchModel> ();
  mSearchModel->setDocument (mDoc.get ());

  generateIndex ();
  return true;
}

PDFWidget *
ApvlvPDF::getWidget ()
{
  auto wid = new PDFWidget ();
  wid->setFile (this);
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
ApvlvPDF::pageIsOnlyImage (int pn)
{
  auto sel = mDoc->getAllText (pn);
  auto has_text = sel.isValid ();
  return !has_text;
}

bool
ApvlvPDF::pageRenderToImage (int pn, double zm, int rot, QImage *pix)
{
  if (mDoc == nullptr)
    return false;

  using enum QPdfDocumentRenderOptions::Rotation;
  auto sizeF = pageSizeF (pn, rot);
  QSize image_size{ int (sizeF.width * zm), int (sizeF.height * zm) };
  auto prot = None;
  if (rot == 90)
    prot = Clockwise90;
  if (rot == 180)
    prot = Clockwise180;
  if (rot == 270)
    prot = Clockwise270;
  QPdfDocumentRenderOptions options{};
  options.setRotation (prot);
  options.setScaledSize (image_size);
  *pix = mDoc->render (pn, image_size, options);

  if (auto comments = mNote.getCommentsInPage (pn); !comments.empty ())
    {
      pageRenderComments (pn, pix, comments);
    }

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

void
ApvlvPDF::pageRenderComments (int pn, QImage *img,
                          const std::vector<Comment> &comments)
{
  auto model = make_unique<QPdfSearchModel> ();
  model->setDocument (mDoc.get ());
  QPainter painter (img);
  painter.setPen (QPen (Qt::blue, 0.4));

  // TODO
  // search not work, need better impl
  for (auto const &comment : comments)
    {
      model->setSearchString (QString::fromLocal8Bit (comment.quoteText));
      auto links = model->resultsOnPage (pn);
      if (links.empty ())
        continue;

      for (auto const &link : links)
        {
          auto rects = link.rectangles ();
          auto brect = rects[0];
          auto erect = rects[rects.count () - 1];
          painter.drawLine (brect.x(), brect.y() + brect.height (), erect.x(), brect.y() + brect.height ());
        }
    }

  painter.end ();
}

bool
ApvlvPDF::generateIndex ()
{
  auto bookmark_model = make_unique<QPdfBookmarkModel> ();
  bookmark_model->setDocument (mDoc.get ());
  mIndex = { "", 0, getFilename (), FileIndexType::FILE };
  getIndexIter (mIndex, bookmark_model.get (), QModelIndex ());
  return true;
}

void
ApvlvPDF::getIndexIter (FileIndex &file_index,
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
                             "", FileIndexType::PAGE);
      if (bookmark_model->hasChildren (index))
        {
          getIndexIter (child_index, bookmark_model, index);
        }

      file_index.mChildrenIndex.emplace_back (child_index);
    }
}

void
PDFWidget::setFile (File *file)
{
  mFile = file;
  auto pdf = dynamic_cast<ApvlvPDF *> (mFile);
  mPdfView.setDocument (pdf->mDoc.get ());
  mPdfView.setSearchModel (pdf->mSearchModel.get ());
}

void
PDFWidget::showPage (int p, double s)
{
  auto nav = mPdfView.pageNavigator ();
  nav->jump (p, { 0, 0 });
  scrollTo (s, 0.0);
  mPageNumber = p;
  mScrollValue = s;
}

void
PDFWidget::showPage (int p, const string &anchor)
{
  auto nav = mPdfView.pageNavigator ();
  nav->jump (p, { 0, 0 });
  mPageNumber = p;
  mScrollValue = 0;
}

void
PDFWidget::setSearchSelect (int select)
{
  auto pdf = dynamic_cast<ApvlvPDF *> (mFile);
  auto model = pdf->mSearchModel.get ();

  auto doc_select = 0;
  for (auto pn = 0; pn < mPageNumber; ++pn)
    {
      auto res = model->resultsOnPage (pn);
      doc_select += static_cast<int> (res.size ());
    }
  doc_select += select;
  mPdfView.setCurrentSearchResultIndex (doc_select);

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
  mPdfView.setZoomFactor (zm);
}
}

// Local Variables:
// mode: c++
// End:

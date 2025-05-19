/*
 * This file is part of the apvlv package
 *
 * Copyright (C) 2008 Alf.
 *
 * Contact: Alf <naihe2010@126.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2.0 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */
/* @CPPFILE ApvlvImageWidget.cc
 *
 *  Author: Alf <naihe2010@126.com>
 */

#include <QBuffer>
#include <QClipboard>
#include <QInputDialog>
#include <QMouseEvent>
#include <QToolTip>
#include <iostream>

#include "ApvlvImageWidget.h"

#include <QTimer>

namespace apvlv
{
using namespace std;

#ifdef APVLV_WITH_OCR
TextContainer::TextContainer (QWidget *parent) : Editor (parent)
{
  setReadOnly (false);
  setHorizontalScrollBarPolicy (Qt::ScrollBarAlwaysOff);
  setVerticalScrollBarPolicy (Qt::ScrollBarAlwaysOff);
}
#endif

ImageContainer::ImageContainer (QWidget *parent) : QLabel (parent)
{
  mCopyAction.setText (tr ("Copy"));
  QObject::connect (&mCopyAction, SIGNAL (triggered (bool)), this,
                    SLOT (copy ()));
  addAction (&mCopyAction);

  mUnderlineAction.setText (tr ("Underline"));
  QObject::connect (&mUnderlineAction, SIGNAL (triggered (bool)), this,
                    SLOT (underline ()));
  addAction (&mUnderlineAction);

  mCommentAction.setText (tr ("Comment"));
  QObject::connect (&mCommentAction, SIGNAL (triggered (bool)), this,
                    SLOT (comment ()));
  addAction (&mCommentAction);

  setContextMenuPolicy (Qt::ContextMenuPolicy::ActionsContextMenu);

  setMouseTracking (true);
  mHoverTimer = new QTimer (this);
  mHoverTimer->setSingleShot (true);
  QObject::connect (mHoverTimer, SIGNAL (timeout ()), this,
                    SLOT (handleHover ()));
}

void
ImageContainer::mousePressEvent (QMouseEvent *event)
{
  if (event->button () != Qt::MouseButton::LeftButton)
    return;

  redraw ();
  mIsSelected = true;
  mPressPosition = event->position ();
}

void
ImageContainer::mouseMoveEvent (QMouseEvent *event)
{
  QToolTip::hideText ();
  mLastMousePos = event->pos ();
  mHoverTimer->start (1000);
  if (!mIsSelected)
    return;

  mMovePosition = event->position ();

  auto range = selectionRange ();
  auto rect_list = mImageWidget->file ()->pageHighlight (
      mImageWidget->pageNumber (), range.first, range.second);
  if (!rect_list)
    return;

  mImageWidget->setSelects (rect_list.value ());
  redraw ();
}

void
ImageContainer::leaveEvent (QEvent *event)
{
  mHoverTimer->stop ();
  QLabel::leaveEvent (event);
}

void
ImageContainer::mouseReleaseEvent (QMouseEvent *event)
{
  mImageWidget->setSelects ({});
  mIsSelected = false;
}

bool
ImageContainer::renderImage (int pn, double zm, int rot)
{
  return mImageWidget->file ()->pageRenderToImage (pn, zm, rot, &mImage);
}

void
ImageContainer::redraw ()
{
  QImage img = mImage;
  if (!mImageWidget->searchResults ().empty ())
    {
      imageSelectSearch (&img, mImageWidget->zoomrate (),
                         mImageWidget->searchSelect (),
                         mImageWidget->searchResults ());
    }
  else if (!mImageWidget->selects ().empty ())
    {
      imageSelect (&img, mImageWidget->zoomrate (), mImageWidget->selects ());
    }
  setPixmap (QPixmap::fromImage (img));
  resize (img.size ());
}

pair<ApvlvPoint, ApvlvPoint>
ImageContainer::selectionRange ()
{
  double left = mPressPosition.x () / mImageWidget->zoomrate ();
  double top = mPressPosition.y () / mImageWidget->zoomrate ();
  double right = mMovePosition.x () / mImageWidget->zoomrate ();
  double bottom = mMovePosition.y () / mImageWidget->zoomrate ();
  return { { left, top }, { right, bottom } };
}

vector<Rectangle>
ImageContainer::selectionArea ()
{
  auto range = selectionRange ();
  auto rect_list = mImageWidget->file ()->pageHighlight (
      mImageWidget->pageNumber (), range.first, range.second);
  if (!rect_list)
    return {};
  return rect_list.value ();
}

string
ImageContainer::selectionText ()
{
  auto range = selectionRange ();
  string text;
  mImageWidget->file ()->pageText (
      mImageWidget->pageNumber (),
      { range.first.x, range.first.y, range.second.x, range.second.y }, text);
  return text;
}

void
ImageContainer::displayComment (QPoint pos)
{
  qDebug () << "display comment at: " << pos.x () << ":" << pos.y ();
  auto image_pos = pos;
  image_pos /= mImageWidget->zoomrate ();

  auto note = mImageWidget->file ()->getNote ();
  auto comments = note->getCommentsInPage (mImageWidget->pageNumber ());
  for (auto const &comment : comments)
    {
      QRectF rect{ comment.begin.x, comment.begin.y,
                   comment.end.x - comment.begin.x,
                   comment.end.y - comment.begin.y };
      if (rect.contains (image_pos))
        {
          qDebug () << "isVisible: " << QToolTip::isVisible ();
          QToolTip::showText (QCursor::pos (),
                              QString::fromStdString (comment.commentText),
                              this);
          break;
        }
    }
}

void
ImageContainer::copy ()
{
  qDebug () << "copy text";
  auto text = selectionText ();
  auto clipboard = QGuiApplication::clipboard ();
  clipboard->setText (QString::fromLocal8Bit (text));
  mImageWidget->setSelects ({});
  redraw ();
}

void
ImageContainer::underline ()
{
  qDebug () << "underline text";
  auto page = mImageWidget->pageNumber ();
  auto range = selectionRange ();
  auto text = selectionText ();
  if (!text.empty ())
    {
      auto note = mImageWidget->file ()->getNote ();
      Comment comment;
      comment.quoteText = text;
      comment.begin.set (page, &range.first);
      comment.end.set (page, &range.second);
      note->addComment (comment);
    }

  mImageWidget->setSelects ({});
  redraw ();
}

void
ImageContainer::comment ()
{
  qDebug () << "comment text";
  do
    {
      auto text = selectionText ();
      if (text.empty ())
        break;

      auto input_text = QInputDialog::getMultiLineText (this, tr ("Input"),
                                                        tr ("Comment"));
      auto commentText = input_text.trimmed ();
      if (commentText.isEmpty ())
        break;

      auto page = mImageWidget->pageNumber ();
      auto range = selectionRange ();
      auto note = mImageWidget->file ()->getNote ();
      Comment comment;
      comment.quoteText = text,
      comment.commentText = commentText.toStdString ();
      comment.begin.set (page, &range.first);
      comment.end.set (page, &range.second);
      note->addComment (comment);
    }
  while (false);

  mImageWidget->setSelects ({});
  redraw ();
}

void
ImageContainer::handleHover ()
{
  auto pos = mapFromGlobal (QCursor::pos ());
  if (rect ().contains (pos) && pos == mLastMousePos)
    {
      qDebug () << "hovered";
      displayComment (pos);
    }
}

ApvlvImage::ApvlvImage ()
{
  setAlignment (Qt::AlignCenter);
  setHorizontalScrollBarPolicy (Qt::ScrollBarPolicy::ScrollBarAsNeeded);
  setVerticalScrollBarPolicy (Qt::ScrollBarPolicy::ScrollBarAsNeeded);

  setWidget (&mImageContainer);
}

ApvlvImage::~ApvlvImage ()
{
  qDebug () << "ApvlvImage: " << this << " be freed";
}

#ifdef APVLV_WITH_OCR
void
ApvlvImage::ocrDisplay (bool is_ocr)
{
  if (is_ocr)
    {
      auto image = mImageContainer.pixmap ();
      auto text = mOCR.getTextFromPixmap (image);
      mTextContainer.setText (text.get ());
      if (widget () != &mTextContainer)
        {
          takeWidget ();
          setWidget (&mTextContainer);
        }
    }
  else
    {
      if (widget () != &mImageContainer)
        {
          takeWidget ();
          setWidget (&mImageContainer);
        }
    }
}

std::unique_ptr<char>
ApvlvImage::ocrGetText ()
{
  auto image = mImageContainer.pixmap ();
  auto text = mOCR.getTextFromPixmap (image);
  return text;
}
#endif

void
ImageWidget::showPage (int p, double s)
{
  if (p != mPageNumber)
    {
      if (!mImage.mImageContainer.renderImage (p, mZoomrate, mRotate))
        return;
    }
  mImage.mImageContainer.redraw ();
#ifdef APVLV_WITH_OCR
  mImage.mTextContainer.resize (mImage.mImageContainer.size ());
  if (mImage.widget () == &mImage.mTextContainer)
    {
      mImage.mTextContainer.setZoomrate (mZoomrate);
      mImage.ocrDisplay (true);
    }
#endif
  scrollTo (0.0, s);
  mPageNumber = p;
}

void
ImageWidget::showPage (int p, const string &anchor)
{
  showPage (p, 0.0f);
  mAnchor = anchor;
}

void
ImageWidget::setSearchResults (const WordListRectangle &wlr)
{
  mSearchResults = wlr;
  mImage.mImageContainer.redraw ();
}

void
ImageWidget::setZoomrate (double zm)
{
  if (mPageNumber != INVALID_PAGENUM)
    {
      if (mImage.mImageContainer.renderImage (mPageNumber, zm, mRotate))
        {
          mImage.mImageContainer.redraw ();
          mZoomrate = zm;
        }
#ifdef APVLV_WITH_OCR
      mImage.mTextContainer.setZoomrate (zm);
#endif
    }
  else
    {
      mZoomrate = zm;
    }
}

void
ImageWidget::setRotate (int rotate)
{
  if (mPageNumber != INVALID_PAGENUM)
    {
      if (mImage.mImageContainer.renderImage (mPageNumber, mZoomrate, rotate))
        {
          mImage.mImageContainer.redraw ();
          mRotate = rotate;
        }
    }
  else
    {
      mRotate = rotate;
    }
}

bool
imageSelect (QImage *pix, double zm, const vector<Rectangle> &rect_list)
{
  for (auto const &rect : rect_list)
    {
      auto p1xz = static_cast<int> (rect.p1x * zm);
      auto p2xz = static_cast<int> (rect.p2x * zm);
      auto p1yz = static_cast<int> (rect.p1y * zm);
      auto p2yz = static_cast<int> (rect.p2y * zm);

      for (auto w = p1xz; w < p2xz; ++w)
        {
          for (auto h = p1yz; h < p2yz; ++h)
            {
              QColor c = pix->pixelColor (w, h);
              c.setRgb (255 - c.red (), 255 - c.red (), 255 - c.red ());
              pix->setPixelColor (w, h, c);
            }
        }
    }

  return true;
}

bool
imageUnderline (QImage *pix, double zm, const vector<Rectangle> &rect_list)
{
  for (auto const &rect : rect_list)
    {
      auto p1xz = static_cast<int> (rect.p1x * zm);
      auto p2xz = static_cast<int> (rect.p2x * zm);
      auto p1yz = static_cast<int> (rect.p1y * zm);
      auto p2yz = static_cast<int> (rect.p2y * zm);

      for (auto w = p1xz; w < p2xz; ++w)
        {
          for (auto h = p1yz; h < p2yz; ++h)
            {
              QColor c = pix->pixelColor (w, h);
              c.setRgb (255 - c.red (), 255 - c.red (), 255 - c.red ());
              pix->setPixelColor (w, h, c);
            }
        }
    }

  return true;
}

bool
imageSelectSearch (QImage *pix, double zm, int select,
                   const WordListRectangle &wordlist)
{
  for (auto itr = wordlist.begin (); itr != wordlist.end (); ++itr)
    {
      auto rectangles = *itr;

      for (auto const &pos : rectangles.rect_list)
        {
          auto p1xz = static_cast<int> (pos.p1x * zm);
          auto p2xz = static_cast<int> (pos.p2x * zm);
          auto p1yz = static_cast<int> (pos.p2y * zm);
          auto p2yz = static_cast<int> (pos.p1y * zm);

          if (pix->format () == QImage::Format_ARGB32)
            {
              imageArgb32ToRgb32 (*pix, p1xz, p1yz, p2xz, p2yz);
            }

          if (std::distance (wordlist.begin (), itr) == select)
            {
              for (int w = p1xz; w < p2xz; ++w)
                {
                  for (int h = p1yz; h < p2yz; ++h)
                    {
                      QColor c = pix->pixelColor (w, h);
                      c.setRgb (255 - c.red (), 255 - c.red (),
                                255 - c.red ());
                      pix->setPixelColor (w, h, c);
                    }
                }
            }
          else
            {
              for (int w = p1xz; w < p2xz; ++w)
                {
                  for (int h = p1yz; h < p2yz; ++h)
                    {
                      QColor c = pix->pixelColor (w, h);
                      c.setRgb (255 - c.red () / 2, 255 - c.red () / 2,
                                255 - c.red () / 2);
                      pix->setPixelColor (w, h, c);
                    }
                }
            }
        }
    }

  return true;
}

}

// Local Variables:
// mode: c++
// End:

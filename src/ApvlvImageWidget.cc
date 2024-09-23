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
#include <QLabel>
#include <QMouseEvent>
#include <QScrollArea>
#include <QSplitter>
#include <iostream>

#include "ApvlvImageWidget.h"

namespace apvlv
{
using namespace std;

ImageContainer::ImageContainer (QWidget *parent) : QLabel (parent)
{
  auto copy = new QAction ("Copy");
  QObject::connect (copy, SIGNAL (triggered (bool)), this, SLOT (copy ()));
  addAction (copy);

  auto underline = new QAction ("Underline");
  QObject::connect (underline, SIGNAL (triggered (bool)), this,
                    SLOT (underline ()));
  addAction (underline);

  auto comment = new QAction ("Comment");
  QObject::connect (comment, SIGNAL (triggered (bool)), this,
                    SLOT (comment ()));
  addAction (comment);

  setContextMenuPolicy (Qt::ContextMenuPolicy::ActionsContextMenu);
}

void
ImageContainer::mousePressEvent (QMouseEvent *event)
{
  if (event->button () != Qt::MouseButton::LeftButton)
    return;

  redraw ();
  // qDebug () << "mouse press at " << event->position ();
  mIsSelected = true;
  mPressPosition = event->position ();
}

void
ImageContainer::mouseMoveEvent (QMouseEvent *event)
{
  if (!mIsSelected)
    return;

  // qDebug () << "mouse move to " << event->position ();
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
ImageContainer::mouseReleaseEvent (QMouseEvent *event)
{
  mImageWidget->setSelects ({});
  mIsSelected = false;
}

bool
ImageContainer::renderImage (int pn, double zm, int rot)
{
  return mImageWidget->file ()->pageRender (pn, zm, rot, &mImage);
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
  qDebug () << "copy text";
}

void
ImageContainer::comment ()
{
  qDebug () << "copy text";
}

ApvlvImage::ApvlvImage ()
{
  setAlignment (Qt::AlignCenter);
  setHorizontalScrollBarPolicy (Qt::ScrollBarPolicy::ScrollBarAsNeeded);
  setVerticalScrollBarPolicy (Qt::ScrollBarPolicy::ScrollBarAsNeeded);

  QScrollArea::setWidget (&mImageContainer);
}

ApvlvImage::~ApvlvImage () { qDebug ("ApvlvImage: %p be freed", this); }

void
ImageWidget::showPage (int p, double s)
{
  auto widget = dynamic_cast<ApvlvImage *> (mWidget);
  if (p != mPageNumber)
    {
      if (!widget->mImageContainer.renderImage (p, mZoomrate, mRotate))
        return;
    }
  widget->mImageContainer.redraw ();
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
  auto widget = dynamic_cast<ApvlvImage *> (mWidget);
  widget->mImageContainer.redraw ();
}

void
ImageWidget::setZoomrate (double zm)
{
  if (mPageNumber != INVALID_PAGENUM)
    {
      auto widget = dynamic_cast<ApvlvImage *> (mWidget);
      if (widget->mImageContainer.renderImage (mPageNumber, zm, mRotate))
        {
          widget->mImageContainer.redraw ();
          mZoomrate = zm;
        }
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
      auto widget = dynamic_cast<ApvlvImage *> (mWidget);
      if (widget->mImageContainer.renderImage (mPageNumber, mZoomrate, rotate))
        {
          widget->mImageContainer.redraw ();
          mRotate = rotate;
        }
    }
  else
    {
      mRotate = rotate;
    }
}

QWidget *
ImageWidget::createWidget ()
{
  auto widget = new ApvlvImage ();
  widget->mImageContainer.setImageWidget (this);
  mHalScrollBar = widget->horizontalScrollBar ();
  mValScrollBar = widget->verticalScrollBar ();
  return widget;
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

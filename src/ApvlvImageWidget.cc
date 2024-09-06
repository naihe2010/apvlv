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
#include <QMessageBox>
#include <QMouseEvent>
#include <QScrollArea>
#include <QSplitter>
#include <iostream>

#include "ApvlvImageWidget.h"

namespace apvlv
{
void
ImageContainer::mousePressEvent (QMouseEvent *event)
{
  if (event->button () != Qt::MouseButton::LeftButton)
    return;

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
  double left = mPressPosition.x () / mImageWidget->zoomrate ();
  double top = mPressPosition.y () / mImageWidget->zoomrate ();
  double right = mMovePosition.x () / mImageWidget->zoomrate ();
  double bottom = mMovePosition.y () / mImageWidget->zoomrate ();
  auto rect_list = mImageWidget->file ()->pageHighlight (
      mImageWidget->pageNumber (), { left, top }, { right, bottom });
  if (!rect_list)
    return;

  QImage img;
  if (mImageWidget->file ()->pageRender (mImageWidget->pageNumber (),
                                         mImageWidget->zoomrate (),
                                         mImageWidget->rotate (), &img)
      == false)
    return;

  imageSelect (&img, mImageWidget->zoomrate (), rect_list.value ());
  setPixmap (QPixmap::fromImage (img));
  resize (img.width (), img.height ());
}

void
ImageContainer::mouseReleaseEvent (QMouseEvent *event)
{
  mIsSelected = false;
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
  QImage img;
  if (mFile->pageRender (p, mZoomrate, mRotate, &img))
    {
      if (!mSearchResults.empty ())
        {
          imageSelectSearch (&img, mZoomrate, mSearchSelect, &mSearchResults);
        }
      auto widget = dynamic_cast<ApvlvImage *> (mWidget);
      widget->mImageContainer.setPixmap (QPixmap::fromImage (img));
      widget->mImageContainer.resize (img.width (), img.height ());
    }
  scrollTo (0.0, s);
  mPageNumber = p;
}

void
ImageWidget::showPage (int p, const string &anchor)
{
  showPage (p, 0.0f);
  mAnchor = anchor;
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
imageSelectSearch (QImage *pix, double zm, int select,
                   WordListRectangle *wordlist)
{
  for (auto itr = (*wordlist).begin (); itr != (*wordlist).end (); ++itr)
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

          if (std::distance ((*wordlist).begin (), itr) == select)
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

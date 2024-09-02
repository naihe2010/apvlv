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
#include <QScrollArea>
#include <QSplitter>
#include <iostream>

#include "ApvlvFile.h"
#include "ApvlvImageWidget.h"

namespace apvlv
{

ApvlvImage::ApvlvImage ()
{
  setAlignment (Qt::AlignCenter);
  setHorizontalScrollBarPolicy (Qt::ScrollBarPolicy::ScrollBarAsNeeded);
  setVerticalScrollBarPolicy (Qt::ScrollBarPolicy::ScrollBarAsNeeded);

  mImageContainer = new QLabel ();
  QScrollArea::setWidget (mImageContainer);
}

ApvlvImage::~ApvlvImage () { qDebug ("ApvlvImage: %p be freed", this); }

void
ImageWidget::showPage (int p, double s)
{
  QImage img;
  double x, y;
  if (mFile->pageSize (p, 0, &x, &y))
    {
      if (mFile->pageRender (p, int (x), int (y), mZoomrate, 0, &img))
        {
          auto widget = dynamic_cast<ApvlvImage *> (mWidget);
          widget->mImageContainer->setPixmap (QPixmap::fromImage (img));
          auto wx = static_cast<int> (x * mZoomrate);
          auto wy = static_cast<int> (y * mZoomrate);
          widget->mImageContainer->resize (wx, wy);
        }
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
  mWidget = new ApvlvImage ();
  auto scrollarea = dynamic_cast<QScrollArea *> (mWidget);
  mHalScrollBar = scrollarea->horizontalScrollBar ();
  mValScrollBar = scrollarea->verticalScrollBar ();
  return mWidget;
}

}

// Local Variables:
// mode: c++
// End:

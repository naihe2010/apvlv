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
/* @CPPFILE ApvlvImageWidget.h
 *
 *  Author: Alf <naihe2010@126.com>
 */

#ifndef _APVLV_IMAGEWIDGET_H_
#define _APVLV_IMAGEWIDGET_H_

#include <QLabel>
#include <QScrollArea>
#include <QScrollBar>
#include <iostream>
#include <map>

#include "ApvlvFileWidget.h"
#include "ApvlvUtil.h"

namespace apvlv
{

class ImageWidget;
class ImageContainer : public QLabel
{
  Q_OBJECT
public:
  ImageContainer (QWidget *parent = nullptr);

  void mousePressEvent (QMouseEvent *event) override;
  void mouseMoveEvent (QMouseEvent *event) override;
  void mouseReleaseEvent (QMouseEvent *event) override;

  virtual bool renderImage (int pn, double zm, int rot);
  virtual void redraw ();

  void
  setImageWidget (ImageWidget *image_widget)
  {
    mImageWidget = image_widget;
  }

private:
  bool mIsSelected{ false };

  QPointF mPressPosition;
  QPointF mMovePosition;

  ImageWidget *mImageWidget{ nullptr };

  friend class ImageWidget;

  QImage mImage;

  std::pair<ApvlvPoint, ApvlvPoint> selectionRange ();
  std::vector<Rectangle> selectionArea ();
  std::string selectionText ();

private slots:
  void copy ();
  void underline ();
  void comment ();
};

class ApvlvImage : public QScrollArea
{
  Q_OBJECT
public:
  ApvlvImage ();

  ~ApvlvImage () override;

private:
  ImageContainer mImageContainer;

  friend class ImageWidget;
};

class ImageWidget : public FileWidget
{
public:
  QWidget *createWidget () override;

  void showPage (int, double s) override;
  void showPage (int, const std::string &anchor) override;

  void setSearchResults (const WordListRectangle &wlr) override;
  void setZoomrate (double zm) override;
  void setRotate (int rotate) override;
};

bool imageSelectSearch (QImage *pix, double zm, int select,
                        const WordListRectangle &poses);

bool imageSelect (QImage *pix, double zm, const std::vector<Rectangle> &poses);

bool imageUnderline (QImage *pix, double zm,
                     const std::vector<Rectangle> &poses);

}

#endif

/* Local Variables: */
/* mode: c++ */
/* End: */

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
#include <QMainWindow>
#include <QScrollArea>
#include <QVBoxLayout>

#include "ApvlvFileWidget.h"
#include "ApvlvUtil.h"
#ifdef APVLV_WITH_OCR
#include "ApvlvEditor.h"
#include "ApvlvOCR.h"
#endif

namespace apvlv
{

#ifdef APVLV_WITH_OCR
class TextContainer : public Editor
{
  Q_OBJECT
public:
  explicit TextContainer (QWidget *parent = nullptr);
  ~TextContainer () override = default;
};
#endif

class ImageWidget;
class ImageContainer : public QLabel
{
  Q_OBJECT
public:
  explicit ImageContainer (QWidget *parent = nullptr);

  void mousePressEvent (QMouseEvent *event) override;
  void mouseReleaseEvent (QMouseEvent *event) override;
  void mouseMoveEvent (QMouseEvent *event) override;
  void leaveEvent (QEvent *event) override;

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

  QTimer *mHoverTimer;
  QPoint mLastMousePos;

  ImageWidget *mImageWidget{ nullptr };

  friend class ImageWidget;

  QImage mImage;
  QAction mCopyAction;
  QAction mUnderlineAction;
  QAction mCommentAction;

  std::pair<ApvlvPoint, ApvlvPoint> selectionRange ();
  std::vector<Rectangle> selectionArea ();
  std::string selectionText ();
  void displayComment (QPoint pos);

private slots:
  void copy ();
  void underline ();
  void comment ();
  void handleHover ();
};

class ApvlvImage : public QScrollArea
{
  Q_OBJECT
public:
  ApvlvImage ();

  ~ApvlvImage () override;

#ifdef APVLV_WITH_OCR
  void ocrDisplay (bool replace);
  std::unique_ptr<char> ocrGetText ();
#endif

private:
  ImageContainer mImageContainer;
#ifdef APVLV_WITH_OCR
  TextContainer mTextContainer;
  OCR mOCR;
#endif

  friend class ImageWidget;
};

class ImageWidget : public FileWidget
{
public:
  ImageWidget ()
  {
    mImage.mImageContainer.setImageWidget (this);
    mHalScrollBar = mImage.horizontalScrollBar ();
    mValScrollBar = mImage.verticalScrollBar ();
  }

  [[nodiscard]] QWidget *
  widget () override
  {
    return &mImage;
  }

  void showPage (int pn, double s) override;
  void showPage (int pn, const std::string &anchor) override;

  void setSearchResults (const WordListRectangle &wlr) override;
  void setZoomrate (double zm) override;
  void setRotate (int rotate) override;

private:
  ApvlvImage mImage{};
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

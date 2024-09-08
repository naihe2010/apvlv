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
/* @CPPFILE ApvlvFileWidget.h
 *
 *  Author: Alf <naihe2010@126.com>
 */

#ifndef _APVLV_FILE_WIDGET_H_
#define _APVLV_FILE_WIDGET_H_

#include <QScrollBar>
#include <string>

#include "ApvlvFile.h"

namespace apvlv
{

using namespace std;

const int WORD_WIDTH_DEFAULT = 40;
const int LINE_HEIGHT_DEFAULT = 15;

const double DEFAULT_ZOOMRATE = 1.3f;
const int INVALID_PAGENUM = -1;

class File;
class FileWidget : public QObject
{
  Q_OBJECT
public:
  FileWidget () = default;

  ~FileWidget () override
  {
    if (mWidget)
      mWidget->deleteLater ();
  };

  QWidget *
  widget ()
  {
    if (mWidget == nullptr)
      mWidget = createWidget ();
    return mWidget;
  }

  File *
  file () const
  {
    return mFile;
  }

  virtual void
  setFile (File *file)
  {
    mFile = file;
  }

  virtual int
  pageNumber ()
  {
    return mPageNumber;
  }

  virtual string
  anchor ()
  {
    return mAnchor;
  }

  virtual double
  zoomrate ()
  {
    return mZoomrate;
  }

  virtual void
  showPage (int pn, double rate)
  {
    mPageNumber = pn;
    mScrollValue = rate;
  }

  virtual void
  showPage (int pn, const string &anchor)
  {
    mPageNumber = pn;
    mAnchor = anchor;
  }

  virtual void scroll (int times, int w, int h);

  virtual double scrollRate ();

  virtual void scrollTo (double s, double y);

  virtual void scrollUp (int times);

  virtual void scrollDown (int times);

  virtual void scrollLeft (int times);

  virtual void scrollRight (int times);

  virtual void
  setZoomrate (double zm)
  {
    mZoomrate = zm;
  }

  virtual void
  setRotate (int rot)
  {
    mRotate = rot;
  }

  virtual int
  rotate ()
  {
    return mRotate;
  }

  void
  setAnchor (const string &anchor)
  {
    mAnchor = anchor;
  }

  virtual void
  setSearchSelect (int select)
  {
    mSearchSelect = select;
  }

  [[nodiscard]] virtual int
  searchSelect () const
  {
    return mSearchSelect;
  }

  virtual void
  setSearchStr (const string &str)
  {
    mSearchStr = str;
  }

  [[nodiscard]] virtual string
  searchStr () const
  {
    return mSearchStr;
  }

  virtual void
  setSearchResults (const WordListRectangle &wlr)
  {
    mSearchResults = wlr;
  }

  virtual const WordListRectangle &
  searchResults ()
  {
    return mSearchResults;
  }

  virtual void
  setSelects (const vector<Rectangle> &rect_list)
  {
    mSelects = rect_list;
  }

  virtual const vector<Rectangle> &
  selects ()
  {
    return mSelects;
  }

protected:
  virtual QWidget *createWidget () = 0;

  QWidget *mWidget{ nullptr };
  QScrollBar *mValScrollBar{ nullptr }, *mHalScrollBar{ nullptr };

  File *mFile{ nullptr };

  int mPageNumber{ INVALID_PAGENUM };
  double mScrollValue{ 0.0f };
  string mAnchor;
  double mZoomrate{ DEFAULT_ZOOMRATE };
  int mRotate{ 0 };

  string mSearchStr;
  WordListRectangle mSearchResults;
  int mSearchSelect{ 0 };

  vector<Rectangle> mSelects;
};
}

#endif

/* Local Variables: */
/* mode: c++ */
/* End: */

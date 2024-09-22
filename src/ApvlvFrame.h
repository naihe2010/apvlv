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
/* @CPPFILE ApvlvFrame.h
 *
 *  Author: Alf <naihe2010@126.com>
 */

#ifndef _APVLV_FRAME_H_
#define _APVLV_FRAME_H_

#include <QBoxLayout>
#include <QFileSystemWatcher>
#include <QScrollArea>
#include <QScrollBar>
#include <QSplitter>
#include <iostream>
#include <map>

#include "ApvlvCmds.h"
#include "ApvlvContent.h"
#include "ApvlvFile.h"
#include "ApvlvFileWidget.h"

namespace apvlv
{
struct ApvlvDocPosition
{
  int pagenum;
  double scrollrate;
};

using ApvlvDocPositionMap = map<char, ApvlvDocPosition>;

struct ApvlvWord
{
  CharRectangle pos;
  string word;
};

struct ApvlvLine
{
  CharRectangle pos;
  vector<ApvlvWord> mWords;
};

class ApvlvFrame;
class ApvlvStatus : public QFrame
{
  Q_OBJECT
public:
  ApvlvStatus ();

  ~ApvlvStatus () override = default;

  void setActive (bool act);

  void showMessages (const vector<string> &msgs);
};

const int DEFAULT_CONTENT_WIDTH = 30;

class FileWidget;
class ApvlvView;
class ApvlvFrame : public QFrame
{
  Q_OBJECT
public:
  explicit ApvlvFrame (ApvlvView *);

  virtual ~ApvlvFrame ();

  virtual bool reload ();

  virtual void inuse (bool use);

  virtual bool inuse ();

  virtual ApvlvFrame *copy ();

  virtual void setDirIndex (const string &path);

  virtual bool loadfile (const string &file, bool check, bool show_content);

  virtual const char *filename ();

  virtual int pageNumber ();

  virtual void showpage (int pn, double s);
  virtual void showpage (int pn, const string &anchor);
  virtual void refresh (int pn, double s);

  virtual void setActive (bool act);

  virtual void updateStatus ();

  virtual bool print (int ct);

  virtual bool totext (const char *name);

  virtual bool rotate (int ct);

  virtual void markposition (char s);

  virtual void setzoom (double zm);

  virtual void setzoom (const char *z);

  virtual void jump (char s);

  virtual void nextpage (int times);

  virtual void prepage (int times);

  virtual void halfnextpage (int times);

  virtual void halfprepage (int times);

  virtual bool search (const char *str, bool reverse);

  virtual void gotolink (int ct);

  virtual void returnlink (int ct);

  bool loadLastPosition (const string &filename);
  bool saveLastPosition (const string &filename);

  void contentShowPage (const FileIndex *index, bool force);

  virtual int getskip ();
  virtual void setskip (int ct);

  virtual void toggleContent ();

  virtual void toggleContent (bool enabled);

  virtual bool toggledControlContent (bool is_right);

  virtual bool isShowContent ();

  virtual bool isControlledContent ();

  virtual CmdReturn process (int has, int times, uint keyval);

  ApvlvView *mView;

  static ApvlvFrame *findByWidget (QWidget *widget);

protected:
  File *mFile{};

  FileIndex mDirIndex{};

  bool mInuse;

  unique_ptr<QFileSystemWatcher> mWatcher;

  string mFilestr;

  uint mProCmd;

  char mSearchCmd{};
  unique_ptr<WordListRectangle> mSearchResults;
  string mSearchStr;

  enum
  {
    NORMAL,
    FITWIDTH,
    FITHEIGHT,
    CUSTOM
  } mZoommode;

  bool mZoominit{};

  int mSkip{};

  ApvlvDocPositionMap mPositions;

  // the main menubar
  QBoxLayout *mVbox;

  // the main panel
  QSplitter *mPaned;
  int mContentWidth;

  // content panel
  ApvlvContent *mContent;

  // the custom widget
  unique_ptr<FileWidget> mWidget;

  // if active
  bool mActive{};

  // status bar
  ApvlvStatus *mStatus;

  void setWidget (DISPLAY_TYPE type);
  void unsetHighlight ();
  void setHighlightAndIndex (const WordListRectangle &poses, int sel);
  bool needsearch (const string &str, bool reverse);
  CmdReturn subprocess (int ct, uint key);

signals:
  void indexGenerited (const FileIndex &);
};
}

#endif

/* Local Variables: */
/* mode: c++ */
/* End: */

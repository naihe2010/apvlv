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

#include <QFileSystemWatcher>
#include <QHBoxLayout>
#include <QScrollArea>
#include <QScrollBar>
#include <QSplitter>
#include <QVBoxLayout>
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

using ApvlvDocPositionMap = std::map<char, ApvlvDocPosition>;

struct ApvlvWord
{
  CharRectangle pos;
  std::string word;
};

struct ApvlvLine
{
  CharRectangle pos;
  std::vector<ApvlvWord> mWords;
};

class ApvlvFrame;
class ApvlvStatus : public QFrame
{
  Q_OBJECT
public:
  ApvlvStatus ();

  ~ApvlvStatus () override = default;

  void setActive (bool act);

  void showMessages (const std::vector<std::string> &msgs);

private:
  QHBoxLayout mLayout;
};

const int DEFAULT_CONTENT_WIDTH = 300;

class FileWidget;
class ApvlvView;
class ApvlvFrame final : public QFrame
{
  Q_OBJECT
public:
  explicit ApvlvFrame (ApvlvView *view);

  ~ApvlvFrame () override;

  bool reload ();

  void inuse (bool use);

  bool inuse ();

  ApvlvFrame *copy ();

  void setDirIndex (const std::string &path);

  bool loadfile (const std::string &file, bool check, bool show_content);

  bool loadUri (const std::string &uri);

  const char *filename ();

  int pageNumber ();

  void showpage (int pn, double s);
  void showpage (int pn, const std::string &anchor);
  void refresh (int pn, double s);

  void setActive (bool act);

  void updateStatus ();

  bool isStatusHidden ();

  void statusShow ();

  void statusHide ();

  bool print (int ct);

  bool totext (const char *name);

  bool rotate (int ct);

  void markposition (char s);

  void setzoom (double zm);

  void setzoom (const char *z);

  void jump (char s);

  void nextpage (int times);

  void prepage (int times);

  void halfnextpage (int times);

  void halfprepage (int times);

  bool search (const char *str, bool reverse);

  void gotoLink (int ct);

  void returnLink (int ct);

  bool loadLastPosition (const std::string &filename);
  bool saveLastPosition (const std::string &filename);

  void contentShowPage (const FileIndex *index, bool force);

  int getskip ();
  void setskip (int ct);

  void toggleContent ();

  void toggleContent (bool enabled);

  bool toggledControlContent (bool is_right);

  bool isShowContent ();

  bool isControlledContent ();

  CmdReturn process (int has, int times, uint keyval);

  ApvlvView *mView;

  void
  focusInEvent (QFocusEvent *event) override
  {
    emit focusIn ();
  }

  static ApvlvFrame *findByWidget (QWidget *widget);

private:
  std::unique_ptr<File> mFile;

  FileIndex mDirIndex{};

  bool mInuse;

  std::unique_ptr<QFileSystemWatcher> mWatcher;

  std::string mFilestr;

  uint mProCmd;

  char mSearchCmd{};
  std::unique_ptr<WordListRectangle> mSearchResults;
  std::string mSearchStr;

  enum class ZoomMode
  {
    NORMAL,
    FITWIDTH,
    FITHEIGHT,
    CUSTOM
  };
  ZoomMode mZoommode;

  bool mZoominit{};

  int mSkip{};

  ApvlvDocPositionMap mPositions;

  // the main menubar
  QVBoxLayout mVbox;

  // the main panel
  QSplitter mPaned;
  int mContentWidth;

  // content panel
  ApvlvContent mContent;

  // the custom widget
  std::unique_ptr<FileWidget> mWidget;

  // if active
  bool mActive{};

  // status bar
  ApvlvStatus mStatus;

  void setWidget (DISPLAY_TYPE type);
  void unsetHighlight ();
  void setHighlightAndIndex (const WordListRectangle &poses, int sel);
  bool needsearch (const std::string &str, bool reverse);
  CmdReturn subprocess (int ct, uint key);

signals:
  void indexGenerited (const FileIndex &index);
  void focusIn ();
};
}

#endif

/* Local Variables: */
/* mode: c++ */
/* End: */

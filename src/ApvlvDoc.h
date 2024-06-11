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
/* @PPCPPFILE ApvlvDoc.h
 *
 *  Author: Alf <naihe2010@126.com>
 */
/* @date Created: 2008/09/30 00:00:00 Alf */

#ifndef _APVLV_DOC_H_
#define _APVLV_DOC_H_

#include <QBoxLayout>
#include <QLabel>
#include <QUrl>
#include <QWebEngineUrlRequestJob>
#include <QWebEngineUrlSchemeHandler>
#include <iostream>
#include <list>
#include <map>
#include <vector>

#include "ApvlvCore.h"
#include "ApvlvFile.h"
#include "ApvlvUtil.h"

namespace apvlv
{

class ApvlvDoc;
class ApvlvSchemeHandler : public QWebEngineUrlSchemeHandler
{
  Q_OBJECT
public:
  explicit ApvlvSchemeHandler (ApvlvDoc *doc) { mDoc = doc; }
  ~ApvlvSchemeHandler () = default;

  void requestStarted (QWebEngineUrlRequestJob *job) override;

private:
  ApvlvDoc *mDoc;

signals:
  void webpageUpdated (const string &key);
};

struct PrintData
{
  ApvlvFile *file;
  uint frmpn, endpn;
};

struct ApvlvDocPosition
{
  int pagenum;
  double scrollrate;
};

typedef map<char, ApvlvDocPosition> ApvlvDocPositionMap;

struct ApvlvWord
{
  ApvlvPos pos;
  string word;
};

struct ApvlvLine
{
  ApvlvPos pos;
  vector<ApvlvWord> mWords;
};

enum class ApvlvVisualMode
{
  VISUAL_NONE,
  VISUAL_V,
  VISUAL_CTRL_V
};

class ApvlvDoc;
class ApvlvDocCache
{
public:
  explicit ApvlvDocCache (ApvlvFile *);

  ~ApvlvDocCache ();

  void set (uint p, double zm, uint rot, bool delay = true);

  void load ();

  [[nodiscard]] int getpagenum () const;

  const QImage &getbuf (bool wait);

  [[nodiscard]] int getwidth () const;

  [[nodiscard]] int getheight () const;

  double getHeightOfLine (double y);

  double getWidthOfWord (double x, double y);

  ApvlvLinks *getlinks ();

  bool mInverted;

  ApvlvWord *getword (double x, double y);

  ApvlvLine *getline (double y);

  vector<ApvlvLine *> getlines (double y1, double y2);

  vector<ApvlvPos> getSelected (ApvlvPoint last, ApvlvPoint cur,
                                ApvlvVisualMode visual);

  bool getAvailableSpace (ApvlvPos pos, ApvlvPos *outpos);

  void setAnnot (const ApvlvAnnotText &annot) const;

  ApvlvAnnotText *annotAtPos (ApvlvPos vpos);

private:
  ApvlvFile *mFile;
  double mZoom;
  uint mRotate;
  int mPagenum;
  QImage mBuf;
  int mWidth;
  int mHeight;

  unique_ptr<ApvlvLinks> mLinks;
  vector<ApvlvLine> mLines;
  ApvlvAnnotTexts mAnnotTexts;

  void preGetLines (int x1, int y1, int x2, int y2);
  void sortLines ();
  void prepare_add (const char *word, ApvlvPoses *results);
};

class ApvlvImage : public QLabel
{
  Q_OBJECT
public:
  ApvlvImage (ApvlvDoc *doc, int id);
  ~ApvlvImage () = default;

  int mId;

  void toCacheSize (double x, double y, ApvlvDocCache *cache, double *rx,
                    double *ry);

  void setImage (const QImage &buf);

private:
  ApvlvDoc *mDoc;

  void contextMenuEvent (QContextMenuEvent *ev) override;
  void mouseMoveEvent (QMouseEvent *evt) override;
  void mousePressEvent (QMouseEvent *evt) override;
  void mouseReleaseEvent (QMouseEvent *evt) override;

public slots:
  void copytoclipboard_cb ();
  void underline_cb ();
  void annotate_cb ();
  void comment_cb ();
};

class ApvlvDoc : public ApvlvCore
{
  Q_OBJECT
public:
  explicit ApvlvDoc (ApvlvView *, const char *zm = "NORMAL",
                     bool cache = false);

  ~ApvlvDoc () override;

  ApvlvDoc *copy () override;

  bool usecache () override;

  void usecache (bool use) override;

  bool loadfile (const string &src, bool check, bool show_content) override;

  int pagenumber () override;

  bool print (int ct) override;

  bool totext (const char *name) override;

  bool rotate (int ct) override;

  void markposition (char s) override;

  void setzoom (const char *z) override;

  void jump (char s) override;

  void showpage (int p, double s) override;

  void showpage (int p, const string &anchor) override;

  void contentShowPage (ApvlvFileIndex *index, bool force);

  void nextpage (int times) override;

  void prepage (int times) override;

  void halfnextpage (int times) override;

  void halfprepage (int times) override;

  void scrollup (int times) override;
  void scrolldown (int times) override;
  void scrollleft (int times) override;
  void scrollright (int times) override;

  bool search (const char *str, bool reverse) override;

  bool find (const char *str) override;

  returnType process (int hastimes, int times, uint keyval) override;

  void gotolink (int ct) override;

  void returnlink (int ct) override;

  void srtranslate (int &rtimes, double &sr, bool single2continuous);

  void updateUrlHandler (ApvlvFile *file);

  static void webEngineRegisterScheme ();

private:
  void blank (ApvlvImage *img);

  static void blankarea (ApvlvImage *image, ApvlvPos pos, uchar *buffer,
                         int width, int height);

  void doubleClickBlank (ApvlvImage *img, double x, double y);

  void togglevisual (int type);

  void yank (ApvlvImage *image, int times);

  void annotUnderline (ApvlvImage *image);

  void annotText (ApvlvImage *image);

  void commentText (ApvlvImage *image);

  returnType subprocess (int ct, uint key);

  int convertindex (int p);

  void markselection ();

  bool needsearch (const char *str, bool reverse = false);

  void refresh () override;

  void display () override;

  bool reload () override;

  bool savelastposition (const char *filename);

  bool loadlastposition (const string &filename);

  void setDisplayType (DISPLAY_TYPE type);

  void updateLastPoint (double x, double y);

  void updateCurPoint (double x, double y, bool updateLast);

  ApvlvVisualMode mInVisual;

  uint mLastpress;

  ApvlvPoint mLastPoint, mCurPoint;

  ApvlvDocPositionMap mPositions;
  vector<ApvlvDocPosition> mLinkPositions;

  unique_ptr<ApvlvDocCache> mCurrentCache[3];

  QBoxLayout *mVbox;

  // image viewer
  unique_ptr<ApvlvImage> mImg[3];

  ApvlvImage *mCurrentImage;
  ApvlvAnnotText *mCurrentAnnotText;

  shared_ptr<ApvlvSchemeHandler> mSchemeHanlder;

  friend class ApvlvDocCache;
  friend class ApvlvImage;

private slots:
  void edit_annotation_cb ();
  void delete_annotation_cb ();
  void on_mouse ();
  void monitor_callback ();
};
}

#endif

/* Local Variables: */
/* mode: c++ */
/* End: */

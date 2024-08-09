/*
 * This file is part of the apvlv package
 * Copyright (C) <2008>  <Alf>
 *
 * Contact: Alf <naihe2010@126.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */
/* @CPPFILE File.h
 *
 *  Author: Alf <naihe2010@126.com>
 */

#ifndef _APVLV_FILE_H_
#define _APVLV_FILE_H_

#include <QPrinter>
#include <iostream>
#include <map>
#include <memory>
#include <vector>

#include "ApvlvSearch.h"

namespace apvlv
{

using namespace std;
typedef enum
{
  DISPLAY_TYPE_IMAGE = 0,
  DISPLAY_TYPE_HTML = 1,
  DISPLAY_TYPE_CUSTOM = 2,
} DISPLAY_TYPE;

//
// link to an url, or a page num
//
struct ApvlvLink
{
  int mPage;
};

struct ApvlvCover
{
  string content;
  string mime_type;
};

typedef vector<ApvlvLink> ApvlvLinks;

struct ApvlvPoint
{
  double x, y;
};

//
// position of a search result, or just an area
//
struct Rectangle
{
  double p1x, p1y, p2x, p2y;
};

using CharRectangle = Rectangle;

using WordRectangle = vector<CharRectangle>;

using WordListRectangle = vector<WordRectangle>;

enum ApvlvAnnotType
{
  APVLV_ANNOT_UNDERLINE,
  APVLV_ANNOT_TEXT,
};

struct ApvlvAnnotText
{
  ApvlvAnnotType type;
  CharRectangle pos;
  string text;
};

using ApvlvAnnotTexts = vector<ApvlvAnnotText>;

enum FileIndexType
{
  FILE_INDEX_PAGE,
  FILE_INDEX_FILE,
  FILE_INDEX_DIR
};

class FileIndex
{
public:
  FileIndex () : page (0), type (FILE_INDEX_PAGE){};
  FileIndex (const string &title, int page, const string &path,
             FileIndexType type);
  FileIndex (string &&title, int page, string &&path, FileIndexType type);
  ~FileIndex ();

  void loadDirectory (const string &path1);
  void appendChild (const FileIndex &child);
  [[nodiscard]] const FileIndex *findIndex (const FileIndex &tmp_index) const;

  bool operator== (const FileIndex &) const;

  string title;
  int page;
  string path;
  string anchor;
  FileIndexType type;
  vector<FileIndex> mChildrenIndex;
};

class ApvlvWebview;
class File
{
public:
  const static map<string, vector<string> > &supportMimeTypes ();
  static vector<string> supportFileExts ();

  static File *newFile (const string &filename, bool check = false);

  File (const string &filename, bool check);

  virtual ~File ();

  // File methods
  [[nodiscard]] virtual DISPLAY_TYPE
  getDisplayType () const
  {
    return DISPLAY_TYPE_HTML;
  }

  // when display type is custom, using this
  virtual QWidget *
  getWidget ()
  {
    return nullptr;
  }

  virtual bool
  widgetGoto (QWidget *widget, int pn)
  {
    return false;
  }

  virtual bool
  widgetGoto (QWidget *widget, const string &anchor)
  {
    return false;
  }

  virtual bool
  widgetZoom (QWidget *widget, double zm)
  {
    return false;
  }

  virtual bool
  widgetSearch (QWidget *widget, const string &word)
  {
    return false;
  }

  const string &
  getFilename ()
  {
    return mFilename;
  }

  const ApvlvCover &
  getCover ()
  {
    return mCover;
  }

  const FileIndex &
  getIndex ()
  {
    return mIndex;
  }

  virtual bool
  writeFile (const char *filename)
  {
    return false;
  }

  virtual unique_ptr<SearchFileMatch> grepFile (const string &seq,
                                                bool is_case, bool is_regex,
                                                atomic<bool> &is_abort);

  virtual int
  sum ()
  {
    return 1;
  };

  // Page methods
  virtual bool
  pageSize (int page, int rot, double *x, double *y)
  {
    return false;
  }

  virtual bool
  pageRender (int page, int x, int y, double zm, int rot, QImage *pix)
  {
    return false;
  }

  virtual bool pageRender (int pn, int x, int y, double zm, int rot,
                           ApvlvWebview *webview);

  virtual bool
  pageText (int pn, string &text)
  {
    return false;
  }

  virtual unique_ptr<ApvlvLinks>
  pageLinks (int pn)
  {
    return nullptr;
  }

  virtual bool
  pageSelection (int pn, double left, double top, double width, double height,
                 string &text)
  {
    return false;
  }

  virtual unique_ptr<WordListRectangle>
  pageSearch (int pn, const char *str, bool reverse)
  {
    return nullptr;
  }

  virtual bool pageSelectSearch (int pn, int ix, int iy, double zm, int rot,
                                 QImage *pix, int select,
                                 WordListRectangle *poses);

  virtual bool pageSelectSearch (int pn, int ix, int iy, double zm, int rot,
                                 ApvlvWebview *webview, int select,
                                 WordListRectangle *poses);

  virtual bool pageAnnotUnderline (int, double, double, double, double);

  virtual bool pageAnnotText (int, double, double, double, double,
                              const char *text);

  virtual bool pageAnnotUpdate (int, ApvlvAnnotText *text);

  virtual ApvlvAnnotTexts pageAnnotTexts (int pn);

  // path methods
  virtual optional<QByteArray> pathContent (const string &path);

  virtual string pathMimeType (const string &path);

  virtual int pathPageNumber (const string &path);

protected:
  static int registerClass (const string &mime,
                            function<File *(const string &)> fun,
                            initializer_list<string> exts);

  string mFilename;
  FileIndex mIndex;
  std::vector<string> mPages;
  std::map<string, int> srcPages;
  std::map<string, string> srcMimeTypes;
  ApvlvCover mCover;

  WordListRectangle mSearchPoses;
  int mSearchSelect;

private:
  static map<string, std::vector<string> > mSupportMimeTypes;
  static map<string, function<File *(const string &)> > mSupportClass;

  optional<QByteArray> pathContentHtml (int, int, int, double, int);
  optional<QByteArray> pathContentPng (int, int, int, double, int);
};

#define FILE_TYPE_DECLARATION(cls)                                            \
private:                                                                      \
  static int class_id_of_##cls
#define FILE_TYPE_DEFINITION(cls, ...)                                        \
  int cls::class_id_of_##cls = registerClass (                                \
      #cls,                                                                   \
      [] (const string &filename) -> File * { return new cls (filename); },   \
      __VA_ARGS__)

};

#endif

/* Local Variables: */
/* mode: c++ */
/* End: */

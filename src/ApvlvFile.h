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
/* @CPPFILE ApvlvFile.h
 *
 *  Author: Alf <naihe2010@126.com>
 */
/* @date Created: 2009/11/20 19:37:44 Alf*/

#ifndef _APVLV_FILE_H_
#define _APVLV_FILE_H_

#include <QPrinter>
#include <QWebEngineView>
#include <iostream>
#include <map>
#include <memory>
#include <vector>

namespace apvlv
{

using namespace std;
typedef enum
{
  DISPLAY_TYPE_IMAGE = 0,
  DISPLAY_TYPE_HTML = 1,
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
struct ApvlvPos
{
  double p1x, p1y, p2x, p2y;
};

typedef vector<ApvlvPos> ApvlvPoses;

enum ApvlvAnnotType
{
  APVLV_ANNOT_UNDERLINE,
  APVLV_ANNOT_TEXT,
};

struct ApvlvAnnotText
{
  ApvlvAnnotType type;
  ApvlvPos pos;
  string text;
};

typedef vector<ApvlvAnnotText> ApvlvAnnotTexts;

enum ApvlvFileIndexType
{
  FILE_INDEX_PAGE,
  FILE_INDEX_FILE,
  FILE_INDEX_DIR
};

class ApvlvFileIndex
{
public:
  ApvlvFileIndex () : page (0), type (FILE_INDEX_PAGE){};
  ApvlvFileIndex (const string &title, int page, const string &path,
                  ApvlvFileIndexType type);
  ApvlvFileIndex (string &&title, int page, string &&path,
                  ApvlvFileIndexType type);
  ~ApvlvFileIndex ();

  void loadDirectory (const string &path1);
  void appendChild (const ApvlvFileIndex &child);
  const ApvlvFileIndex *findIndex (const ApvlvFileIndex &tmp_index) const;

  bool operator== (const ApvlvFileIndex &) const;

  string title;
  int page;
  string path;
  string anchor;
  ApvlvFileIndexType type;
  vector<ApvlvFileIndex> mChildrenIndex;
};

class ApvlvFile
{
public:
  const static map<string, vector<string> > &supportMimeTypes ();
  const static vector<string> &supportFileExts ();

  ApvlvFile (const string &filename, bool check);

  virtual ~ApvlvFile ();

  static ApvlvFile *newFile (const string &filename, bool check = false);

  virtual bool writefile (const char *filename) = 0;

  virtual bool pagesize (int page, int rot, double *x, double *y) = 0;

  virtual int pagesum () = 0;

  virtual bool pagetext (int, double, double, double, double, char **) = 0;

  virtual bool render (int, int, int, double, int, QImage *);

  virtual bool render (int pn, int, int, double, int, QWebEngineView *);

  virtual unique_ptr<ApvlvPoses> pagesearch (int pn, const char *str,
                                             bool reverse)
      = 0;

  virtual bool pageselectsearch (int, int, int, double, int, QImage *,
                                 ApvlvPoses *);

  virtual bool annot_underline (int, double, double, double, double);

  virtual bool annot_text (int, double, double, double, double,
                           const char *text);

  virtual bool annot_update (int, ApvlvAnnotText *text);

  virtual ApvlvAnnotTexts getAnnotTexts (int pn);

  virtual unique_ptr<ApvlvLinks> getlinks (int pn) = 0;

  virtual bool pageprint (int pn, QPrinter *cr) = 0;

  virtual optional<QByteArray> get_ocf_file (const string &path);

  virtual string get_ocf_mime_type (const string &path);

  virtual int get_ocf_page (const string &path);

  virtual DISPLAY_TYPE get_display_type ();

  const string &
  getFilename ()
  {
    return mFilename;
  }

  const ApvlvCover &
  get_cover ()
  {
    return mCover;
  }

  const ApvlvFileIndex &
  get_index ()
  {
    return mIndex;
  }

  bool hasByteArray (const string &key);
  optional<const QByteArray> getByteArray (const string &key);
  void cacheByteArray (const string &key, const QByteArray &array);

protected:
  string mFilename;
  ApvlvFileIndex mIndex;
  std::vector<string> mPages;
  std::map<string, int> srcPages;
  std::map<string, string> srcMimeTypes;
  ApvlvCover mCover;

private:
  QMap<string, QByteArray> mCacheByteArray;

  const static std::map<string, std::vector<string> > mSupportMimeTypes;
  const static std::vector<string> mSupportFileExts;
};

};

#endif

/* Local Variables: */
/* mode: c++ */
/* End: */

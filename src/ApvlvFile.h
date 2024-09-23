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

#include "ApvlvParams.h"
#include "ApvlvSearch.h"

namespace apvlv
{

enum DISPLAY_TYPE
{
  DISPLAY_TYPE_IMAGE = 0,
  DISPLAY_TYPE_HTML = 1,
  DISPLAY_TYPE_CUSTOM = 2,
};

//
// link to an url, or a page num
//
struct ApvlvLink
{
  int mPage;
};

struct ApvlvCover
{
  std::string content;
  std::string mime_type;
};

using ApvlvLinks = std::vector<ApvlvLink>;

struct ApvlvPoint
{
  double x, y;
};

struct Size
{
  int width;
  int height;
};

struct SizeF
{
  double width;
  double height;
};

//
// position of a search result, or just an area
//
struct Rectangle
{
  double p1x, p1y, p2x, p2y;
};

using CharRectangle = Rectangle;

struct WordRectangle
{
  std::string word;
  std::vector<CharRectangle> rect_list;
};

using WordListRectangle = std::vector<WordRectangle>;

enum FileIndexType
{
  FILE_INDEX_PAGE,
  FILE_INDEX_FILE,
  FILE_INDEX_DIR
};

class FileWidget;
class FileIndex
{
public:
  FileIndex () : page (0), type (FILE_INDEX_PAGE){};
  FileIndex (const std::string &title, int page, const std::string &path,
             FileIndexType type);
  FileIndex (std::string &&title, int page, std::string &&path,
             FileIndexType type);
  ~FileIndex ();

  void loadDirectory (const std::string &path1);
  void appendChild (const FileIndex &child);
  [[nodiscard]] const FileIndex *findIndex (const FileIndex &tmp_index) const;

  bool operator== (const FileIndex &) const;

  std::string title;
  int page;
  std::string path;
  std::string anchor;
  FileIndexType type;
  std::vector<FileIndex> mChildrenIndex;
};

class WebView;
class File
{
public:
  const static std::map<std::string, std::vector<std::string> > &
  supportMimeTypes ();
  static std::vector<std::string> supportFileExts ();

  static std::unique_ptr<File> loadFile (const std::string &filename);

  virtual ~File ();

  virtual bool load (const std::string &filename) = 0;

  // File methods
  [[nodiscard]] virtual DISPLAY_TYPE
  getDisplayType () const
  {
    return DISPLAY_TYPE_HTML;
  }

  virtual FileWidget *
  getWidget ()
  {
    return nullptr;
  }

  const std::string &
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

  virtual std::unique_ptr<SearchFileMatch>
  grepFile (const std::string &seq, bool is_case, bool is_regex,
            std::atomic<bool> &is_abort);

  virtual int
  sum ()
  {
    return 1;
  };

  // Page methods
  Size
  pageSize (int page, int rot)
  {
    auto sizef = pageSizeF (page, rot);
    return { static_cast<int> (sizef.width), static_cast<int> (sizef.height) };
  }

  virtual SizeF
  pageSizeF (int page, int rot)
  {
    return { 0, 0 };
  }

  virtual int
  pageNumberWrap (int page)
  {
    auto scrdoc = gParams->getBoolOrDefault ("autoscrolldoc");
    int c = sum ();

    if (page >= 0 && page < c)
      {
        return page;
      }
    else if (page >= c && scrdoc)
      {
        return page % c;
      }
    else if (page < 0 && scrdoc)
      {
        while (page < 0)
          page += c;
        return page;
      }
    else
      {
        return -1;
      }
  }

  virtual bool
  pageRender (int page, double zm, int rot, QImage *pix)
  {
    return false;
  }

  virtual bool pageRender (int pn, double zm, int rot, WebView *webview);

  virtual std::unique_ptr<ApvlvLinks>
  pageLinks (int pn)
  {
    return nullptr;
  }

  virtual bool
  pageText (int pn, const Rectangle &rect, std::string &text)
  {
    return false;
  }

  virtual std::unique_ptr<WordListRectangle>
  pageSearch (int pn, const char *str)
  {
    return nullptr;
  }

  virtual std::optional<std::vector<Rectangle> >
  pageHighlight (int pn, const ApvlvPoint &pa, const ApvlvPoint &pb)
  {
    return std::nullopt;
  }

  // path methods
  virtual std::optional<QByteArray> pathContent (const std::string &path);

  virtual std::string pathMimeType (const std::string &path);

  virtual int pathPageNumber (const std::string &path);

protected:
  File () = default;

  static int registerClass (const std::string &mime,
                            std::function<File *()> fun,
                            std::initializer_list<std::string> exts);

  std::string mFilename;
  FileIndex mIndex;
  std::vector<std::string> mPages;
  std::map<std::string, int> srcPages;
  std::map<std::string, std::string> srcMimeTypes;
  ApvlvCover mCover;

private:
  static std::map<std::string, std::vector<std::string> > mSupportMimeTypes;
  static std::map<std::string, std::function<File *()> > mSupportClass;

  std::optional<QByteArray> pathContentHtml (int, double, int);
  std::optional<QByteArray> pathContentPng (int, double, int);
};

#define FILE_TYPE_DECLARATION(cls)                                            \
private:                                                                      \
  static int class_id_of_##cls
#define FILE_TYPE_DEFINITION(cls, ...)                                        \
  int cls::class_id_of_##cls = registerClass (                                \
      #cls, [] () -> File * { return new cls (); }, __VA_ARGS__)

};

#endif

/* Local Variables: */
/* mode: c++ */
/* End: */

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

#ifndef _APVLV_FILE_INDEX_H_
#define _APVLV_FILE_INDEX_H_

#include <QImage>
#include <list>
#include <string>

#include "ApvlvSearch.h"

namespace apvlv
{

enum class FileIndexType
{
  PAGE,
  FILE,
  DIR
};

class FileIndex
{
public:
  FileIndex () = default;
  FileIndex (const std::string &title, int page, const std::string &path,
             FileIndexType type);
  ~FileIndex ();

  friend bool
  operator== (const FileIndex &a, const FileIndex &b)
  {
    return a.title == b.title && a.page == b.page && a.path == b.path
           && a.anchor == b.anchor;
  }

  void loadDirectory (const std::string &path1);
  void moveChildChildren (const FileIndex &other_index);
  void removeChild (const FileIndex &child);

  /* public variables */
  FileIndexType type{ FileIndexType::PAGE };
  std::string title;
  int page{ 0 };
  std::string path;
  std::string anchor;
  std::list<FileIndex> mChildrenIndex;

  /* public file variables */
  std::int64_t size{ 0 };
  std::int64_t mtime{ 0 };
};

}

#endif

/* Local Variables: */
/* mode: c++ */
/* End: */

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
/* @CPPFILE File.cc
 *
 *  Author: Alf <naihe2010@126.com>
 */

#include <QBuffer>
#include <QDebug>
#include <algorithm>
#include <filesystem>
#include <iostream>

#include "ApvlvFile.h"
#include "ApvlvFileIndex.h"
#include "ApvlvUtil.h"

namespace apvlv
{

using namespace std;

void
FileIndex::loadDirectory (const string &path1)
{
  auto exts = FileFactory::supportFileExts ();

  try
    {
      for (auto &entry : filesystem::directory_iterator (
               path1, filesystem::directory_options::follow_directory_symlink))
        {
          if (entry.is_directory ())
            {
              auto index
                  = FileIndex (entry.path ().filename ().string (), 0,
                               entry.path ().string (), FileIndexType::DIR);
              index.loadDirectory (entry.path ().string ());
              auto last = entry.last_write_time ();
              index.mtime = filesystemTimeToMSeconds (last);
              if (!index.mChildrenIndex.empty ())
                {
                  mChildrenIndex.emplace_back (index);
                }
            }
          else if (entry.file_size () > 0)
            {
              auto file_ext = filenameExtension (entry.path ().string ());
              if (find (exts.cbegin (), exts.cend (), file_ext)
                  != exts.cend ())
                {
                  auto index = FileIndex (entry.path ().filename ().string (),
                                          0, entry.path ().string (),
                                          FileIndexType::FILE);
                  index.size = static_cast<int64_t> (entry.file_size ());
                  auto last = entry.last_write_time ();
                  index.mtime = filesystemTimeToMSeconds (last);
                  mChildrenIndex.emplace_back (index);
                }
            }
        }
    }
  catch (filesystem::filesystem_error &err)
    {
      qWarning () << "file system error: " << err.what ();
    }
}

void
FileIndex::moveChildChildren (const FileIndex &other_index)
{
  Q_ASSERT (type == FileIndexType::FILE);
  Q_ASSERT (other_index.type == FileIndexType::FILE);
  mChildrenIndex = other_index.mChildrenIndex;
}

void
FileIndex::removeChild (const FileIndex &child)
{
  mChildrenIndex.remove (child);
}

FileIndex::FileIndex (const string &title, int page, const string &path,
                      FileIndexType type)
{
  this->title = title;
  this->page = page;
  this->path = path;
  this->type = type;
}

FileIndex::~FileIndex () = default;
}

// Local Variables:
// mode: c++
// End:

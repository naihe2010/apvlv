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
/* @CPPFILE ApvlvEditor.cc
 *
 *  Author: Alf <naihe2010@126.com>
 */

#include <QDir>
#include <filesystem>
#include <fstream>

#include "ApvlvFile.h"
#include "ApvlvNote.h"
#include "ApvlvUtil.h"

namespace apvlv
{
using namespace std;

void
Location::set (int page1, const ApvlvPoint *point1, int offset1,
               const std::string &path1, const std::string &anchor1)
{
  page = page1;
  x = point1->x;
  y = point1->y;
  offset = offset1;
  path = path1;
  anchor = anchor1;
}

Note::Note (File *file) : mFile (file) {}

Note::~Note () = default;

bool
Note::loadStreamV1 (std::ifstream &is)
{
  string line;

  do
    {
      getline (is, line); // skip path
    }
  while (!line.starts_with ("# Meta Data"));

  // parse ^- tag:
  getline (is, line);
  if (line.starts_with ("- tag: "))
    {
      stringstream s (line);
      string dash;
      string token;
      string tags;
      s >> dash >> token >> tags;
      for (auto it = tags.begin (); it != tags.end ();)
        {
          if (*it != ',')
            {
              ++it;
              continue;
            }

          auto offset = it - tags.begin ();
          string tag = tags.substr (0, offset);
          mTagSet.insert (tag);

          tags = tags.substr (offset + 1);
          it = tags.begin ();
        }
    }

  // parse ^- score:
  getline (is, line);
  if (line.starts_with ("- score: "))
    {
      stringstream s (line);
      string pad;
      s >> pad >> pad >> mScore;
    }

  while (!line.starts_with ("# Comments"))
    {
      getline (is, line);
    }

  while (true)
    {
      getline (is, line);
      stringstream s (line);

      string dash;
      int number;
      s >> dash >> number;
      if (dash != "-")
        {
          break;
        }

      Comment comment;
      is >> comment;
      mCommentList.insert ({ comment.begin, comment });
    }

  while (!line.starts_with ("# References"))
    {
      getline (is, line);
    }
  do
    {
      getline (is, line);
      stringstream s (line);
      string dash;
      string ref;
      s >> dash >> ref;
      mReferences.insert (ref);
    }
  while (line.starts_with ("- "));

  while (!line.starts_with ("# Links"))
    {
      getline (is, line);
    }
  do
    {
      getline (is, line);
      stringstream s (line);
      string dash;
      string link;
      s >> dash >> link;
      mLinks.insert (link);
    }
  while (line.starts_with ("- "));

  return true;
}

bool
Note::loadStream (std::ifstream &is)
{
  string line;

  getline (is, line);
  if (!line.starts_with ("---"))
    {
      qWarning () << "note header not found";
      return false;
    }

  string version;
  is >> version >> version;
  if (version == "1")
    {
      return loadStreamV1 (is);
    }

  return false;
}

bool
Note::load (std::string_view sv)
{
  string path = string (sv);
  if (path.empty ())
    path = notePathOfFile (mFile);

  ifstream ifs{ path };
  if (!ifs.is_open ())
    return false;

  auto ret = loadStream (ifs);
  ifs.close ();
  return ret;
}

bool
Note::dumpStream (std::ostream &os)
{
  os << "---" << endl;
  os << "version: 1" << endl;
  os << "path: " << mFile->getFilename () << endl;

  os << "---" << endl;
  os << "# Meta Data" << endl;
  os << "- tag: ";
  std::ranges::for_each (mTagSet,
                         [&os] (const string &tag) { os << tag << ","; });
  os << endl;
  os << "- score: " << mScore << endl;
  os << endl;

  os << "# Comments" << endl;
  auto index = 0;
  std::ranges::for_each (mCommentList,
                         [&os, &index] (const pair<Location, Comment> &pair1) {
                           os << " - " << index++ << endl;
                           os << pair1.second;
                         });
  os << endl;

  os << "# References" << endl;
  std::ranges::for_each (
      mReferences, [&os] (const string &ref) { os << "- " << ref << endl; });
  os << endl;

  os << "# Links" << endl;
  std::ranges::for_each (
      mLinks, [&os] (const string &link) { os << "- " << link << endl; });
  os << endl;

  return true;
}

bool
Note::dump (std::string_view sv)
{
  string path = string (sv);
  if (path.empty ())
    path = notePathOfFile (mFile);

  auto fspath = filesystem::path (path).parent_path ();
  std::error_code code;
  filesystem::create_directories (fspath, code);

  ofstream ofs{ path };
  if (!ofs.is_open ())
    return false;

  auto ret = dumpStream (ofs);
  ofs.close ();
  return ret;
}

std::string
Note::notePathOfFile (File *file)
{
  auto homedir = QDir::home ().filesystemAbsolutePath ().string ();
  auto filename = file->getFilename ();
  if (filename.find (homedir) == 0)
    filename = filename.substr (homedir.size () + 1);
  if (filename[0] == filesystem::path::preferred_separator)
    filename = filename.substr (1);
  if (filename[1] == ':')
    filename[1] = '-';
  auto path = NotesDir + filesystem::path::preferred_separator + filename;
  return path + ".md";
}
}

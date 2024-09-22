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
#include <algorithm>
#include <filesystem>
#include <functional>
#include <iostream>
#include <optional>
#include <sstream>
#include <stack>
#include <utility>

#include "ApvlvFile.h"
#include "ApvlvUtil.h"
#include "ApvlvWebViewWidget.h"

namespace apvlv
{
const string html_template = "<?xml version='1.0' encoding='UTF-8'?>\n"
                             "<html xmlns=\"http://www.w3.org/1999/xhtml\" "
                             "lang=\"en\" xml:lang=\"en\">\n"
                             "  <head>\n"
                             "    <title></title>\n"
                             "    <meta http-equiv=\"Content-Type\" "
                             "content=\"text/html; charset=utf-8\"/>\n"
                             "  </head>\n"
                             "  <body>\n"
                             "    <div content>"
                             "      <image src=%s />"
                             "    </div>"
                             "  </body>\n"
                             "</html>\n";

using namespace std;
using namespace std::filesystem;

const map<string, vector<string> > &
File::supportMimeTypes ()
{
  return mSupportMimeTypes;
}

vector<string>
File::supportFileExts ()
{
  vector<string> exts;
  for (const auto &pair : mSupportMimeTypes)
    {
      exts.insert (exts.end (), pair.second.begin (), pair.second.end ());
    }
  return exts;
}

map<string, vector<string> > File::mSupportMimeTypes{};
map<string, function<File *()> > File::mSupportClass{};

File::~File ()
{
  mPages.clear ();
  srcPages.clear ();
  srcMimeTypes.clear ();
}

int
File::registerClass (const string &mime, function<File *()> fun,
                     initializer_list<string> exts)
{
  mSupportMimeTypes.insert ({ mime, exts });
  for_each (exts.begin (), exts.end (),
            [fun] (const string &t) { mSupportClass.insert ({ t, fun }); });
  return static_cast<int> (mSupportMimeTypes.size ());
}

File *
File::loadFile (const string &filename, bool check)
{
  File *file;

  auto ext = filename_ext (filename);
  if (ext.empty ())
    return nullptr;

  file = nullptr;
  if (mSupportClass.find (ext) != mSupportClass.end ())
    {
      file = mSupportClass[ext]();
      if (file->load (filename) == false)
        {
          delete file;
          file = nullptr;
        }
    }

  if (file == nullptr)
    {
      qCritical ("open %s error.", filename.c_str ());
    }

  return file;
}

unique_ptr<SearchFileMatch>
File::grepFile (const string &seq, bool is_case, bool is_regex,
                atomic<bool> &is_abort)
{
  vector<SearchPageMatch> page_matches;
  for (int pn = 0; pn < sum (); ++pn)
    {
      if (is_abort.load () == true)
        return nullptr;

      auto size = pageSizeF (pn, 0);
      string content;
      if (pageText (pn, { 0, 0, size.width, size.height }, content) == false)
        continue;

      istringstream iss{ content };
      string line;
      SearchMatchList matches;
      while (getline (iss, line))
        {
          if (is_abort.load () == true)
            return nullptr;

          auto founds = apvlv::grep (line, seq, is_case, is_regex);
          for (auto const &found : founds)
            {
              SearchMatch match{ line.substr (found.first, found.second), line,
                                 found.first, found.second };
              matches.push_back (match);
            }
        }
      if (!matches.empty ())
        {
          page_matches.push_back ({ pn, matches });
        }
    }

  if (page_matches.empty ())
    return nullptr;

  auto file_match = make_unique<SearchFileMatch> ();
  file_match->filename = getFilename ();
  file_match->page_matches = std::move (page_matches);
  return file_match;
}

bool
File::pageRender (int pn, double zm, int rot, WebView *webview)
{
  webview->setZoomFactor (zm);
  QUrl pdfuri = QString ("apvlv:///%1-%2-%3-%4.html")
                    .arg (pn)
                    .arg (zm)
                    .arg (rot)
                    .arg (rand ());
  webview->load (pdfuri);
  return true;
}

string
File::pathMimeType (const string &path)
{
  if (srcMimeTypes.find (path) != srcMimeTypes.end ())
    return srcMimeTypes[path];
  else if (QString::fromLocal8Bit (path).endsWith (".png"))
    return "image/png";
  else
    return "text/html";
}

int
File::pathPageNumber (const string &path)
{
  if (srcPages.find (path) != srcPages.end ())
    return srcPages[path];
  return -1;
}

optional<QByteArray>
File::pathContent (const string &path)
{
  auto words = QString::fromLocal8Bit (path).split ("-");
  int pn = words[0].toInt ();
  double zm = words[1].toDouble ();
  int rot = words[2].toInt ();

  if (QString::fromLocal8Bit (path).endsWith (".html"))
    return pathContentHtml (pn, zm, rot);
  else
    return pathContentPng (pn, zm, rot);
}

optional<QByteArray>
File::pathContentHtml (int pn, double zm, int rot)
{
  auto src = QString ("apvlv:///%1-%2-%3-%4.png")
                 .arg (pn)
                 .arg (zm)
                 .arg (rot)
                 .arg (rand ());
  auto html = QString::asprintf (html_template.c_str (),
                                 src.toStdString ().c_str ());
  return QByteArray::fromStdString (html.toStdString ());
}

optional<QByteArray>
File::pathContentPng (int pn, double zm, int rot)
{
  QImage image;
  if (pageRender (pn, zm, rot, &image) == false)
    return nullopt;

  QByteArray array;
  QBuffer buffer (&array);
  buffer.open (QIODevice::WriteOnly);
  image.save (&buffer, "PNG");
  buffer.close ();
  return array;
}

void
FileIndex::loadDirectory (const string &path1)
{
  auto exts = File::supportFileExts ();

  try
    {
      for (auto &entry : directory_iterator (
               path1, directory_options::follow_directory_symlink))
        {
          if (entry.is_directory ())
            {
              auto index = FileIndex (entry.path ().filename ().string (), 0,
                                      entry.path ().string (), FILE_INDEX_DIR);
              index.loadDirectory (entry.path ().string ());
              if (!index.mChildrenIndex.empty ())
                {
                  mChildrenIndex.emplace_back (index);
                }
            }
          else if (entry.file_size () > 0)
            {
              auto file_ext = filename_ext (entry.path ().string ());
              if (find (exts.cbegin (), exts.cend (), file_ext)
                  != exts.cend ())
                {
                  auto index
                      = FileIndex (entry.path ().filename ().string (), 0,
                                   entry.path ().string (), FILE_INDEX_FILE);
                  mChildrenIndex.emplace_back (index);
                }
            }
        }
    }
  catch (filesystem_error &err)
    {
      qWarning ("file system error: %s", err.what ());
    }

  sort (mChildrenIndex.begin (), mChildrenIndex.end (),
        [] (const FileIndex &a, const FileIndex &b) {
          return a.title < b.title;
        });
}

void
FileIndex::appendChild (const FileIndex &child_index)
{
  Q_ASSERT (type == FILE_INDEX_FILE);
  Q_ASSERT (child_index.type == FILE_INDEX_FILE);
  // Q_ASSERT (path == child_index.path);
  mChildrenIndex = child_index.mChildrenIndex;
}

const FileIndex *
FileIndex::findIndex (const FileIndex &tmp_index) const
{
  stack<const FileIndex *> indexes;
  indexes.push (this);
  while (!indexes.empty ())
    {
      auto top_index = indexes.top ();
      indexes.pop ();
      if (*top_index == tmp_index)
        return top_index;
      for (auto &child : top_index->mChildrenIndex)
        {
          indexes.push (&child);
        }
    }
  return nullptr;
}

bool
FileIndex::operator== (const FileIndex &tmp_index) const
{
  return title == tmp_index.title && page == tmp_index.page
         && path == tmp_index.path && anchor == tmp_index.anchor
         && type == tmp_index.type;
}

FileIndex::FileIndex (const string &title, int page, const string &path,
                      FileIndexType type)
{
  this->title = title;
  this->page = page;
  this->path = path;
  this->type = type;
}

FileIndex::FileIndex (string &&title, int page, string &&path,
                      FileIndexType type)
{
  this->title = std::move (title);
  this->page = page;
  this->path = std::move (path);
  this->type = type;
}

FileIndex::~FileIndex () {}
}

// Local Variables:
// mode: c++
// End:

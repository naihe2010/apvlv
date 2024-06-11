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
/* @CPPFILE ApvlvFile.cc
 *
 *  Author: Alf <naihe2010@126.com>
 */
/* @date Created: 2009/11/20 19:38:30 Alf*/

#include <algorithm>
#include <filesystem>
#include <functional>
#include <iostream>
#include <optional>
#include <utility>

#ifdef APVLV_WITH_POPPLER
#include "ApvlvPopplerPdf.h"
#else
#include "ApvlvQtPdf.h"
#endif
#include "ApvlvEpub.h"
#include "ApvlvFile.h"
#include "ApvlvHtm.h"
#include "ApvlvUtil.h"
#ifdef APVLV_WITH_DJVU
#include "ApvlvDjvu.h"
#endif
#include "ApvlvFb2.h"
#include "ApvlvTxt.h"

namespace apvlv
{
#ifndef MAX
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif

using namespace std;
using namespace std::filesystem;

const map<string, vector<string> > &
ApvlvFile::supportMimeTypes ()
{
  return mSupportMimeTypes;
}

const map<string, vector<string> > ApvlvFile::mSupportMimeTypes = {
#ifdef APVLV_WITH_POPPLER
  { "PDF File", { "*.pdf", "*.PDF" } },
#endif
  { "HTML File", { ".HTM", ".htm", "*.HTML", ".html" } },
  { "ePub File", { "*.EPUB", "*.epub" } },
#ifdef APVLV_WITH_DJVU
  { "DJVU File", { "*.DJV", "*.djv", "*.DJVU", "*.djvu" } },
#endif
  { "TXT File", { "*.TXT", "*.txt" } },
  { "FB2 File", { "*.FB2", "*.fb2" } },
};

const vector<string> &
ApvlvFile::supportFileExts ()
{
  return mSupportFileExts;
}

const vector<string> ApvlvFile::mSupportFileExts = {
#ifdef APVLV_WITH_POPPLER
  ".pdf",
#endif
  ".html", ".htm", ".epub", ".djv", ".djvu", ".txt", ".fb2"
};

ApvlvFile::ApvlvFile (const string &filename, bool check) {}

ApvlvFile::~ApvlvFile ()
{
  mPages.clear ();
  srcPages.clear ();
  srcMimeTypes.clear ();
}

ApvlvFile *
ApvlvFile::newFile (const string &filename, bool check)
{
  ApvlvFile *file;
  map<string, function<ApvlvFile *()> > type_class;

  type_class[".pdf"]
      = [filename] () -> ApvlvFile * { return new ApvlvPDF (filename); };
  type_class[".html"]
      = [filename] () -> ApvlvFile * { return new ApvlvHTML (filename); };
  type_class[".htm"]
      = [filename] () -> ApvlvFile * { return new ApvlvHTML (filename); };
  type_class[".epub"]
      = [filename] () -> ApvlvFile * { return new ApvlvEPUB (filename); };
#ifdef APVLV_WITH_DJVU
  type_class[".djv"]
      = [filename] () -> ApvlvFile * { return new ApvlvDJVU (filename); };
  type_class[".djvu"]
      = [filename] () -> ApvlvFile * { return new ApvlvDJVU (filename); };
#endif
  type_class[".txt"]
      = [filename] () -> ApvlvFile * { return new ApvlvTXT (filename); };
  type_class[".fb2"]
      = [filename] () -> ApvlvFile * { return new ApvlvFB2 (filename); };

  auto ext = filename_ext (filename);
  if (ext.empty ())
    return nullptr;

  if (type_class.find (ext) == type_class.end ())
    return nullptr;

  file = nullptr;
  try
    {
      file = type_class[ext]();
    }

  catch (const bad_alloc &e)
    {
      delete file;
      file = nullptr;
    }

  return file;
}

bool
ApvlvFile::render (int pn, int ix, int iy, double zm, int rot, QImage *pix)
{
  return false;
}

bool
ApvlvFile::render (int pn, int, int, double, int, QWebEngineView *)
{
  return false;
}

optional<QByteArray>
ApvlvFile::get_ocf_file (const string &path)
{
  return nullopt;
}

string
ApvlvFile::get_ocf_mime_type (const string &path)
{
  if (srcMimeTypes.find (path) != srcMimeTypes.end ())
    return srcMimeTypes[path];
  return "text/html";
}

int
ApvlvFile::get_ocf_page (const string &path)
{
  if (srcPages.find (path) != srcPages.end ())
    return srcPages[path];
  return -1;
}

DISPLAY_TYPE
ApvlvFile::get_display_type () { return DISPLAY_TYPE_IMAGE; }

bool
ApvlvFile::hasByteArray (const string &key)
{
  return mCacheByteArray.contains (key);
}

optional<const QByteArray>
ApvlvFile::getByteArray (const string &key)
{
  if (mCacheByteArray.contains (key))
    return mCacheByteArray[key];
  else
    return nullopt;
}

void
ApvlvFile::cacheByteArray (const string &key, const QByteArray &array)
{
  mCacheByteArray.insert (key, array);
}

bool
ApvlvFile::pageselectsearch (int pn, int ix, int iy, double zm, int rot,
                             QImage *pix, ApvlvPoses *poses)
{
  for (auto const &pos : *poses)
    {
      for (int w = pos.p1x * zm; w < pos.p2x * zm; ++w)
        {
          for (int h = pos.p2y * zm; h < pos.p1y * zm; ++h)
            {
              QColor rgb = pix->pixel (w, h);
              rgb.setRgb (255 - rgb.red (), 255 - rgb.green (),
                          255 - rgb.blue ());
              pix->setPixel (w, h, rgb.rgba ());
            }
        }
    }

  return true;
}

bool
ApvlvFile::annot_underline (int, double, double, double, double)
{
  return false;
}

bool
ApvlvFile::annot_text (int, double, double, double, double, const char *text)
{
  return false;
}

ApvlvAnnotTexts
ApvlvFile::getAnnotTexts (int pn)
{
  ApvlvAnnotTexts texts;
  return texts;
}
bool
ApvlvFile::annot_update (int, ApvlvAnnotText *text)
{
  return false;
}

void
ApvlvFileIndex::load_dir (const string &path1)
{
  for (auto &entry :
       directory_iterator (path1, directory_options::follow_directory_symlink))
    {
      if (entry.is_directory ())
        {
          auto index
              = ApvlvFileIndex (entry.path ().string (), 0,
                                entry.path ().string (), FILE_INDEX_DIR);
          index.load_dir (entry.path ().string ());
          mChildrenIndex.emplace_back (index);
        }
      else if (entry.file_size () > 0)
        {
          auto file_ext = filename_ext (entry.path ().string ());
          auto exts = ApvlvFile::supportFileExts ();
          if (count (exts.rbegin (), exts.rend (), file_ext) > 0)
            {
              auto index
                  = ApvlvFileIndex (entry.path ().string (), 0,
                                    entry.path ().string (), FILE_INDEX_FILE);
              mChildrenIndex.emplace_back (index);
            }
        }
    }
}

ApvlvFileIndex::ApvlvFileIndex (string title, int page, string path,
                                ApvlvFileIndexType type)
{
  this->title = std::move (title);
  this->page = page;
  this->path = std::move (path);
  this->type = type;
}

ApvlvFileIndex::~ApvlvFileIndex () {}
}

// Local Variables:
// mode: c++
// End:

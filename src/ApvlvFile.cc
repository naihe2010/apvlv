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

#include "ApvlvFile.h"
#include "ApvlvEpub.h"
#include "ApvlvHtm.h"
#include "ApvlvPdf.h"
#include "ApvlvUtil.h"

#ifdef APVLV_WITH_DJVU
#include "ApvlvDjvu.h"
#endif
#include "ApvlvFb2.h"
#include "ApvlvTxt.h"

#include <algorithm>
#include <functional>
#include <glib.h>
#include <iostream>
#include <sys/stat.h>
#include <utility>

namespace apvlv
{
#ifndef MAX
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif

const map<string, vector<string> > &
ApvlvFile::supportMimeTypes ()
{
  return mSupportMimeTypes;
}

const map<string, vector<string> > ApvlvFile::mSupportMimeTypes = {
  { "PDF File", { "*.pdf", "*.PDF" } },
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

const vector<string> ApvlvFile::mSupportFileExts
    = { ".pdf", ".html", ".htm", ".epub", ".djv", ".djvu", ".txt", ".fb2" };

ApvlvFile::ApvlvFile (__attribute__ ((unused)) const char *filename,
                      __attribute__ ((unused)) bool check)
{
  mRawdata = nullptr;
  mRawdataSize = 0;
}

ApvlvFile::~ApvlvFile ()
{
  delete[] mRawdata;
  mRawdata = nullptr;

  mPages.clear ();
  srcPages.clear ();
  srcMimeTypes.clear ();
}

ApvlvFile *
ApvlvFile::newFile (const char *filename, __attribute__ ((unused)) bool check)
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
ApvlvFile::render (int pn, int ix, int iy, double zm, int rot, GdkPixbuf *pix,
                   char *buffer)
{
  return false;
}

bool
ApvlvFile::renderweb (int pn, int, int, double, int, GtkWidget *widget)
{
  return false;
}

gchar *
ApvlvFile::get_ocf_file (const gchar *path, gssize *sizep)
{
  return nullptr;
}

const gchar *
ApvlvFile::get_ocf_mime_type (const gchar *path)
{
  if (srcMimeTypes.find (path) != srcMimeTypes.end ())
    return srcMimeTypes[path].c_str ();
  return "text/html";
}

int
ApvlvFile::get_ocf_page (const gchar *path)
{
  if (srcPages.find (path) != srcPages.end ())
    return srcPages[path];
  return -1;
}

DISPLAY_TYPE
ApvlvFile::get_display_type () { return DISPLAY_TYPE_IMAGE; }

bool
ApvlvFile::annot_underline (int, gdouble, gdouble, gdouble, gdouble)
{
  return false;
}

bool
ApvlvFile::annot_text (int, gdouble, gdouble, gdouble, gdouble,
                       const char *text)
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
ApvlvFileIndex::load_dir (const gchar *path)
{
  GDir *dir = g_dir_open (path, 0, nullptr);
  if (dir != nullptr)
    {
      const gchar *name;
      while ((name = g_dir_read_name (dir)) != nullptr)
        {
          if (strcmp (name, ".") == 0)
            {
              debug ("avoid hidden file: %s", name);
              continue;
            }

          gchar *realname = g_strjoin (PATH_SEP_S, path, name, nullptr);
          struct stat buf[1];
          char *wrealname
              = g_locale_from_utf8 (realname, -1, nullptr, nullptr, nullptr);
          if (wrealname == nullptr)
            {
              g_free (realname);
              continue;
            }

          int ret = stat (wrealname, buf);
          g_free (wrealname);

          if (ret < 0)
            {
              g_free (realname);
              continue;
            }

          auto file_ext = filename_ext (name);

          if (S_ISDIR (buf->st_mode))
            {
              auto index = ApvlvFileIndex (name, 0, realname, FILE_INDEX_DIR);
              index.load_dir (realname);
              children.emplace_back (index);
            }
          else if (!file_ext.empty ())
            {
              auto exts = ApvlvFile::supportFileExts ();
              for (auto ext = exts.rbegin (); ext != exts.rend (); ++ext)
                {
                  if (*ext == file_ext)
                    {
                      auto index = ApvlvFileIndex (name, 0, realname,
                                                   FILE_INDEX_FILE);
                      children.emplace_back (index);
                    }
                }
            }

          g_free (realname);
        }
    }
  g_dir_close (dir);
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

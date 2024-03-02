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

#include <glib/poppler.h>
#include <gtk/gtk.h>

#include <iostream>
#include <map>
#include <vector>
using namespace std;

namespace apvlv
{
//
// link to a url, or a page num
//
struct ApvlvLink
{
  int mPage;
};

typedef vector<ApvlvLink> ApvlvLinks;

struct ApvlvPoint
{
  double x, y;
};

//
// position of a search result, or just a area
//
struct ApvlvPos
{
  double x1, x2, y1, y2;
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
  ApvlvFileIndex (string title, int page, string path,
                  ApvlvFileIndexType type);
  ~ApvlvFileIndex ();

  static ApvlvFileIndex *newDirIndex (const gchar *path,
                                      ApvlvFileIndex *parent_index = nullptr);

  string title;
  int page;
  string path;
  string anchor;
  ApvlvFileIndexType type;
  vector<ApvlvFileIndex *> children;
};

class ApvlvFile
{
public:
  ApvlvFile (__attribute__ ((unused)) const char *filename,
             __attribute__ ((unused)) bool check);

  virtual ~ApvlvFile ();

  static ApvlvFile *newFile (const char *filename,
                             __attribute__ ((unused)) bool check = false);

  virtual bool writefile (const char *filename) = 0;

  virtual bool pagesize (int page, int rot, double *x, double *y) = 0;

  virtual int pagesum () = 0;

  virtual bool pagetext (int, gdouble, gdouble, gdouble, gdouble, char **) = 0;

  virtual bool render (int, int, int, double, int, GdkPixbuf *, char *buffer);

  virtual bool annot_underline (int, gdouble, gdouble, gdouble, gdouble);

  virtual bool annot_text (int, gdouble, gdouble, gdouble, gdouble,
                           const char *text);

  virtual bool annot_update (int, ApvlvAnnotText *text);

  virtual bool renderweb (int pn, int, int, double, int, GtkWidget *widget);

  virtual ApvlvPoses *pagesearch (int pn, const char *str, bool reverse) = 0;

  virtual bool pageselectsearch (int, int, int, double, int, GdkPixbuf *,
                                 char *, int, ApvlvPoses *)
      = 0;

  virtual ApvlvAnnotTexts getAnnotTexts (int pn);

  virtual ApvlvLinks *getlinks (int pn) = 0;

  virtual ApvlvFileIndex *new_index () = 0;

  virtual void free_index (ApvlvFileIndex *) = 0;

  virtual bool pageprint (int pn, cairo_t *cr) = 0;

  virtual gchar *get_ocf_file (const gchar *path, gssize *);

  virtual const gchar *get_ocf_mime_type (const gchar *path);

  virtual int get_ocf_page (const gchar *path);

protected:
  ApvlvFileIndex *mIndex;

  gchar *mRawdata;
  guint mRawdataSize;
  std::vector<string> mPages;
  std::map<string, int> srcPages;
  std::map<string, string> srcMimeTypes;
};

};

#endif

/* Local Variables: */
/* mode: c++ */
/* End: */

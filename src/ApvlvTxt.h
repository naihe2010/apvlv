/*
* This file is part of the apvlv package
* Copyright (C) <2010>  <Alf>
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
/* @CFILE ApvlvTxt.h
*
*  Author: Alf <naihe2010@126.com>
*/
/* @date Created: 2012/01/16 11:05:10 Alf*/

#ifndef _APVLV_TXT_H_
#define _APVLV_TXT_H_

#include "ApvlvFile.h"

namespace apvlv
{
class ApvlvTxt;
class ApvlvTxtPage
{
public:
  ApvlvTxtPage (int pn, const gchar *, gsize);
  ~ApvlvTxtPage ();

  bool pagesize (int rot, double *x, double *y);

  bool render (int, int, double, int, GdkPixbuf *, char *);

private:
  int mId;
  GString *mContent;

  guchar *mRenderBuf;

  gdouble mZoomrate;

  gint mWidth, mHeight;

  guint mLayoutWidth, mLayoutHeight;
  guint mStride;

  gboolean self_render (int);
};

class ApvlvTXT:public ApvlvFile
{
public:
  ApvlvTXT (const char *filename, bool check = true);

  ~ApvlvTXT ();

  bool writefile (const char *filename);

  bool pagesize (int page, int rot, double *x, double *y);

  int pagesum ();

  bool pagetext (int, int, int, int, int, char **);

  bool render (int, int, int, double, int, GdkPixbuf *, char *);

  bool pageselectsearch (int, int, int, double, int, GdkPixbuf *,
                         char *, int, ApvlvPoses *);

  ApvlvPoses *pagesearch (int pn, const char *s, bool reverse = false);

  ApvlvLinks *getlinks (int pn);

  ApvlvFileIndex *new_index ();

  void free_index (ApvlvFileIndex *);

  bool pageprint (int pn, cairo_t * cr);

private:
  GString *mContent;
  gsize mLength;

  gint mPageCount;
  GPtrArray *mPages;

  gboolean load_pages ();
};

}
#endif

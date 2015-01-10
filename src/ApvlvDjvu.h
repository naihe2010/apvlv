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
/* @CPPFILE ApvlvDjvu.h
 *
 *  Author: Alf <naihe2010@126.com>
 */
/* @date Created: 2011/09/16 13:49:22 Alf*/

#ifndef _APVLV_DJVU_H_
#define _APVLV_DJVU_H_

#include "ApvlvFile.h"

#include <libdjvu/ddjvuapi.h>

namespace apvlv
{
  class ApvlvDJVU:public ApvlvFile
  {
  public:
    ApvlvDJVU (const char *filename, bool check = true);

    ~ApvlvDJVU ();

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
    ddjvu_context_t * mContext;
    ddjvu_document_t *mDoc;
  };
}

#endif

/* Local Variables: */
/* mode: c++ */
/* End: */

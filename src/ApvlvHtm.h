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
/* @CPPFILE ApvlvHtm.h
 *
 *  Author: Alf <naihe2010@126.com>
 */
/* @date Created: 2011/09/16 13:50:49 Alf*/

#ifndef _APVLV_HTM_H_
#define _APVLV_HTM_H_

#include "ApvlvFile.h"

#include <gtk/gtk.h>
#include <webkit2/webkit2.h>

namespace apvlv
{
  const double HTML_DEFAULT_WIDTH = 800;
  const double HTML_DEFAULT_HEIGHT = 800;
  
  class ApvlvHTML:public ApvlvFile
  {
  public:
    ApvlvHTML (const char *, bool check=true);

    ~ApvlvHTML ();

    bool writefile (const char *filename);

    bool pagesize (int page, int rot, double *x, double *y);

    int pagesum ();

    bool pagetext (int, int, int, int, int, char **);

    bool renderweb (int pn, int ix, int iy, double zm, int rot, GtkWidget *widget);

    ApvlvPoses *pagesearch (int pn, const char *str, bool reverse = false);

    bool pageselectsearch (int, int, int, double, int,
                           GdkPixbuf *, char *, int, ApvlvPoses *);

    ApvlvLinks *getlinks (int pn);

    ApvlvFileIndex *new_index ();

    void free_index (ApvlvFileIndex *);

    bool pageprint (int pn, cairo_t * cr);

  private:
    string mUri;
  };

}

#endif

/* Local Variables: */
/* mode: c++ */
/* End: */

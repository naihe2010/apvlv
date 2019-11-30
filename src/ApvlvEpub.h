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
/* @CPPFILE ApvlvEpub.h
 *
 *  Author: Alf <naihe2010@126.com>
 */
/* @date Created: 2018/04/19 13:50:49 Alf*/

#ifndef _APVLV_EPUB_H_
#define _APVLV_EPUB_H_

#include "ApvlvFile.h"

#include <epub.h>
#include <gtk/gtk.h>
#include <libxml/parser.h>

#include <map>

namespace apvlv
{
  class ApvlvEPUB:public ApvlvFile
  {
  public:
    ApvlvEPUB(const char *, bool check=true);
    ~ApvlvEPUB();

    bool writefile (const char *filename);

    bool pagesize (int page, int rot, double *x, double *y);

    int pagesum ();

    bool pagetext (int, int, int, int, int, char **);

    bool renderweb (int pn, int ix, int iy, double zm, int rot, GtkWidget *widget);

    ApvlvPoses *pagesearch (int pn, const char *str, bool reverse =
			    false);

    bool pageselectsearch (int, int, int, double, int,
                           GdkPixbuf *, char *, int, ApvlvPoses *);

    ApvlvLinks *getlinks (int pn);

    ApvlvFileIndex *new_index ();

    void free_index (ApvlvFileIndex *);

    bool pageprint (int pn, cairo_t * cr);

  private:
    string container_get_contentfile (const char *container, int len);

    std::map <string, string> content_get_media (struct epub *epub, string contentfile);

    ApvlvFileIndex * ncx_get_index (struct epub* epub, string ncxfile);

    ApvlvFileIndex ncx_node_get_index (xmlNodePtr node, string ncxfile);

    bool generate_pages ();
    
    std::map <int, string> mPages;
    gchar *mRoot;
  };

}

#endif

/* Local Variables: */
/* mode: c++ */
/* End: */

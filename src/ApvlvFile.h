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

#include <gtk/gtk.h>
#include <glib/poppler.h>

#include <iostream>
#include <vector>
using namespace std;

namespace apvlv
{
  //
  // link to a url, or a page num
  //
  struct ApvlvLink
  {
    string mUrl;
    int mPage;
  };

  typedef vector < ApvlvLink > ApvlvLinks;

  //
  // position of a search result, or just a area
  //
  struct ApvlvPos
  {
    double x1, x2, y1, y2;
  };

  typedef vector < ApvlvPos > ApvlvPoses;

  struct ApvlvFileIndex
  {
    string title;
    int page;
    vector < ApvlvFileIndex > children;
  };

  typedef vector < ApvlvFileIndex >::iterator ApvlvFileIndexIter;

  class ApvlvFile
  {
  public:
    ApvlvFile (const char *filename, bool check);

    virtual ~ ApvlvFile ();

    static ApvlvFile *newfile (const char *filename, bool check = false);

    virtual bool writefile (const char *filename) = 0;

    virtual bool pagesize (int page, int rot, double *x, double *y) = 0;

    virtual int pagesum () = 0;

    virtual bool pagetext (int, int, int, int, int, char **) = 0;

    virtual bool render (int, int, int, double, int, GdkPixbuf *,
			 char *buffer = NULL);

    virtual bool renderweb (int pn, int, int, double, int, GtkWidget *widget);

    virtual ApvlvPoses *pagesearch (int pn, const char *str, bool reverse =
                                    false) = 0;

    virtual bool pageselectsearch (int, int, int, double, int,
				   GdkPixbuf *, char *, int, ApvlvPoses *) =
      0;

    virtual ApvlvLinks *getlinks (int pn) = 0;

    virtual ApvlvFileIndex *new_index () = 0;

    virtual void free_index (ApvlvFileIndex *) = 0;

    virtual bool pageprint (int pn, cairo_t * cr) = 0;

    string get_anchor ();

  protected:

    ApvlvFileIndex * mIndex;
    unsigned short mIndexRef;

    string mAnchor;

    gchar *mRawdata;
    guint mRawdataSize;
  };

};

#endif

/* Local Variables: */
/* mode: c++ */
/* End: */

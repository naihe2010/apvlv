/*
* This file is part of the apvlv package
* Copyright (C) <2008>  <Alf>
*
* Contact: Alf <naihe2010@gmail.com>
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
/* @CFILE ApvlvFile.hpp xxxxxxxxxxxxxxxxxxxxxxxxxx.
*
*  Author: Alf <naihe2010@gmail.com>
*/
/* @date Created: 2009/11/20 19:37:44 Alf*/

#ifndef _APVLV_FILE_H_
#define _APVLV_FILE_H_

#ifdef HAVE_CONFIG_H
# include "config.hpp"
#endif

#include <gtk/gtk.h>
#include <glib/poppler.h>
#ifdef HAVE_LIBDJVU
# include <libdjvu/ddjvuapi.h>
#endif

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

    virtual bool pagetext (int, char **) = 0;

    virtual bool render (int, int, int, double, int, GdkPixbuf *,
			 char *buffer = NULL) = 0;

    virtual ApvlvPoses *pagesearch (int pn, const char *str, bool reverse =
				    false) = 0;

    virtual bool pageselectsearch (int, int, int, double, int,
				   GdkPixbuf *, char *, int, ApvlvPoses *) =
      0;

    virtual ApvlvLinks *getlinks (int pn) = 0;

    virtual ApvlvFileIndex *new_index () = 0;

    virtual void free_index (ApvlvFileIndex *) = 0;

    virtual bool pageprint (int pn, cairo_t * cr) = 0;

  protected:

      ApvlvFileIndex * mIndex;
    unsigned short mIndexRef;
  };

  class ApvlvPDF:public ApvlvFile
  {
  public:
    ApvlvPDF (const char *filename, bool check = true);

     ~ApvlvPDF ();

    bool writefile (const char *filename);

    bool pagesize (int page, int rot, double *x, double *y);

    int pagesum ();

    bool pagetext (int, char **);

    bool render (int, int, int, double, int, GdkPixbuf *, char *);

    bool pageselectsearch (int, int, int, double, int, GdkPixbuf *,
			   char *, int, ApvlvPoses *);

    ApvlvPoses *pagesearch (int pn, const char *s, bool reverse = false);

    ApvlvLinks *getlinks (int pn);

    ApvlvFileIndex *new_index ();

    void free_index (ApvlvFileIndex *);

    bool pageprint (int pn, cairo_t * cr);

  private:
      bool walk_poppler_index_iter (ApvlvFileIndex * titr,
				    PopplerIndexIter * iter);

    PopplerDocument *mDoc;
  };

  class ApvlvDJVU:public ApvlvFile
  {
  public:
    ApvlvDJVU (const char *filename, bool check = true);

     ~ApvlvDJVU ();

    bool writefile (const char *filename);

    bool pagesize (int page, int rot, double *x, double *y);

    int pagesum ();

    bool pagetext (int, char **);

    bool render (int, int, int, double, int, GdkPixbuf *, char *);

    bool pageselectsearch (int, int, int, double, int, GdkPixbuf *,
			   char *, int, ApvlvPoses *);

    ApvlvPoses *pagesearch (int pn, const char *s, bool reverse = false);

    ApvlvLinks *getlinks (int pn);

    ApvlvFileIndex *new_index ();

    void free_index (ApvlvFileIndex *);

    bool pageprint (int pn, cairo_t * cr);

  private:
#ifdef HAVE_LIBDJVU
      ddjvu_context_t * mContext;
    ddjvu_document_t *mDoc;
#endif
  };

};

#endif

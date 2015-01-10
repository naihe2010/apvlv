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
/* @CPPFILE ApvlvUmd.cc
 *
 *  Author: Alf <naihe2010@126.com>
 */
/* @date Created: 2011/09/16 13:56:35 Alf*/

#include "ApvlvUtil.h"
#include "ApvlvUmd.h"

#define LIBUMD_ENABLE_GTK
#include <umd.h>

namespace apvlv
{
  ApvlvUMD::ApvlvUMD (const char *filename, bool check):ApvlvFile (filename,
								   check)
  {
    gchar * lname = g_locale_from_utf8 (filename, -1, NULL, NULL, NULL);
    if (lname)
      {
	mUmd = umd_new_from_file (lname);
	g_free (lname);
      }
    else
      {
	mUmd = umd_new_from_file (filename);
      }

    if (mUmd == NULL)
      {
	throw std::bad_alloc ();
      }
  }

  ApvlvUMD::~ApvlvUMD ()
  {
    if (mUmd)
      {
	umd_destroy (mUmd);
      }
  }

  bool ApvlvUMD::writefile (const char *filename)
  {
    if (umd_write_file (mUmd, filename) == 0)
      {
	return true;
      }
    return false;
  }

  bool ApvlvUMD::pagesize (int pn, int rot, double *x, double *y)
  {
    umd_page_t * page;

    page = umd_get_nth_page (mUmd, pn);
    if (page)
      {
	int ix, iy;
	umd_page_get_size (page, &ix, &iy);
	*x = ix;
	*y = iy;
	return true;
      }
    return false;
  }

  int ApvlvUMD::pagesum ()
  {
    return mUmd ? umd_get_page_n (mUmd) : 0;
  }

  bool ApvlvUMD::render (int pn, int ix, int iy, double zm, int rot,
			 GdkPixbuf * pix, char *buffer)
  {
    umd_page_t * page;

    page = umd_get_nth_page (mUmd, pn);
    if (page)
      {
	umd_page_render (page, 0, 0, ix, iy, zm, rot, (unsigned char *) buffer, 3 * ix);
	return true;
      }
    return false;
  }

  bool ApvlvUMD::pageselectsearch (int pn, int ix, int iy, double zm,
				   int rot, GdkPixbuf * pix, char *buffer,
				   int sel, ApvlvPoses * poses)
  {
    return false;
  }

  ApvlvPoses *ApvlvUMD::pagesearch (int pn, const char *str, bool reverse)
  {
    return NULL;
  }

  ApvlvLinks *ApvlvUMD::getlinks (int pn)
  {
    return NULL;
  }

  bool ApvlvUMD::pagetext (int pn, int x1, int y1, int x2, int y2,
			   char **out)
  {
    return false;
  }

  ApvlvFileIndex *ApvlvUMD::new_index ()
  {
    if (mIndex != NULL)
      {
	debug ("file %p has index: %p, return", this, mIndex);
	return mIndex;
      }

    int cpt_n = umd_get_chapter_n (mUmd);
    if (cpt_n <= 0)
      {
	debug ("no index.");
	return NULL;
      }

    mIndex = new ApvlvFileIndex;
    for (int i = 0;
	 i < cpt_n;
	 ++ i)
      {
	umd_chapter_t * chapter = umd_get_nth_chapter (mUmd, i);
	if (chapter)
	  {
	    ApvlvFileIndex inx;

	    inx.title = chapter->title;
	    inx.page = chapter->page_index;

	    mIndex->children.push_back (inx);

	    free (chapter->title);
	    free (chapter->content);
	    free (chapter);
	  }
      }

    return mIndex;
  }

  void ApvlvUMD::free_index (ApvlvFileIndex * index)
  {
    delete mIndex;
  }

  bool ApvlvUMD::pageprint (int pn, cairo_t * cr)
  {
    return false;
  }
}

// Local Variables:
// mode: c++
// End:

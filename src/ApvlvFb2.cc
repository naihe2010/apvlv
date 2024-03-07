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
/* @CPPFILE ApvlvFb2.cc
 *
 *  Author: Alf <naihe2010@126.com>
 */
/* @date Created: 2024/03/07 11:06:35 Alf*/

#include "ApvlvFb2.h"
#include "ApvlvUtil.h"

#include <cmath>
#include <webkit2/webkit2.h>

namespace apvlv
{
ApvlvFB2::ApvlvFB2 (const char *filename, bool check)
    : ApvlvFile (filename, check)
{
  GError *error = nullptr;
  if (g_file_get_contents (filename, &mContent, &mLength, &error) == FALSE)
    {
      if (error != nullptr)
        {
          debug ("get file: %s centents error: %s", filename, error->message);
          g_error_free (error);
        }

      throw bad_alloc ();
    }
}

ApvlvFB2::~ApvlvFB2 () { g_free (mContent); }

bool
ApvlvFB2::writefile (const char *filename)
{
  if (g_file_set_contents (filename, mContent, int (mLength), nullptr) == TRUE)
    {
      return true;
    }
  return false;
}

bool
ApvlvFB2::pagesize (int pn, int rot, double *px, double *py)
{
  return false;
}

int
ApvlvFB2::pagesum ()
{
  return 1;
}

bool
ApvlvFB2::pageselectsearch (int pn, int ix, int iy, double zm, int rot,
                            GdkPixbuf *pix, char *buffer, int sel,
                            ApvlvPoses *poses)
{
  return false;
}

ApvlvPoses *
ApvlvFB2::pagesearch (int pn, const char *str, bool reverse)
{
  return nullptr;
}

ApvlvLinks *
ApvlvFB2::getlinks (int pn)
{
  return nullptr;
}

bool
ApvlvFB2::pagetext (int pn, gdouble x1, gdouble y1, gdouble x2, gdouble y2,
                    char **out)
{
  return false;
}

bool
ApvlvFB2::render (int, int, int, double, int, GdkPixbuf *, char *)
{
  return false;
}

bool
ApvlvFB2::renderweb (int pn, int ix, int iy, double zm, int rot,
                     GtkWidget *widget)
{
  webkit_web_view_set_zoom_level (WEBKIT_WEB_VIEW (widget), zm);
  string uri = "ascii";
  string epuburi = "apvlv:///" + uri;
  webkit_web_view_load_uri (WEBKIT_WEB_VIEW (widget), epuburi.c_str ());
  return true;
}

ApvlvFileIndex *
ApvlvFB2::new_index ()
{
  return nullptr;
}

void
ApvlvFB2::free_index (ApvlvFileIndex *index)
{
}

bool
ApvlvFB2::pageprint (int pn, cairo_t *cr)
{
  return false;
}

gchar *
ApvlvFB2::get_ocf_file (const gchar *path, gssize *lenp)
{
  *lenp = (gssize)mLength;
  return g_strdup (mContent);
}

const gchar *
ApvlvFB2::get_ocf_mime_type (const gchar *path)
{
  return "text/plain";
}

}

// Local Variables:
// mode: c++
// End:

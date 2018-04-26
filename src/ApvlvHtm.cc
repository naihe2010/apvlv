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
/* @CPPFILE ApvlvHtm.cc
 *
 *  Author: Alf <naihe2010@126.com>
 */
/* @date Created: 2011/09/16 13:51:04 Alf*/

#include "ApvlvUtil.h"
#include "ApvlvView.h"
#include "ApvlvHtm.h"

#include <gdk/gdkkeysyms.h>
#include <webkit/webkit.h>

namespace apvlv
{
  ApvlvHTML::ApvlvHTML (const char *filename, bool check):ApvlvFile (filename,
                                                                     check)
  {
    mUri = filename;
    if (g_path_is_absolute(filename)) {
      gchar* fileUri = g_filename_to_uri(filename, NULL, NULL);
      mUri = fileUri;
      g_free(fileUri);
    }
  }

  ApvlvHTML::~ApvlvHTML ()
  {
  }

  bool ApvlvHTML::writefile (const char *filename)
  {
    return false;
  }

  bool ApvlvHTML::pagesize (int page, int rot, double *x, double *y)
  {
    *x = HTML_DEFAULT_WIDTH;
    *y = HTML_DEFAULT_HEIGHT;
    return true;
  }

  int ApvlvHTML::pagesum ()
  {
    return 1;
  }

  bool ApvlvHTML::pagetext (int, int, int, int, int, char **)
  {
    return false;
  }

  bool ApvlvHTML::renderweb (int pn, int ix, int iy, double zm, int rot, GtkWidget *widget)
  {
    webkit_web_view_set_zoom_level (WEBKIT_WEB_VIEW (widget), zm);
    webkit_web_view_load_uri (WEBKIT_WEB_VIEW (widget), mUri.c_str());
    return false;
  }

  ApvlvPoses *ApvlvHTML::pagesearch (int pn, const char *str, bool reverse)
  {
    return NULL;
  }

  bool ApvlvHTML::pageselectsearch (int pn, int ix, int iy,
                                    double zm, int rot, GdkPixbuf *pix,
                                    char *buffer, int sel, ApvlvPoses *poses)
  {
    return false;
  }

  ApvlvLinks *ApvlvHTML::getlinks (int pn)
  {
    return NULL;
  }

  ApvlvFileIndex *ApvlvHTML::new_index ()
  {
    return NULL;
  }

  void ApvlvHTML::free_index (ApvlvFileIndex *index)
  {
    delete index;
  }

  bool ApvlvHTML::pageprint (int pn, cairo_t * cr)
  {
    return false;
  }
}

// Local Variables:
// mode: c++
// End:

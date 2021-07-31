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
/* @CPPFILE ApvlvTxt.cc
 *
 *  Author: Alf <naihe2010@126.com>
 */
/* @date Created: 2012/01/16 11:06:35 Alf*/

#include "ApvlvTxt.h"
#include "ApvlvUtil.h"

#include <cmath>

namespace apvlv
{
#define DOUBLE_EQ(a, b) (fabs ((a) - (b)) < 0.0001)
#define DOUBLE_NE(a, b) (!DOUBLE_EQ (a, b))

const gint PAGE_CHARACTER_COUNT = 10000;
const gdouble PAGE_RENDER_ZOOM = 1.0;
const gint PAGE_RENDER_WIDTH = 200;
const gint PAGE_RENDER_HEIGHT = 1200;

ApvlvTxtPage::ApvlvTxtPage (int id, const gchar *str, gsize size)
{
  mContent = g_string_new_len (str, size);

  mZoomrate = PAGE_RENDER_ZOOM;
  mRenderBuf = nullptr;
  mWidth = mHeight = 0;
}

ApvlvTxtPage::~ApvlvTxtPage ()
{
  if (mContent)
    {
      g_string_free (mContent, TRUE);
    }
  if (mRenderBuf)
    {
      g_free (mRenderBuf);
    }
}

ApvlvTXT::ApvlvTXT (const char *filename, bool check)
    : ApvlvFile (filename, check)
{
  gboolean ok;
  gchar *content;
  const gchar *begin;
  gsize length;

  gchar *lname = g_locale_from_utf8 (filename, -1, nullptr, nullptr, nullptr);
  if (lname)
    {
      ok = g_file_get_contents (lname, &content, &length, nullptr);
      g_free (lname);
    }
  else
    {
      ok = g_file_get_contents (filename, &content, &length, nullptr);
    }

  if (ok == FALSE)
    {
      throw std::bad_alloc ();
    }

  debug ("total chars: %d", length);
  gchar *content2 = nullptr;
  gsize len2;
  if (g_utf8_validate (content, length, &begin) == FALSE && begin == content)
    {
      content2 = g_locale_to_utf8 (content, length, nullptr, &len2, nullptr);
    }

  if (content2)
    {
      mContent = g_string_new_len (content2, len2);
      g_free (content2);
    }
  else
    {
      mContent = g_string_new_len (content, length);
    }
  g_free (content);

  mLength = g_utf8_strlen (mContent->str, mContent->len);
  debug ("total Character: %d", mLength);

  load_pages ();
}

ApvlvTXT::~ApvlvTXT ()
{
  if (mContent)
    {
      g_string_free (mContent, TRUE);
    }

  if (mPages)
    {
      g_ptr_array_free (mPages, TRUE);
    }
}

gboolean
ApvlvTXT::load_pages ()
{
  ApvlvTxtPage *page;
  gchar *b, *e;
  gunichar unic;
  gsize i, len;

  mPages = g_ptr_array_new_with_free_func ([] (void *p) {
    ApvlvTxtPage *page = (ApvlvTxtPage *)p;
    delete page;
  });
  g_return_val_if_fail (mPages, FALSE);

  for (i = 0, mPageCount = 0; i < mLength; i += len, ++mPageCount)
    {
      len = (i + PAGE_CHARACTER_COUNT) > mLength ? mLength - i
                                                 : PAGE_CHARACTER_COUNT;

      b = g_utf8_offset_to_pointer (mContent->str, i);
      e = g_utf8_offset_to_pointer (b, len);
      unic = g_utf8_get_char (e);
      while (g_unichar_isspace (unic) == FALSE && *e != '\0')
        {
          ++len;
          e = g_utf8_next_char (e);
          unic = g_utf8_get_char (e);
        }
      page = new ApvlvTxtPage (mPageCount, b, e - b);
      g_ptr_array_add (mPages, page);
    }
  debug ("total page: %d", mPageCount);

  return TRUE;
}

bool
ApvlvTXT::writefile (const char *filename)
{
  if (g_file_set_contents (filename, mContent->str, mContent->len, nullptr)
      == TRUE)
    {
      return true;
    }
  return false;
}

bool
ApvlvTXT::pagesize (int pn, int rot, double *px, double *py)
{
  ApvlvTxtPage *page;

  page = (ApvlvTxtPage *)g_ptr_array_index (mPages, pn);
  if (page != nullptr)
    {
      return page->pagesize (rot, px, py);
    }
  else
    {
      return false;
    }
}

bool
ApvlvTxtPage::pagesize (int rot, double *px, double *py)
{
  if (DOUBLE_NE (mZoomrate, PAGE_RENDER_ZOOM))
    {
      if (mRenderBuf)
        {
          g_free (mRenderBuf);
          mRenderBuf = nullptr;
        }
      mZoomrate = PAGE_RENDER_ZOOM;
    }

  if (!mRenderBuf)
    {
      self_render (rot);
    }

  if (!mRenderBuf)
    {
      return false;
    }

  if (px)
    {
      *px = mWidth + 2;
    }
  if (py)
    {
      *py = mHeight + 2;
    }

  return true;
}

int
ApvlvTXT::pagesum ()
{
  return mPageCount;
}

bool
ApvlvTxtPage::render (int ix, int iy, double zm, int rot, GdkPixbuf *pix,
                      char *buffer)
{
  guint i, j, passed, fromx, fromy, tox, toy;

  if (DOUBLE_NE (zm, mZoomrate))
    {
      if (mRenderBuf)
        {
          g_free (mRenderBuf);
          mRenderBuf = nullptr;
        }
      mZoomrate = zm;
    }

  if (!mRenderBuf)
    {
      self_render (rot);
    }

  if (!mRenderBuf)
    {
      return false;
    }

  fromx = 0;
  fromy = 0;
  tox = (guint)ix > mLayoutWidth ? mLayoutWidth : ix;
  toy = (guint)iy > mLayoutHeight ? mLayoutHeight : iy;

  for (j = fromy; j <= toy; ++j)
    {
      passed = j * mStride;

      for (i = fromx; i <= tox; ++i)
        {
          memcpy (buffer + i * 3, mRenderBuf + passed + i * 4 + 1, 3);
        }
      buffer += 3 * ix;
    }

  return true;
}

bool
ApvlvTXT::render (int pn, int ix, int iy, double zm, int rot, GdkPixbuf *pix,
                  char *buffer)
{
  ApvlvTxtPage *page;

  page = (ApvlvTxtPage *)g_ptr_array_index (mPages, pn);
  if (page)
    {
      return page->render (ix, iy, zm, rot, pix, buffer);
    }
  else
    {
      return false;
    }
}

gboolean
ApvlvTxtPage::self_render (int rot)
{
  cairo_surface_t *surface;
  cairo_t *cr;
  int wunits, hunits;
  PangoLayout *layout;

  mLayoutWidth = PAGE_RENDER_WIDTH * mZoomrate;
  mLayoutHeight = PAGE_RENDER_HEIGHT * mZoomrate;
  mStride = cairo_format_stride_for_width (CAIRO_FORMAT_RGB24, mLayoutWidth);

  mRenderBuf = (guchar *)g_malloc (mStride * mLayoutHeight);
  g_return_val_if_fail (mRenderBuf, false);

  surface = cairo_image_surface_create_for_data (
      mRenderBuf, CAIRO_FORMAT_RGB24, mLayoutWidth, mLayoutHeight, mStride);
  if (surface == nullptr)
    {
      g_free (mRenderBuf);
      return false;
    }

  cr = cairo_create (surface);
  if (cr == nullptr)
    {
      g_free (mRenderBuf);
      cairo_surface_destroy (surface);
      return false;
    }

  /* init the background */
  cairo_set_source_rgb (cr, 1.0, 1.0, 1.0);
  cairo_rectangle (cr, 0, 0, mLayoutWidth, mLayoutHeight);
  cairo_fill (cr);

  cairo_select_font_face (cr, "Sans", CAIRO_FONT_SLANT_NORMAL,
                          CAIRO_FONT_WEIGHT_NORMAL);
  cairo_set_font_size (cr, 20 * mZoomrate);
  cairo_set_source_rgb (cr, 0.0, 0.0, 0.0);

  layout = pango_cairo_create_layout (cr);

  wunits = pango_units_from_double (mLayoutWidth);
  hunits = pango_units_from_double (mLayoutHeight);
  pango_layout_set_width (layout, wunits);
  pango_layout_set_height (layout, hunits);
  pango_layout_set_wrap (layout, PANGO_WRAP_WORD_CHAR);
  pango_layout_set_spacing (layout, 80);
  pango_layout_set_indent (layout, 160);

  pango_layout_set_text (layout, mContent->str, mContent->len);

  pango_cairo_update_layout (cr, layout);
  pango_cairo_show_layout (cr, layout);

  pango_layout_get_pixel_size (layout, &mWidth, &mHeight);
  debug ("txt page size: %d:%d", mWidth, mHeight);

  cairo_surface_destroy (surface);
  cairo_destroy (cr);

  return true;
}

bool
ApvlvTXT::pageselectsearch (int pn, int ix, int iy, double zm, int rot,
                            GdkPixbuf *pix, char *buffer, int sel,
                            ApvlvPoses *poses)
{
  return false;
}

ApvlvPoses *
ApvlvTXT::pagesearch (int pn, const char *str, bool reverse)
{
  return nullptr;
}

ApvlvLinks *
ApvlvTXT::getlinks (int pn)
{
  return nullptr;
}

bool
ApvlvTXT::pagetext (int pn, gdouble x1, gdouble y1, gdouble x2, gdouble y2,
                    char **out)
{
  return false;
}

ApvlvFileIndex *
ApvlvTXT::new_index ()
{
  return nullptr;
}

void
ApvlvTXT::free_index (ApvlvFileIndex *index)
{
}

bool
ApvlvTXT::pageprint (int pn, cairo_t *cr)
{
  return false;
}
}

// Local Variables:
// mode: c++
// End:

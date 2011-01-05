/*
* This file is part of the libumd package
* Copyright (C) <2010>  <Alf>
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
/* @CFILE umd_page.c
*
*  Author: Alf <naihe2010@gmail.com>
*/
/* @date Created: 2010/12/17 12:56:12 Alf*/

#include "umd.h"
#include "umd_internals.h"

#include <cairo.h>
#include <pango/pangocairo.h>

#include <stdlib.h>
#include <string.h>
#include <math.h>

#ifndef LIBUMD_FONT_FACE
#define LIBUMD_FONT_FACE                "Sans"
#endif
#ifndef LIBUMD_FONT_SIZE
#define LIBUMD_FONT_SIZE                20
#endif

#ifndef LIBUMD_RENDER_PAGE_WIDTH
#define LIBUMD_RENDER_PAGE_WIDTH        900
#endif
#ifndef LIBUMD_RENDER_PAGE_HEIGHT_MAX
#define LIBUMD_RENDER_PAGE_HEIGHT_MAX   0x1000
#endif

#ifndef LIBUMD_RENDER_ZOOM
#define LIBUMD_RENDER_ZOOM              (1.0)
#endif

#define DOUBLE_EQ(a,b)                  (fabs ((a) - (b)) < 0.0001)
#define DOUBLE_NE(a,b)                  (!DOUBLE_EQ(a,b))

struct umd_page_s
{
  umd_t *umd;

  size_t id;

  char *content;
  size_t length;

  double zoomrate;
  int stride;
  int layout_width;
  int layout_height;
  unsigned char *imgbuf;
  int width;
  int height;
};

static int umd_page_render_self (umd_page_t *);

int
umd_get_page_n (umd_t * umd)
{
  return umd->page_count;
}

umd_page_t *
umd_get_nth_page (umd_t * umd, size_t id)
{
  size_t ilen, olen;
  char *ip, *op;
  umd_page_t *page;
  umd_pageinfo_t *pi;

  for (pi = umd->pageinfo_head; pi != NULL; pi = pi->next)
    {
      if (pi->id == id)
	{
	  break;
	}
    }

  page = calloc (1, sizeof (umd_page_t));
  if (page == NULL)
    {
      UMD_ERROR (umd, "Memory error.\n");
      return NULL;
    }

  ilen = pi->length;
  ip = pi->char_begin;
  olen = pi->length << 1;
  op = page->content = malloc (olen);
  if (op == NULL)
    {
      UMD_ERROR (umd, "Memory error.\n");
      free (page);
      return NULL;
    }

  iconv (umd->iconv, &ip, &ilen, &op, &olen);
  *op = '\0';
  page->length = (pi->length << 1) - olen;
  page->umd = umd;
  page->id = id;

  page->zoomrate = LIBUMD_RENDER_ZOOM;

  return page;
}

int
umd_page_get_size (umd_page_t * page, int *px, int *py)
{
  if (DOUBLE_NE (page->zoomrate, LIBUMD_RENDER_ZOOM))
    {
      if (page->imgbuf)
	{
	  free (page->imgbuf);
	  page->imgbuf = NULL;
	}
      page->zoomrate = LIBUMD_RENDER_ZOOM;
    }

  if (!page->imgbuf)
    {
      umd_page_render_self (page);
    }

  if (!page->imgbuf)
    {
      return -1;
    }

  if (px)
    {
      *px = page->width;
    }
  if (py)
    {
      *py = page->height;
    }

  return 0;
}

int
umd_page_render (umd_page_t * page,
		 int fromx, int fromy,
		 int tox, int toy,
		 double zm, int rot, unsigned char *buffer, size_t size)
{
  int i, j, passed;

  if (!DOUBLE_NE (zm, page->zoomrate))
    {
      if (page->imgbuf)
	{
	  free (page->imgbuf);
	  page->imgbuf = NULL;
	}
      page->zoomrate = zm;
    }

  if (!page->imgbuf)
    {
      umd_page_render_self (page);
    }

  if (!page->imgbuf)
    {
      return -1;
    }

  if (fromx < 0)
    {
      fromx = 0;
    }
  if (fromy < 0)
    {
      fromy = 0;
    }
  if (tox > page->layout_width)
    {
      tox = page->layout_width;
    }
  if (toy > page->layout_height)
    {
      toy = page->layout_height;
    }

  for (j = fromy; j <= toy; ++j)
    {
      passed = j * page->stride;

      for (i = fromx; i <= tox; ++i)
	{
	  memcpy (buffer + i * 3, page->imgbuf + passed + i * 4 + 1, 3);
	}
      buffer += size;
    }

  return 0;
}

int
umd_page_get_content (umd_page_t * page, char *buf, size_t size)
{
  size_t len;

  len = page->length;
  if (size < len)
    {
      len = size;
    }

  memcpy (buf, page->content, len);

  return len;
}

void
umd_page_free (umd_page_t * page)
{
  if (page->content)
    {
      free (page->content);
    }
  if (page->imgbuf)
    {
      free (page->imgbuf);
    }
  free (page);
}

static int
umd_page_render_self (umd_page_t * page)
{
  int w, h;
  cairo_surface_t *surface;
  cairo_t *cr;
  PangoLayout *layout;
  umd_t *umd;

  umd = page->umd;
  if (umd == NULL)
    {
      return -1;
    }

  page->layout_width = (int) (LIBUMD_RENDER_PAGE_WIDTH * page->zoomrate);
  page->layout_height = (int) (LIBUMD_RENDER_PAGE_HEIGHT_MAX * page->zoomrate);
  page->stride = page->layout_width * 4;

  page->imgbuf = malloc (page->stride * page->layout_height);
  if (page->imgbuf == NULL)
    {
      return -1;
    }

  surface = cairo_image_surface_create_for_data (page->imgbuf,
						 CAIRO_FORMAT_RGB24,
						 page->layout_width,
						 page->layout_height,
						 page->stride);
  if (surface == NULL)
    {
      return -1;
    }
  cr = cairo_create (surface);
  if (cr == NULL)
    {
      return -1;
    }

  /* init the background */
  cairo_set_source_rgb (cr, 1.0, 1.0, 1.0);
  cairo_rectangle (cr, 0, 0, page->layout_width, page->layout_height);
  cairo_fill (cr);

  cairo_select_font_face (cr,
			  umd->render_font ?
			  umd->render_font : LIBUMD_FONT_FACE,
			  umd->render_italic ?
			  CAIRO_FONT_SLANT_ITALIC : CAIRO_FONT_SLANT_NORMAL,
			  umd->render_bold ?
			  CAIRO_FONT_WEIGHT_BOLD : CAIRO_FONT_WEIGHT_NORMAL);
  cairo_set_font_size (cr, LIBUMD_FONT_SIZE * page->zoomrate);
  cairo_set_source_rgb (cr, 0.0, 0.0, 0.0);

  layout = pango_cairo_create_layout (cr);

  w = pango_units_from_double (page->layout_width);
  h = pango_units_from_double (page->layout_height);
  pango_layout_set_width (layout, w);
  pango_layout_set_height (layout, h);
  pango_layout_set_wrap (layout, PANGO_WRAP_WORD_CHAR);
  pango_layout_set_spacing (layout, 8);

  pango_layout_set_text (layout, page->content, page->length);
  pango_cairo_update_layout (cr, layout);
  pango_cairo_show_layout (cr, layout);

  pango_layout_get_pixel_size (layout, &page->width, &page->height);

  cairo_surface_destroy (surface);
  cairo_destroy (cr);

  return 0;
}

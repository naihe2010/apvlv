/*
 * This file is part of the fdupves package
 * Copyright (C) <2008> Alf
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
/* @CFILE ihash.c
 *
 *  Author: Alf <naihe2010@126.com>
 */

#include "ebook.h"
#include "ApvlvUtil.h"
#include <poppler.h>
#include <string.h>

static void
_poppler_page_render_to_png (PopplerPage *page, int width, int height,
                             const gchar *file)
{
  cairo_t *cr;
  cairo_surface_t *surface;

  surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, width, height);
  cr = cairo_create (surface);
  cairo_save (cr);

  poppler_page_render (page, cr);
  cairo_restore (cr);

  cairo_set_operator (cr, CAIRO_OPERATOR_DEST_OVER);
  cairo_set_source_rgb (cr, 1., 1., 1.);
  cairo_paint (cr);

  cairo_destroy (cr);

  cairo_surface_write_to_png (surface, file);
  cairo_surface_destroy (surface);
}

static void
pdf_get_cover_hash (PopplerDocument *doc, ebook_hash_t *ehash)
{
  PopplerPage *page;
  GError *gerr;
  double width, height;
  int cover_fd;
  gchar *cover;

  page = poppler_document_get_page (doc, 0);
  if (page == NULL)
    {
      return;
    }

  poppler_page_get_size (page, &width, &height);
  gerr = NULL;
  cover_fd = g_file_open_tmp ("apvlv_cover_XXXXXX.png", &cover, &gerr);
  if (cover_fd < 0)
    {
      if (gerr)
        {
          g_warning ("create tempory png file error: %s", gerr->message);
          g_error_free (gerr);
        }

      g_object_unref (page);
      return;
    }

  _poppler_page_render_to_png (page, (int)width, (int)height, cover);
  g_object_unref (page);

  ehash->cover_hash = image_file_hash (cover);
  close (cover_fd);
  unlink (cover);
  g_free (cover);
}

static void
pdf_get_isbn (PopplerDocument *doc, ebook_hash_t *ehash)
{
  gchar *meta;

  meta = poppler_document_get_metadata (doc);
  if (meta && strcasestr (meta, "isbn"))
    {
      do
        {
          if (xml_get_value (meta, "//pdfx:isbn", ehash->isbn,
                             sizeof ehash->isbn)
              != NULL)
            break;

          if (xml_get_value (meta, "//pdfx:ISBN", ehash->isbn,
                             sizeof ehash->isbn)
              != NULL)
            break;
        }
      while (0);
    }

  g_free (meta);
}

int
pdf_hash (const char *file, ebook_hash_t *ehash)
{
  PopplerDocument *doc;
  gchar *uri, *title, *author, *producer;
  GError *gerr = NULL;

  uri = g_filename_to_uri (file, NULL, &gerr);
  if (uri == NULL)
    {
      if (gerr)
        {
          g_warning ("conver path: %s to uri error: %s", file, gerr->message);
          g_error_free (gerr);
        }
      return -1;
    }

  doc = poppler_document_new_from_file (uri, NULL, &gerr);
  g_free (uri);
  if (doc == NULL)
    {
      if (gerr)
        {
          g_warning ("open pdf file error: %s", gerr->message);
          g_error_free (gerr);
        }
      return -1;
    }

  pdf_get_cover_hash (doc, ehash);
  //pdf_get_isbn (doc, ehash);

#define _pdf_get_value_to_ehash(key)                                          \
  do                                                                          \
    {                                                                         \
      key = poppler_document_get_##key (doc);                                 \
      if (key)                                                                \
        {                                                                     \
          snprintf (ehash->key, sizeof ehash->key, "%s", key);                \
          g_free (key);                                                       \
        }                                                                     \
    }                                                                         \
  while (0)

  //_pdf_get_value_to_ehash (title);
  //_pdf_get_value_to_ehash (author);
  //_pdf_get_value_to_ehash (producer);

  g_object_unref (G_OBJECT (doc));
  return 0;
}

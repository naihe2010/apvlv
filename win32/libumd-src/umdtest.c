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
/* @CFILE umdtest.c
*
*  Author: Alf <naihe2010@gmail.com>
*/
/* @date Created: 2010/12/06 14:05:54 Alf*/

#include <stdlib.h>
#ifdef ENABLE_GTK
#include <sys/stat.h>
#include <gtk/gtk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#endif

#include "umd.h"
#include "umd_internals.h"

int
main (int argc, char *argv[])
{
  umd_t *umd;
  umd_chapter_t *chapter;
  umd_page_t *page;
  umd_cover_t *cover;
  FILE *fp;
  int i, len, cpt_count, pg_count, w, h;
  unsigned char *data;
  char line[0x1000], content[0x2000];
#ifdef ENABLE_GTK
  GdkPixbuf *pixbuf;

  gtk_init (&argc, &argv);
#endif

  if (argc < 2)
    {
      return 1;
    }

  umd = umd_new_from_file (argv[1]);
  if (umd == NULL)
    {
      return 2;
    }

  umd_fprintf (umd, stderr);

  cpt_count = umd_get_chapter_n (umd);
  if (cpt_count == 0)
    {
      return 3;
    }

  for (i = 0; i < cpt_count; ++i)
    {
      chapter = umd_get_nth_chapter (umd, i);
      if (chapter == NULL)
	{
	  return 31;
	}
      fprintf (stdout, "Chapter %d: %s\n", i, chapter->title);
//      fprintf (stdout, "\n---------------->\n%s\n<--------------\n\n\n",
//               chapter->content);
      fprintf (stdout, "at page: %d\n", chapter->page_index);

      free (chapter->title);
      free (chapter->content);
      free (chapter);
    }

  pg_count = umd_get_page_n (umd);
  if (pg_count == 0)
    {
      return 4;
    }

  fprintf (stdout, "Page Count: %d\n", pg_count);
  for (i = 0; i < pg_count; ++i)
    {
      page = umd_get_nth_page (umd, i);
      if (page == NULL)
	{
	  return 41;
	}

      if ((len = umd_page_get_content (page, content, sizeof content)) < 0)
	{
	  return 42;
	}
      content[len] = '\0';

      fprintf (stdout, "Page: %d\n", i);
      fprintf (stdout, "\n---------------->\n%s\n<--------------\n\n",
	       content);

      umd_page_get_size (page, &w, &h);
      data = malloc ((w + 100) * (h + 100) * 3);
      umd_set_font_face (umd, "Nimbus Sans L", 0, 0);
      umd_page_render (page, 0, 0, w, h, 1.0, 0, data, (size_t) (w * 3));
#ifdef ENABLE_GTK
      sprintf (line, "%s.dir", argv[1]);
      mkdir (line, 0755);
      sprintf (line, "%s.dir/%04d.jpg", argv[1], i);
      pixbuf = gdk_pixbuf_new_from_data (data, GDK_COLORSPACE_RGB,
					 FALSE, 8, w, h, 3 * w, NULL, NULL);
      gdk_pixbuf_save (pixbuf, line, "jpeg", NULL, NULL);
#endif

      umd_page_free (page);
    }

  cover = umd_get_cover (umd);
  if (cover == NULL)
    {
      return 5;
    }

  fp = fopen ("cover.jpg", "wb");
  if (fp == NULL)
    {
      return 6;
    }

  fwrite (cover->image_data, 1, cover->image_size, fp);
  fclose (fp);

  free (cover->image_data);
  free (cover);

  umd_set_encoding (umd, "GB2312");
  if (umd_parser_file (umd, argv[1]) < 0)
    {
      return 7;
    }

  umd_destroy (umd);

  return 0;
}

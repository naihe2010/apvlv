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
/* @CFILE umd.c
*
*  Author: Alf <naihe2010@gmail.com>
*/
/* @date Created: 2010/12/03 09:41:30 Alf*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdarg.h>
#include <ctype.h>
#include <zlib.h>

#include "umd.h"
#include "umd_internals.h"

static void default_log (void *, const char *, ...);

static int umd_parser_fp (umd_t *, FILE *);

static int umd_parser_control_seg (umd_t *, FILE *);
static umd_content_t *umd_parser_data_seg (umd_t *, FILE *, int);
static int umd_parser_chapter_offset (umd_t *, FILE *);
static int umd_parser_chapter_title (umd_t *, FILE *);
static int umd_parser_page_offset (umd_t *, FILE *,
				   unsigned char, unsigned char);
static int umd_parser_text_end (umd_t *, FILE *);

static int change_lineend (char *, size_t, const char *, const char *,
			   size_t);
static int uncompress_string (const char *, size_t, char **, size_t *);
static int get_near_pageend (const char *, size_t);

static const char *const default_encoding = "UTF-8";
static const char default_umd_lineend[] = { 0x29, 0x20 };
static const char default_lineend[] = { 0x0A, 0x00 };
static const char default_umd_pageend[] = { 0x20, 0x00 };

umd_t *
umd_new ()
{
  umd_t *umd;

  umd = calloc (1, sizeof (umd_t));
  if (umd == NULL)
    {
      default_log (NULL, "Memory error.\n");
      return NULL;
    }

  umd->to_encoding = default_encoding;

  umd->iconv = (iconv_t) - 1;

  umd->loglevel = 0;
  umd->log = default_log;
  umd->logarg = NULL;

  return umd;
}

umd_t *
umd_new_from_file (const char *file)
{
  umd_t *umd;

  umd = umd_new ();
  if (umd == NULL)
    {
      return NULL;
    }

  if (umd_parser_file (umd, file) < 0)
    {
      umd_destroy (umd);
      return NULL;
    }

  return umd;
}

#define UMD_GET_ARGU(a)                         \
inline char *                                   \
umd_get_##a (umd_t *umd)                        \
{                                               \
  char *ip, *op, ubufx200[0x200];               \
  size_t ilen, olen;                            \
                                                \
  if (* (umd->s_##a) == '\0')                   \
    {                                           \
      return NULL;                              \
    }                                           \
                                                \
  ip = umd->s_##a;                              \
  ilen = strlen (umd->s_##a);                   \
  op = ubufx200;                                \
  olen = sizeof ubufx200;                       \
                                                \
  iconv (umd->iconv, &ip, &ilen, &op, &olen);   \
  * op = '\0';                                  \
                                                \
  return strdup (ubufx200);                     \
}

UMD_GET_ARGU (title);
UMD_GET_ARGU (author);
UMD_GET_ARGU (year);
UMD_GET_ARGU (month);
UMD_GET_ARGU (day);
UMD_GET_ARGU (type);
UMD_GET_ARGU (publisher);
UMD_GET_ARGU (vendor);

int
umd_get_chapter_n (umd_t * umd)
{
  return umd->chapter_count;
}

umd_chapter_t *
umd_get_nth_chapter (umd_t * umd, size_t id)
{
  umd_chapter_t *chapter;
  size_t ilen, olen;
  char *ip, *op;

  if (id >= umd->chapter_count)
    {
      return NULL;
    }

  chapter = calloc (1, sizeof (umd_chapter_t));
  chapter->title = strdup (umd->chapter_list[id].title);

  chapter->page_index = umd->chapter_list[id].page_start;

  ilen = umd->chapter_list[id].length;
  ip = umd->total_string + umd->chapter_list[id].offset;
  olen = umd->chapter_list[id].length << 1;
  op = chapter->content = malloc (olen);
  if (chapter->content == NULL)
    {
      free (chapter->title);
      free (chapter);
      return NULL;
    }

  iconv (umd->iconv, &ip, &ilen, &op, &olen);
  *op = '\0';
  chapter->length = (umd->chapter_list[id].length << 1) - olen;

  return chapter;
}

char *
umd_get_content (umd_t * umd, size_t * lenp)
{
  char *string;

  if (umd->total_string == NULL || umd->total_length == 0)
    {
      return NULL;
    }

  string = malloc (umd->total_length);
  if (string == NULL)
    {
      return NULL;
    }

  memcpy (string, umd->total_string, umd->total_length);
  if (lenp)
    {
      *lenp = umd->total_length;
    }

  return string;
}

umd_cover_t *
umd_get_cover (umd_t * umd)
{
  umd_cover_t *cover;

  if (umd->cover_content->content == NULL)
    {
      return NULL;
    }

  cover = malloc (sizeof (umd_cover_t));
  if (cover == NULL)
    {
      return NULL;
    }

  cover->type = umd->cover_type;
  cover->image_data = malloc (umd->cover_content->length);
  if (cover->image_data == NULL)
    {
      free (cover);
      return NULL;
    }

  memcpy (cover->image_data, umd->cover_content->content,
	  umd->cover_content->length);
  cover->image_size = umd->cover_content->length;

  return cover;
}

int *
umd_write_file (umd_t * umd, const char *file)
{
  return 0;
}

int *
umd_write_fp (umd_t * umd, FILE * fp)
{
  return 0;
}

void
umd_set_log (umd_t * umd, int level, umd_log log, void *arg)
{
  umd->loglevel = level;
  umd->log = log;
  umd->logarg = arg;
}

void
umd_set_encoding (umd_t * umd, const char *to)
{
  umd->to_encoding = to;
}

void
umd_set_font_face (umd_t * umd, const char *face, int italic, int bold)
{
  umd->render_font = face;
  umd->render_italic = italic;
  umd->render_bold = bold;
}

void
umd_fprintf (umd_t * umd, FILE * fp)
{
  if (umd->s_title)
    {
      fprintf (fp, "Title: %s\n", umd->s_title);
    }
  if (umd->s_author)
    {
      fprintf (fp, "Author: %s\n", umd->s_author);
    }
  if (umd->s_year)
    {
      fprintf (fp, "Year: %s\n", umd->s_year);
    }
  if (umd->s_month)
    {
      fprintf (fp, "Month: %s\n", umd->s_month);
    }
  if (umd->s_day)
    {
      fprintf (fp, "Day: %s\n", umd->s_day);
    }
  if (umd->s_type)
    {
      fprintf (fp, "Type: %s\n", umd->s_type);
    }
  if (umd->s_publisher)
    {
      fprintf (fp, "Publisher: %s\n", umd->s_publisher);
    }
  if (umd->s_vendor)
    {
      fprintf (fp, "Seller: %s\n", umd->s_vendor);
    }
}

void
umd_destroy (umd_t * umd)
{
  umd_free (umd);
  free (umd);
}

void
umd_free (umd_t * umd)
{
  umd_content_t *content;
  umd_pageinfo_t *pi;
  size_t i;

  iconv_close (umd->iconv);
  umd->iconv = (iconv_t) - 1;

  if (umd->cover_content)
    {
      free (umd->cover_content->content);
      free (umd->cover_content);
      umd->cover_content = NULL;
    }

  if (umd->chapter_list)
    {
      for (i = 0; i < umd->chapter_count; ++i)
	{
	  free (umd->chapter_list[i].title);
	}
      free (umd->chapter_list);
      umd->chapter_list = NULL;
    }

  while ((pi = umd->pageinfo_head) != NULL)
    {
      umd->pageinfo_head = pi->next;
      free (pi);
    }

  while ((content = umd->content_head) != NULL)
    {
      umd->content_head = content->next;

      free (content->content);
      free (content);
    }

  if (umd->total_string)
    {
      free (umd->total_string);
      umd->total_string = NULL;
    }
}

int
umd_parser_file (umd_t * umd, const char *file)
{
  int ret;
  FILE *fp;

  fp = fopen (file, "rb");
  if (fp == NULL)
    {
      UMD_ERROR (umd, "Open file: %s error: %s", file, strerror (errno));
      return -1;
    }

  ret = umd_parser_fp (umd, fp);

  fclose (fp);

  return ret;
}

static int
umd_parser_fp (umd_t * umd, FILE * fp)
{
  int ret, pg_ind, offset, length;
  size_t i;
  char bufx10[0x10], *buffer;
  umd_content_t *content;
  umd_pageinfo_t *cur_pi, *pi;

  /* make sure this call be called mutiple times */
  umd_free (umd);

  if (umd->iconv == (iconv_t) - 1)
    {
      umd->iconv = iconv_open (umd->to_encoding, "UTF-16LE");
    }

  if (umd->iconv == (iconv_t) - 1)
    {
      UMD_ERROR (umd, "iconv open failed.\n");
      return -1;
    }

  if (fread (bufx10, 1, 4, fp) < 4)
    {
      UMD_ERROR (umd, "Read umd header failed: %s\n", strerror (errno));
      return -1;
    }

  if (*(size_t *) bufx10 != UMD_MAGIC)
    {
      UMD_ERROR (umd, "It is not a UMD file.\n");
      return -1;
    }

  while (fread (bufx10, 1, 1, fp) == 1)
    {
      if (*bufx10 == UMD_CONTROL_SEPARATOR)
	{
	  ret = umd_parser_control_seg (umd, fp);
	}
      else if (*bufx10 == UMD_DATA_SEPARATOR)
	{
	  content = umd_parser_data_seg (umd, fp, 0);
	  if (umd->content_head == NULL)
	    {
	      umd->content_head = content;
	      umd->content_tail = content;
	    }
	  else
	    {
	      umd->content_tail->next = content;
	      umd->content_tail = content;
	    }
	}
      else
	{
	  UMD_ERROR (umd, "Can't parser this charater.\n");
	  ret = -1;
	}

      if (ret < 0)
	{
	  return -1;
	}
    }

  cur_pi = NULL;

  /** caculate the page for renderer */
  for (pg_ind = 0, buffer = umd->total_string, length = umd->total_length;
       length > 0; pg_ind++, buffer += offset, length -= offset)
    {
      pi = calloc (1, sizeof (umd_pageinfo_t));
      if (pi == NULL)
	{
	  return -1;
	}

      offset = get_near_pageend (buffer, length);

      pi->length = offset;
      pi->id = pg_ind;
      UMD_DEBUG (umd, "pi %p: id: %d.\n", pi, pi->id);
      pi->char_begin = umd->total_string + umd->total_length - length;

      if (cur_pi == NULL)
	{
	  umd->pageinfo_head = pi;
	}
      else
	{
	  cur_pi->next = pi;
	}
      cur_pi = pi;
    }

  umd->page_count = pg_ind;

  for (i = 0, pi = umd->pageinfo_head; i < umd->chapter_count; ++i)
    {
      umd->chapter_list[i].page_start = pi->id;
      UMD_DEBUG (umd, "pi %p: id: %d.\n", pi, pi->id);
      while (pi->next
	     && umd->chapter_list[i].offset + umd->chapter_list[i].length
	     > pi->char_begin - umd->total_string + pi->length)
	{
	  pi = pi->next;
	}
      umd->chapter_list[i].page_end = pi->id;
      UMD_DEBUG (umd, "chapter %d contained: %d->%d.\n", i,
		 umd->chapter_list[i].page_start,
		 umd->chapter_list[i].page_end);
    }

  return 0;
}

static int
umd_parser_control_seg (umd_t * umd, FILE * fp)
{
  unsigned char id, categore[2], len;
  char bufx100[0x100];
  int ret;

  if (fread (&id, 1, 1, fp) < 1
      || fread (categore, 1, 2, fp) < 2 || fread (&len, 1, 1, fp) < 1)
    {
      return -1;
    }

  len -= 5;
  if (fread (bufx100, 1, len, fp) < len)
    {
      return -1;
    }

  UMD_DEBUG (umd, "Control ID: 0x%x, length: %d\n", id, len);

  bufx100[len] = '\0';
  ret = 0;

  if (id == UMD_TEXT_VERSION)
    {
      umd->book_type = *bufx100;
    }

  else if (id >= UMD_TEXT_TITLE && id <= UMD_TEXT_VENDOR)
    {
      switch (id)
	{
	case UMD_TEXT_TITLE:
	  strncpy (umd->s_title, bufx100, sizeof bufx100 - 1);
	  break;

	case UMD_TEXT_AUTHOR:
	  strncpy (umd->s_author, bufx100, sizeof bufx100 - 1);
	  break;

	case UMD_TEXT_YEAR:
	  strncpy (umd->s_year, bufx100, sizeof bufx100 - 1);
	  break;

	case UMD_TEXT_MONTH:
	  strncpy (umd->s_month, bufx100, sizeof bufx100 - 1);
	  break;

	case UMD_TEXT_DAY:
	  strncpy (umd->s_day, bufx100, sizeof bufx100 - 1);
	  break;

	case UMD_TEXT_TYPE:
	  strncpy (umd->s_type, bufx100, sizeof bufx100 - 1);
	  break;

	case UMD_TEXT_PUBLISHER:
	  strncpy (umd->s_publisher, bufx100, sizeof bufx100 - 1);
	  break;

	case UMD_TEXT_VENDOR:
	  strncpy (umd->s_vendor, bufx100, sizeof bufx100 - 1);
	  break;
	}
    }

  else if (id == UMD_TEXT_TOPPAGE)
    {
      umd->cover_content = umd_parser_data_seg (umd, fp, 1);
      if (umd->cover_content == NULL)
	{
	  UMD_ERROR (umd, "Get text cover failed.\n");
	  return -1;
	}
    }

  else if (id == UMD_TEXT_CHAPTER_OFFSET)
    {
      ret = umd_parser_chapter_offset (umd, fp);
    }

  else if (id == UMD_TEXT_CHAPTER_TITLE)
    {
      ret = umd_parser_chapter_title (umd, fp);
    }

  else if (id == UMD_TEXT_PAGE_OFFSET)
    {
      ret = umd_parser_page_offset (umd, fp, bufx100[0], bufx100[1]);
    }

  else if (id == UMD_TEXT_END)
    {
      ret = umd_parser_text_end (umd, fp);
    }

  return ret;
}

static int
umd_parser_chapter_offset (umd_t * umd, FILE * fp)
{
  umd_content_t *content;
  size_t i;

  content = umd_parser_data_seg (umd, fp, 1);
  if (content == NULL)
    {
      UMD_ERROR (umd, "Get text cover failed.\n");
      return -1;
    }

  umd->chapter_count = content->length >> 2;
  umd->chapter_list = calloc (umd->chapter_count, sizeof
			      (umd_chapter_context_t));
  if (umd->chapter_list == NULL)
    {
      UMD_ERROR (umd, "Memory error.\n");
      free (content->content);
      free (content);
      return -1;
    }

  for (i = 0; i < umd->chapter_count; ++i)
    {
      umd->chapter_list[i].offset = *(size_t *) (content->content + 4 * i);
    }

  free (content->content);
  free (content);

  return 0;
}

static int
umd_parser_chapter_title (umd_t * umd, FILE * fp)
{
  umd_content_t *content;
  size_t i, offset, ilen, olen;
  char obufx100[0x100], *ip, *op;
  unsigned char len;

  content = umd_parser_data_seg (umd, fp, 1);
  if (content == NULL)
    {
      UMD_ERROR (umd, "Get text cover failed.\n");
      return -1;
    }

  for (i = 0, offset = 0; i < umd->chapter_count; ++i)
    {
      len = *(content->content + offset);

      ip = content->content + offset + 1;
      ilen = len;
      op = obufx100;
      olen = sizeof obufx100;

      iconv (umd->iconv, &ip, &ilen, &op, &olen);
      *op = '\0';

      umd->chapter_list[i].title = strdup (obufx100);
      UMD_DEBUG (umd, "get chapter title: [%s]\n", obufx100);

      offset += (1 + len);
    }

  free (content->content);
  free (content);

  return 0;
}

static int
umd_parser_page_offset (umd_t * umd, FILE * fp, unsigned char x,
			unsigned char y)
{
  umd_content_t *content;

  content = umd_parser_data_seg (umd, fp, 1);
  if (content == NULL)
    {
      UMD_ERROR (umd, "Get page offset data seg failed.\n");
      return -1;
    }

  free (content->content);
  free (content);

  return 0;
}

static int
umd_parser_text_end (umd_t * umd, FILE * fp)
{
  umd_content_t *content;
  int i, last, ret;
  char *ip;
  size_t len;

  for (content = umd->content_head, umd->total_length = 0;
       content != NULL; content = content->next, umd->total_length += len)
    {
      if ((ret =
	   uncompress_string (content->content, content->length, &ip,
			      &len)) < 0)
	{
	  UMD_ERROR (umd, "uncompress failed: %d\n", ret);
	  return -1;
	}
      else
	{
	  free (content->content);
	  content->content = (char *) ip;
	  content->length = (size_t) len;
	}
    }

  umd->total_string = malloc (umd->total_length + 1);
  for (content = umd->content_head, umd->total_length = 0;
       content != NULL; content = umd->content_head)
    {
      change_lineend (content->content, content->length, default_umd_lineend,
		      default_lineend, sizeof default_lineend);

      memcpy (umd->total_string + umd->total_length, content->content,
	      content->length);
      umd->total_length += content->length;
      umd->content_head = content->next;
      free (content->content);
      free (content);
    }
  umd->total_string[umd->total_length] = '\0';

  last = umd->chapter_count - 1;
  for (i = 0; i < last; ++i)
    {
      umd->chapter_list[i].length = umd->chapter_list[i + 1].offset -
	umd->chapter_list[i].offset;
    }
  umd->chapter_list[last].length =
    umd->total_length - umd->chapter_list[last].offset;

  return 0;
}

static umd_content_t *
umd_parser_data_seg (umd_t * umd, FILE * fp, int readid)
{
  char randval[4];
  size_t len;
  umd_content_t *content;
  char *p;

  if (readid)
    {
      fread (randval, 1, 1, fp);
    }

  if (fread (randval, 1, 4, fp) < 4 || fread (&len, 1, 4, fp) < 4)
    {
      UMD_ERROR (umd, "read error: %s\n", strerror (errno));
      return NULL;
    }

  len -= 9;

  UMD_DEBUG (umd, "data seg randval: %02x%02x%02x%02x, length: %lu\n",
	     (int) randval[0], (int) randval[1], (int) randval[2],
	     (int) randval[3], len);

  p = (char *) malloc (len);
  if (p == NULL)
    {
      UMD_ERROR (umd, "Memory error.\n");
      return NULL;
    }

  if (fread (p, 1, len, fp) < len)
    {
      UMD_ERROR (umd, "read error: %s\n", strerror (errno));
      return NULL;
    }

  content = calloc (1, sizeof (umd_content_t));
  if (content == NULL)
    {
      UMD_ERROR (umd, "Memory error.\n");
      free (p);
      return NULL;
    }

  memcpy (content->randval, randval, 4);
  content->content = p;
  content->length = len;
  content->next = NULL;

  return content;
}

static void
default_log (void *arg, const char *format, ...)
{
  va_list vap;

  va_start (vap, format);
  vfprintf (stderr, format, vap);
  va_end (vap);
}

static int
change_lineend (char *input, size_t isize,
		const char *from, const char *to, size_t size)
{
  size_t i, skip;

  skip = size - 1;
  isize -= skip;

  for (i = 0; i < isize; ++i)
    {
      if (memcmp (input + i, from, size) == 0)
	{
	  memcpy (input + i, to, size);
	  i += skip;
	}
    }

  return 0;
}

static int
uncompress_string (const char *src, size_t slen, char **dstp, size_t * dlenp)
{
  Bytef *op;
  uLongf len;
  int ret;

  len = slen << 1;
  op = malloc (len);
  if (op == NULL)
    {
      return Z_BUF_ERROR;
    }

  while ((ret = uncompress (op, &len, (const Bytef *) src, (uLongf) slen)) ==
	 Z_BUF_ERROR)
    {
      free (op);
      len = len << 1;
      op = malloc (len);
      if (op == NULL)
	{
	  return ret;
	}
    }

  if (ret != Z_OK)
    {
      return ret;
    }

  *dstp = (char *) op;
  *dlenp = (size_t) len;

  return 0;
}

static int
get_near_pageend (const char *buffer, size_t size)
{
  const char *p;

  if (size < LIBUMD_PAGE_CHARS)
    {
      return size;
    }

  for (p = buffer + LIBUMD_PAGE_CHARS - 1; p >= buffer; --p)
    {
      if (memcmp (p - 1, default_umd_lineend, sizeof default_umd_lineend) == 0
	  || memcmp (p - 1, default_umd_pageend,
		     sizeof default_umd_pageend) == 0)
	{
	  return p - buffer + 1;
	}
    }

  return LIBUMD_PAGE_CHARS;
}

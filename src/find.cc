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
/* @CFILE ifind.c
 *
 *  Author: Alf <naihe2010@126.com>
 */

#include "find.h"
#include "cache.h"
#include "ebook.h"
#include "hash.h"

#include <cstring>

#ifndef FD_COMP_CNT
#define FD_COMP_CNT 2
#endif

struct st_hash
{
  int seek;
  hash_t hash;
};

struct st_file
{
  const char *path;
  float length;
  float offset;
  struct st_hash head[1];
  struct st_hash tail[1];
  hash_array_t *hashArray;
};

struct st_find
{
  GPtrArray *ptr[0x10];
  find_step *step;
  find_step_cb cb;
  GThreadPool *thread_pool;
  gpointer arg;
};

int
find_ebooks (GPtrArray *ptr, find_step_cb cb, gpointer arg)
{
  guint i, j;
  int dist, count;
  ebook_hash_t *hashs;
  char *path;
  find_step step[1];

  if (g_cache == nullptr)
    {
      cache_open ();
    }

  count = 0;

  hashs = g_new0 (ebook_hash_t, ptr->len);
  g_return_val_if_fail (hashs, 0);

  step->found = FALSE;
  step->total = ptr->len;
  step->doing = "Generate ebook hash value";
  for (i = 0; i < ptr->len; ++i)
    {
      path = (gchar *)g_ptr_array_index (ptr, i);
      if (cache_get (g_cache, path, 0, 0, &(hashs[i].cover_hash)) != TRUE)
        {
          if (ebook_file_hash (path, hashs + i) == 0)
            {
              cache_set (g_cache, path, 0, 0, hashs[i].cover_hash);
            }
        }
      step->now = i;
      if (cb (step, arg) == FALSE)
        {
          goto ret;
        }
    }

  step->doing = "Compare ebook hash value";
  step->now = 0;
  for (i = 0; i < ptr->len - 1; ++i)
    {
      for (j = i + 1; j < ptr->len; ++j)
        {
          dist = ebook_hash_cmp (hashs + i, hashs + j);
          if (dist < 10)
            {
              step->afile = (char *)g_ptr_array_index (ptr, i);
              step->bfile = (char *)g_ptr_array_index (ptr, j);
              step->found = TRUE;
              step->type = FD_SAME_EBOOK;
              if (cb (step, arg) == FALSE)
                goto ret;

              ++count;
            }
        }

      step->now = i;
      step->found = FALSE;
      if (cb (step, arg) == FALSE)
        {
          goto ret;
        }
    }

ret:
  g_free (hashs);

  return count;
}

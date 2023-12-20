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
/* @CFILE cache.c
 *
 *  Author: Alf <naihe2010@126.com>
 */

#include "cache.h"

#include <glib.h>
#include <glib/gstdio.h>

#include <errno.h>
#include <sqlite3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

cache_t *g_cache;

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

#ifdef WIN32
#define strtouq _strtoui64
#endif

struct cache_s
{
  sqlite3 *db;
  gchar *file;
};

static gboolean cache_exec (cache_t *cache,
                            int (*cb) (void *, int, char **, char **),
                            void *arg, const char *fmt, ...);

const char *init_text
    = "create table media(id INTEGER PRIMARY KEY AUTOINCREMENT, path text, "
      "size bigint, mtime bigint);"
      "create table hash(id INTEGER PRIMARY KEY AUTOINCREMENT, media_id "
      "integer, alg int, offset real, hash varchar(32));"
      "create unique index index_path on media (path);";

static void
cache_init (cache_t *cache)
{
  cache_exec (cache, NULL, NULL, init_text);
}

cache_t *
cache_open ()
{
  cache_t *cache;
  gchar *dirname;
  gboolean needInit;

  cache = static_cast<cache_t *> (g_malloc (sizeof (cache_t)));
  g_return_val_if_fail (cache, NULL);

  cache->file = g_strdup_printf ("%s/.cache/apvlv/hash.db", g_get_home_dir ());

  needInit = FALSE;
  if (g_file_test (cache->file, G_FILE_TEST_EXISTS) == FALSE)
    {
      needInit = TRUE;
      dirname = g_path_get_dirname (cache->file);
      g_mkdir_with_parents (dirname, 0755);
      g_free (dirname);
    }

  if (sqlite3_open (cache->file, &cache->db) != 0)
    {
      g_warning ("Open cache file: %s failed:%s.", cache->file,
                 strerror (errno));
      g_free (cache);
      return NULL;
    }

  if (needInit)
    {
      cache_init (cache);
    }

  if (g_cache == NULL)
    {
      g_cache = cache;
    }

  return cache;
}

void
cache_close (cache_t *cache)
{
  sqlite3_close (cache->db);
  g_free (cache->file);
  g_free (cache);
}

static gboolean
cache_exec (cache_t *cache, int (*cb) (void *, int, char **, char **),
            void *arg, const char *fmt, ...)
{
  int rc;
  va_list ap;
  char text[1024], *errMsg;

  va_start (ap, fmt);
  vsnprintf (text, sizeof text, fmt, ap);
  va_end (ap);

  rc = sqlite3_exec (cache->db, text, cb, arg, &errMsg);
  if (rc != SQLITE_OK)
    {
      g_warning ("SQL error: %s in [%s]\n", errMsg, text);
      sqlite3_free (errMsg);
      return FALSE;
    }

  return TRUE;
}

static int
get_id_callback (void *para, int n_column, char **column_value,
                 char **column_name)
{
  int *idp = (int *)para;
  if (column_value[0] != NULL)
    *idp = atoi (column_value[0]);
  return 0;
}

static int
get_hash_callback (void *para, int n_column, char **column_value,
                   char **column_name)
{
  hash_t *hp = (hash_t *)para;
  if (column_value[0] != NULL)
    {
      *hp = strtoull (column_value[0], NULL, 10);
    }
  return 0;
}

static int
cache_get_media_id (cache_t *cache, const gchar *file)
{
  int media_id;
  gboolean ret;

  media_id = -1;

  ret = cache_exec (cache, get_id_callback, &media_id,
                    "select id from media where path='%s';", file);
  g_return_val_if_fail (ret, -1);

  if (media_id == -1)
    {
      GStatBuf buf[1];
      if (g_stat (file, buf) != 0)
        {
          g_warning ("stat error: %s", strerror (errno));
          return -1;
        }

#if WIN32
      ret = cache_exec (
          cache, NULL, NULL,
          "insert into media(path, size, mtime) values('%s', %ld, %ld);", file,
          buf->st_size, buf->st_mtime);
#else
      ret = cache_exec (
          cache, NULL, NULL,
          "insert into media(path, size, mtime) values('%s', %ld, %ld);", file,
          buf->st_size, buf->st_mtim.tv_sec);
#endif
      g_return_val_if_fail (ret, -1);
    }

  ret = cache_exec (cache, get_id_callback, &media_id,
                    "select id from media where path='%s';", file);
  g_return_val_if_fail (ret, -1);

  return media_id;
}

gboolean
cache_get (cache_t *cache, const gchar *file, float off, int alg, hash_t *hp)
{
  int media_id;
  gboolean ret;

  media_id = cache_get_media_id (cache, file);
  g_return_val_if_fail (media_id != -1, FALSE);

  *hp = 0;
  ret = cache_exec (
      cache, get_hash_callback, hp,
      "select hash from hash where offset=%f and alg=%d and media_id=%d", off,
      alg, media_id);
  g_return_val_if_fail (ret, FALSE);

  return *hp != 0;
}

gboolean
cache_set (cache_t *cache, const gchar *file, float off, int alg, hash_t h)
{
  int media_id;
  gboolean ret;

  media_id = cache_get_media_id (cache, file);
  g_return_val_if_fail (media_id != -1, FALSE);

  ret = cache_exec (cache, NULL, NULL,
                    "insert into hash(media_id, offset, alg, hash) values(%d, "
                    "%f, %d, '%lld')",
                    media_id, off, alg, h);
  g_return_val_if_fail (ret, FALSE);

  return TRUE;
}

gboolean
cache_remove (cache_t *cache, const gchar *file)
{
  int media_id;

  media_id = cache_get_media_id (cache, file);
  g_return_val_if_fail (media_id != -1, FALSE);

  cache_exec (cache, NULL, NULL, "delete from hash where media_id=%d;",
              media_id);

  cache_exec (cache, NULL, NULL, "delete from media where id=%d;", media_id);

  return TRUE;
}

static int
cache_remove_if_no_exists_callback (void *para, int n_column,
                                    char **column_value, char **column_name)
{
  cache_t *cache = (cache_t *)para;
  char *path;
  int media_id;

  if (column_value[0] != NULL)
    {
      path = column_value[1];
      if (g_file_test (path, G_FILE_TEST_EXISTS) == FALSE)
        {
          media_id = atoi (column_value[0]);
          cache_exec (cache, NULL, NULL, "delete from hash where media_id=%d;",
                      media_id);
          cache_exec (cache, NULL, NULL, "delete from media where id=%d;",
                      media_id);
        }
    }
  return 0;
}

void
cache_cleanup (cache_t *cache)
{
  cache_exec (cache, cache_remove_if_no_exists_callback, cache,
              "select id, path from media;");
}

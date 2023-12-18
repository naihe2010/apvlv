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
/* @CFILE cache.h
 *
 *  Author: Alf <naihe2010@126.com>
 */

#ifndef _FDUPVES_CACHE_H_
#define _FDUPVES_CACHE_H_

#include "hash.h"

#include <glib.h>

typedef struct cache_s cache_t;

cache_t *cache_open(const gchar *file);

void cache_close(cache_t *cache);

gboolean cache_get(cache_t *, const gchar *, float, int, hash_t *);

gboolean cache_set(cache_t *, const gchar *, float, int, hash_t);

gboolean cache_gets(cache_t *, const gchar *, int alg, hash_array_t **);

gboolean cache_sets(cache_t *, const gchar *, int alg, hash_array_t *);

gboolean cache_remove(cache_t *, const gchar *);

void cache_cleanup(cache_t *);

extern cache_t *g_cache;

#endif

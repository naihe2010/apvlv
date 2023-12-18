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
/* @CFILE ebook.h
 *
 *  Author: Alf <naihe2010@126.com>
 */

#ifndef _FDUPVES_EBOOK_H_
#define _FDUPVES_EBOOK_H_

#include <libxml/tree.h>
#include <libxml/xpath.h>
#include "hash.h"

#define FDUPVES_TITLE_LEN 1024
#define FDUPVES_AUTHOR_LEN 256
#define FDUPVES_ISBN_LEN 128

typedef struct
{
  hash_t cover_hash;
  char title[FDUPVES_TITLE_LEN];
  char author[FDUPVES_AUTHOR_LEN];
  char producer[FDUPVES_AUTHOR_LEN];
  struct
  {
    unsigned short year;
    unsigned char month;
    unsigned char day;
  } public_date;
  char isbn[FDUPVES_ISBN_LEN];
} ebook_hash_t;

int ebook_file_hash (const char *file, ebook_hash_t *ehash);

int ebook_hash_cmp (ebook_hash_t *ha, ebook_hash_t *hb);

int is_ebook (const gchar *);

xmlNodeSetPtr xmldoc_get_nodeset (xmlDocPtr doc, const char *xpath,
                                  const char *ns, const char *url);

xmlNodePtr xmldoc_get_node (xmlDocPtr doc, const char *xpath, const char *ns,
                            const char *url);

const gchar *xml_get_value (const gchar *content, const gchar *xpath,
                            gchar *out, size_t outlen);

char *xmlnode_attr_get (xmlNodePtr node, const char *attr);

#endif

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
/* @CFILE ebook_epub.c
 *
 *  Author: Alf <naihe2010@126.com>
 */

#include "ebook.h"
#include "ApvlvUtil.h"
#include <ctype.h>
#include <epub.h>
#include <libxml/tree.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>
#include <string.h>

char *
container_get_contentfile (const char *container, int len)
{
  xmlDocPtr doc;
  xmlNodePtr node;
  char *path;

  doc = xmlReadMemory (container, len, NULL, NULL,
                       XML_PARSE_NOBLANKS | XML_PARSE_NSCLEAN);
  if (doc == NULL)
    {
      return NULL;
    }

  node = xmldoc_get_node (
      doc, "//c:container/c:rootfiles/c:rootfile/@full-path", "c",
      "urn:oasis:names:tc:opendocument:xmlns:container");
  if (node == NULL)
    {
      xmlFreeDoc (doc);
      return NULL;
    }

  path = g_strdup ((char *)node->children->content);
  xmlFreeDoc (doc);

  return path;
}

static char *
content_get_cover (struct epub *epub, const char *contentfile,
                   const char *cover_id)
{
  char *contentopf;
  xmlDocPtr doc;
  xmlNodeSetPtr nodeset;
  xmlNodePtr node;
  char *href, *href2, *dirname, *data, *id, *cover = NULL;
  int cover_fd, dlen;
  GError *gerr;

  gint clen = epub_get_ocf_file (epub, contentfile, &contentopf);
  if (clen <= 0)
    {
      return NULL;
    }

  doc = xmlReadMemory (contentopf, clen, NULL, NULL,
                       XML_PARSE_NOBLANKS | XML_PARSE_NSCLEAN);
  free (contentopf);
  if (doc == NULL)
    {
      return NULL;
    }

  nodeset = xmldoc_get_nodeset (doc, "//c:package/c:manifest/c:item", "c",
                                "http://www.idpf.org/2007/opf");
  if (nodeset == NULL)
    {
      nodeset = xmldoc_get_nodeset (doc, "//item", NULL, NULL);
    }
  if (nodeset)
    {
      for (int i = 0; i < nodeset->nodeNr; ++i)
        {
          node = nodeset->nodeTab[i];
          href = xmlnode_attr_get (node, "href");
          if (href == NULL)
            {
              continue;
            }

          id = xmlnode_attr_get (node, "id");
          if (strcasecmp (id, cover_id) != 0)
            {
              g_free (id);
              g_free (href);
              continue;
            }

          if (strchr (contentfile, '/'))
            {
              dirname = g_path_get_dirname (contentfile);
              href2 = href;
              href = g_strjoin ("/", dirname, href2, NULL);
              g_free (href2);
              g_free (dirname);
            }

          dlen = epub_get_ocf_file (epub, href, &data);
          if (dlen > 0)
            {
              gerr = NULL;
              cover_fd = g_file_open_tmp ("fdupves_cover_XXXXXX.png", &cover,
                                          &gerr);
              if (cover_fd < 0)
                {
                  if (gerr)
                    {
                      g_warning ("create tempory png file error: %s",
                                 gerr->message);
                      g_error_free (gerr);
                    }
                  g_free (cover);
                  cover = NULL;
                }
              else
                {
                  g_file_set_contents (cover, data, dlen, NULL);
                }
              free (data);
            }

          g_free (id);
          g_free (href);
          break;
        }
    }

  xmlXPathFreeNodeSet (nodeset);

  xmlFreeDoc (doc);
  return cover;
}

static int
meta_get_cover_id (const char *meta, char *cover_id, size_t cover_id_len)
{
  gchar **tokens, *b;
  int got = 0;

  tokens = g_strsplit (meta, ":", 2);
  if (tokens)
    {
      if (strcasecmp (tokens[0], "cover") == 0)
        {
          b = tokens[1];
          while (!isalnum (*b))
            ++b;
          g_snprintf (cover_id, cover_id_len, "%s", b);
          got = 1;
        }
      g_strfreev (tokens);
    }

  return got;
}

#define _epub_get_value(key, value)                                           \
  do                                                                          \
    {                                                                         \
      unsigned char **idp;                                                    \
      int i, idsize;                                                          \
      idp = epub_get_metadata (epub, key, &idsize);                           \
      if (idp != NULL)                                                        \
        {                                                                     \
          snprintf (value, sizeof value, "%s", (char *)idp[0]);               \
          for (i = 0; i < idsize; ++i)                                        \
            free (idp[i]);                                                    \
          free (idp);                                                         \
        }                                                                     \
    }                                                                         \
  while (0)

static void
epub_get_cover_hash (struct epub *epub, ebook_hash_t *ehash)
{
  int i, len, metasize;
  unsigned char **metas;
  gchar *contentfile, cover_id[0x100], *cover, *container;

  metas = epub_get_metadata (epub, EPUB_META, &metasize);
  if (metas)
    {
      for (i = 0; i < metasize; ++i)
        {
          if (meta_get_cover_id (reinterpret_cast<const char *> (metas[i]), cover_id, sizeof cover_id))
            break;
        }
      for (i = 0; i < metasize; ++i)
        {
          free (metas[i]);
        }
      free (metas);
    }
  else
    {
      snprintf (cover_id, sizeof cover_id, "%s", "x_cover");
    }

  len = epub_get_ocf_file (epub, "META-INF/container.xml", &container);
  if (len <= 0)
    {
      return;
    }

  contentfile = container_get_contentfile (container, len);
  free (container);
  if (contentfile == NULL)
    {
      return;
    }

  cover = content_get_cover (epub, contentfile, cover_id);
  g_free (contentfile);

  if (cover)
    {
      ehash->cover_hash = image_file_hash (cover);
      unlink (cover);
      g_free (cover);
    }
}

static const char *maohao2kuohao[] = {
  "Unspecified (uid:",
  "Unspecified (epubmerge-id:",
  "Unspecified (BookId:",
  "URN (BookId:",
  ":urn:uuid:",
  "duokan-book-id:",
  "duokan-book-id (uuid_id:",
  "MOBI-ASIN (Unspecified:",
  "calibre (Unspecified:",
  "ISBN (Unspecified:",
  "ISBN (uid:",
  "uuid (uuid_id:",
  "UUID (BookId:",
};

static int
epub_id_get_isbn (const char *id, char *isbn, size_t isbn_size)
{
  const char *token, *p, *e;
  int len;
  size_t i;

  for (i = 0; i < sizeof (maohao2kuohao) / sizeof (maohao2kuohao[0]); ++i)
    {
      token = maohao2kuohao[i];
      if (strcasestr (id, token))
        {
          p = strcasestr (id, token);
          p += strlen (token);
          e = strchr (p, ')');
          len = e - p;
          if (len >= isbn_size)
            {
              g_warning ("get isbn too long: %s", id);
              return -1;
            }

          memcpy (isbn, p, len);
          isbn[len] = '\0';
          return 0;
        }
    }

  g_warning ("can not parse id: %s", id);
  return -1;
}

int
epub_hash (const char *file, ebook_hash_t *ehash)
{
  struct epub *epub;
  char idstr[0x1000] = { 0 };

  epub = epub_open (file, 0);
  if (epub == NULL)
    {
      g_warning ("open epub file:%s error", file);
      return -1;
    }

  epub_get_cover_hash (epub, ehash);

  _epub_get_value (EPUB_ID, idstr);
  // epub_id_get_isbn (idstr, ehash->isbn, sizeof ehash->isbn);

  //_epub_get_value (EPUB_TITLE, ehash->title);
  //_epub_get_value (EPUB_CREATOR, ehash->author);
  //_epub_get_value (EPUB_PUBLISHER, ehash->producer);

  epub_close (epub);

  return 0;
}

static int
mobi_hash (const char *file, ebook_hash_t *ehash)
{
  return -1;
}

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
/* @CFILE ebook.c
 *
 *  Author: Alf <naihe2010@126.com>
 */

#include "ebook.h"
#include <libxml/xpathInternals.h>
#include <string.h>

#define FDUPVES_EBOOK_HASH_MAX (64)

extern int pdf_hash (const char *file, ebook_hash_t *ehash);
extern int epub_hash (const char *file, ebook_hash_t *ehash);
extern int mobi_hash (const char *file, ebook_hash_t *ehash);

typedef enum
{
  FDUPVES_EBOOK_PDF = 0,
  FDUPVES_EBOOK_EPUB,
  FDUPVES_EBOOK_MOBI,
  FDUPVES_EBOOK_UNKOWN
} fdupves_ebook_type;

struct ebook_impl
{
  fdupves_ebook_type type;
  const char *type_prefix;
  int (*func) (const char *, ebook_hash_t *);
} ebook_impls[FDUPVES_EBOOK_UNKOWN] = {
  {
      FDUPVES_EBOOK_PDF,
      "pdf",
      pdf_hash,
  },
  {
      FDUPVES_EBOOK_EPUB,
      "epub",
      epub_hash,
  },
  {
      FDUPVES_EBOOK_MOBI,
      "mobi",
      mobi_hash,
  },
};

static struct ebook_impl *
ebook_find_impl (const char *file)
{
  const char *p;
  int i;

  p = strrchr (file, '.');

  if (p)
    {
      for (i = 0; i < sizeof ebook_impls / sizeof ebook_impls[0]; ++i)
        {
          if (strcasecmp (p + 1, ebook_impls[i].type_prefix) == 0)
            return ebook_impls + i;
        }
    }

  return NULL;
}

int
ebook_file_hash (const char *file, ebook_hash_t *ehash)
{
  struct ebook_impl const *impl;

  impl = ebook_find_impl (file);
  if (impl == NULL)
    {
      return -1;
    }

  return impl->func (file, ehash);
}

int
ebook_hash_cmp (ebook_hash_t *ha, ebook_hash_t *hb)
{
  if (ha->cover_hash && hb->cover_hash)
    {
      return hash_cmp (ha->cover_hash, hb->cover_hash);
    }

#define _ebook_key_cmp(key)                                                   \
  do                                                                          \
    {                                                                         \
      if (*ha->key != '\0' && *hb->key != '\0'                                \
          && strcmp (ha->key, ha->key) == 0)                                  \
        return 0;                                                             \
    }                                                                         \
  while (0)

  //_ebook_key_cmp (title);
  //_ebook_key_cmp (isbn);
  //_ebook_key_cmp (author);

  return FDUPVES_EBOOK_HASH_MAX;
}


xmlNodeSetPtr
xmldoc_get_nodeset (xmlDocPtr doc, const char *xpath, const char *ns,
                    const char *url)
{
  xmlXPathContextPtr xpathctx;
  xmlXPathObjectPtr xpathobj;
  xmlNodeSetPtr nodes;

  xpathctx = xmlXPathNewContext (doc);
  if (xpathctx == NULL)
    {
      g_warning ("unable to create new XPath context\n");
      return NULL;
    }

  xmlXPathRegisterNs (xpathctx, BAD_CAST ns, BAD_CAST url);

  xpathobj = xmlXPathEvalExpression (BAD_CAST xpath, xpathctx);
  xmlXPathFreeContext (xpathctx);
  if (xpathobj == NULL)
    {
      g_warning ("unable to evaluate xpath expression \"%s\"\n", xpath);
      return NULL;
    }

  if (xmlXPathNodeSetIsEmpty (xpathobj->nodesetval))
    {
      g_debug ("unable to get \"%s\"", xpath);
      xmlXPathFreeObject (xpathobj);
      return NULL;
    }

  nodes = xpathobj->nodesetval;

  xmlXPathFreeNodeSetList (xpathobj);

  return nodes;
}

xmlNodePtr
xmldoc_get_node (xmlDocPtr doc, const char *xpath, const char *ns,
                 const char *url)
{
  xmlNodePtr node = NULL;
  xmlNodeSetPtr nodes = xmldoc_get_nodeset (doc, xpath, ns, url);
  if (nodes != NULL)
    {
      node = nodes->nodeTab[0];
      xmlXPathFreeNodeSet (nodes);
    }

  return node;
}

const gchar *
xml_get_value (const gchar *content, const gchar *xpath, gchar *out,
               size_t outlen)
{
  xmlDocPtr doc;
  xmlNodePtr node;

  doc = xmlReadMemory (content, strlen (content), NULL, NULL,
                       XML_PARSE_NOBLANKS | XML_PARSE_NSCLEAN);
  if (doc == NULL)
    {
      return NULL;
    }

  node = xmldoc_get_node (doc, xpath, "pdfx", "http://ns.adobe.com/pdfx/1.3/");
  if (node == NULL)
    {
      xmlFreeDoc (doc);
      return NULL;
    }

  g_snprintf (out, outlen, "%s", node->children->content);
  xmlFreeDoc (doc);

  return out;
}

char *
xmlnode_attr_get (xmlNodePtr node, const char *attr)
{
  xmlAttrPtr prop;

  for (prop = node->properties; prop != NULL; prop = prop->next)
    {
      if (prop->type == XML_ATTRIBUTE_NODE
          && strcmp ((char *)(prop->name), attr) == 0)
        {
          return g_strdup ((char *)prop->children->content);
        }
    }

  return NULL;
}
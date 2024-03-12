/*
 * This file is part of the apvlv package
 * Copyright (C) <2010>  <Alf>
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
/* @CPPFILE ApvlvHtm.cc
 *
 *  Author: Alf <naihe2010@126.com>
 */
/* @date Created: 2018/04/19 13:51:04 Alf*/

#include "ApvlvEpub.h"
#include "ApvlvHtm.h"
#include "ApvlvInfo.h"
#include "ApvlvUtil.h"

#include "epub.h"

#include <glib.h>
#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <webkit2/webkit2.h>

namespace apvlv
{
ApvlvEPUB::ApvlvEPUB (const char *filename, bool check)
    : ApvlvFile (filename, check)
{
  mEpub = epub_open (filename, 0);
  if (mEpub == nullptr)
    {
      throw std::bad_alloc ();
    }

  char *container;
  gint len = epub_get_ocf_file (mEpub, "META-INF/container.xml", &container);
  if (len <= 0)
    {
      epub_close (mEpub);
      throw std::bad_alloc ();
    }

  string contentfile = container_get_contentfile (container, len);
  free (container);
  if (contentfile.empty ())
    {
      epub_close (mEpub);
      throw std::bad_alloc ();
    }

  if (content_get_media (mEpub, contentfile) == false)
    {
      epub_close (mEpub);
      throw std::bad_alloc ();
    }

  mIndex = ncx_get_index (mEpub, idSrcs["ncx"]);
}

ApvlvEPUB::~ApvlvEPUB () { epub_close (mEpub); }

bool
ApvlvEPUB::writefile (const char *filename)
{
  return false;
}

bool
ApvlvEPUB::pagesize (int page, int rot, double *x, double *y)
{
  *x = HTML_DEFAULT_WIDTH;
  *y = HTML_DEFAULT_HEIGHT;
  return true;
}

int
ApvlvEPUB::pagesum ()
{
  return int (mPages.size ());
}

bool
ApvlvEPUB::pagetext (int, gdouble, gdouble, gdouble, gdouble, char **)
{
  return false;
}

bool
ApvlvEPUB::renderweb (int pn, int ix, int iy, double zm, int rot,
                      GtkWidget *widget)
{
  webkit_web_view_set_zoom_level (WEBKIT_WEB_VIEW (widget), zm);
  string uri = mPages[pn];
  string epuburi = "apvlv:///" + uri;
  webkit_web_view_load_uri (WEBKIT_WEB_VIEW (widget), epuburi.c_str ());
  return true;
}

ApvlvPoses *
ApvlvEPUB::pagesearch (int pn, const char *str, bool reverse)
{
  return nullptr;
}

bool
ApvlvEPUB::pageselectsearch (int pn, int ix, int iy, double zm, int rot,
                             GdkPixbuf *pix, char *buffer, int sel,
                             ApvlvPoses *poses)
{
  return false;
}

ApvlvLinks *
ApvlvEPUB::getlinks (int pn)
{
  return nullptr;
}

ApvlvFileIndex *
ApvlvEPUB::new_index ()
{
  return mIndex;
}

void
ApvlvEPUB::free_index (ApvlvFileIndex *index)
{
  delete index;
}

bool
ApvlvEPUB::pageprint (int pn, cairo_t *cr)
{
  return false;
}

gchar *
ApvlvEPUB::get_ocf_file (const char *path, gssize *sizep)
{
  gchar *content = nullptr;
  *sizep = epub_get_ocf_file (mEpub, path, &content);
  return content;
}

string
ApvlvEPUB::container_get_contentfile (const char *container, int len)
{
  xmlDocPtr doc;
  xmlNodePtr node;
  string path;

  doc = xmlReadMemory (container, len, nullptr, nullptr,
                       XML_PARSE_NOBLANKS | XML_PARSE_NSCLEAN);
  if (doc == nullptr)
    {
      return path;
    }

  node = xmldoc_get_node (
      doc, "//c:container/c:rootfiles/c:rootfile/@full-path", "c",
      "urn:oasis:names:tc:opendocument:xmlns:container");
  if (node == nullptr)
    {
      xmlFreeDoc (doc);
      return path;
    }

  path = (char *)node->children->content;

  xmlFreeDoc (doc);

  return path;
}

bool
ApvlvEPUB::content_get_media (struct epub *epub, const string &contentfile)
{
  char *contentopf;
  xmlDocPtr doc;
  xmlNodeSetPtr nodeset;
  xmlNodePtr node;
  unsigned char **coverp, *cp;
  int ind, sizep;
  string cover_id = "cover";

  gint clen = epub_get_ocf_file (epub, contentfile.c_str (), &contentopf);
  if (clen <= 0)
    {
      return false;
    }

  coverp = epub_get_metadata (epub, EPUB_META, &sizep);
  if (coverp)
    {
      for (ind = 0; ind < sizep; ++ind)
        {
          cp = coverp[ind];
          auto vs = g_strsplit ((char *)cp, ":", 2);
          if (vs)
            {
              if (g_ascii_strcasecmp (vs[0], "cover") == 0)
                {
                  cover_id = vs[1];
                }
              g_strfreev (vs);
            }
          free (cp);
        }
      free (coverp);
    }

  doc = xmlReadMemory (contentopf, clen, nullptr, nullptr,
                       XML_PARSE_NOBLANKS | XML_PARSE_NSCLEAN);
  free (contentopf);
  if (doc == nullptr)
    {
      return false;
    }

  nodeset = xmldoc_get_nodeset (doc, "//c:package/c:manifest/c:item", "c",
                                "http://www.idpf.org/2007/opf");
  if (nodeset)
    {
      for (int i = 0; i < nodeset->nodeNr; ++i)
        {
          node = nodeset->nodeTab[i];
          string href = xmlnode_attr_get (node, "href");
          if (href.empty ())
            {
              continue;
            }

          if (contentfile.rfind ('/') != string::npos)
            {
              string dirname = contentfile.substr (0, contentfile.rfind ('/'));
              href = dirname + "/" + href;
            }

          string id = xmlnode_attr_get (node, "id");
          string type = xmlnode_attr_get (node, "media-type");
          if (id == cover_id)
            {
              idSrcs["cover"] = href;
              srcMimeTypes[href] = type;
            }
          else
            {
              idSrcs[id] = href;
              srcMimeTypes[href] = type;
            }
        }

      xmlXPathFreeNodeSet (nodeset);
    }

  nodeset = xmldoc_get_nodeset (doc, "//c:package/c:spine/c:itemref", "c",
                                "http://www.idpf.org/2007/opf");
  if (nodeset)
    {
      for (int i = 0; i < nodeset->nodeNr; ++i)
        {
          node = nodeset->nodeTab[i];

          string id = xmlnode_attr_get (node, "idref");
          mPages.push_back (idSrcs[id]);
          srcPages[idSrcs[id]] = mPages.size () - 1;
        }
      xmlXPathFreeNodeSet (nodeset);
    }

  xmlFreeDoc (doc);
  return true;
}

ApvlvFileIndex *
ApvlvEPUB::ncx_get_index (struct epub *epub, const string &ncxfile)
{
  ApvlvFileIndex *index = nullptr;
  xmlDocPtr doc;
  xmlNodePtr map, node;

  char *tocncx;
  gint len = epub_get_ocf_file (epub, ncxfile.c_str (), &tocncx);
  if (len <= 0)
    {
      return FALSE;
    }

  doc = xmlReadMemory (tocncx, len, nullptr, nullptr,
                       XML_PARSE_NOBLANKS | XML_PARSE_NSCLEAN);
  free (tocncx);
  if (doc == nullptr)
    {
      return index;
    }

  map = xmldoc_get_node (doc, "//c:ncx/c:navMap", "c",
                         "http://www.daisy.org/z3986/2005/ncx/");
  if (map == nullptr)
    {
      xmlFreeDoc (doc);
      return index;
    }

  index = new ApvlvFileIndex ("__cover__", 0, "", FILE_INDEX_PAGE);

  for (node = map->children; node != nullptr; node = node->next)
    {
      if (node->type != XML_ELEMENT_NODE)
        {
          continue;
        }

      ApvlvFileIndex *childindex = ncx_node_get_index (node, ncxfile);
      index->children.push_back (childindex);
    }

  xmlFreeDoc (doc);
  return index;
}

ApvlvFileIndex *
ApvlvEPUB::ncx_node_get_index (xmlNodePtr node, const string &ncxfile)
{
  auto *index = new ApvlvFileIndex ("", 0, "", FILE_INDEX_PAGE);
  xmlNodePtr child;

  for (child = node->children; child != nullptr; child = child->next)
    {
      if (g_ascii_strcasecmp ((gchar *)child->name, "navLabel") == 0)
        {
          for (xmlNodePtr ln = child->children; ln != nullptr; ++ln)
            {
              if (g_ascii_strcasecmp ((gchar *)ln->name, "text") == 0)
                {
                  index->title = string ((char *)ln->children->content);
                  break;
                }
            }
        }

      if (g_ascii_strcasecmp ((gchar *)child->name, "content") == 0)
        {
          string srcstr = xmlnode_attr_get (child, "src");
          if (srcstr.empty ())
            continue;

          if (ncxfile.find ('/') != string::npos)
            {
              auto ncxdir = g_path_get_dirname (ncxfile.c_str ());
              srcstr = string (ncxdir) + '/' + srcstr;
              g_free (ncxdir);
            }

          index->path = srcstr;

          auto href = srcstr;
          if (srcstr.find ('#') != string::npos)
            {
              index->anchor = srcstr.substr (srcstr.find ('#'));
              href = srcstr.substr (0, srcstr.find ('#'));
            }

          for (decltype (mPages.size ()) ind = 0; ind < mPages.size (); ++ind)
            {
              if (mPages[ind] == href)
                {
                  index->page = int (ind);
                  break;
                }
            }
        }

      if (g_ascii_strcasecmp ((gchar *)child->name, "navPoint") == 0)
        {
          ApvlvFileIndex *childindex = ncx_node_get_index (child, ncxfile);
          index->children.push_back (childindex);
        }
    }

  return index;
}
}

// Local Variables:
// mode: c++
// End:

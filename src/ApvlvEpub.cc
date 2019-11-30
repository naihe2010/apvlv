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

#include "ApvlvUtil.h"
#include "ApvlvView.h"
#include "ApvlvParams.h"
#include "ApvlvInfo.h"
#include "ApvlvHtm.h"
#include "ApvlvEpub.h"

#include "epub.h"

#include <gdk/gdkkeysyms.h>
#include <webkit2/webkit2.h>
#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>

namespace apvlv
{
  xmlNodeSetPtr
  xmldoc_get_nodeset (xmlDocPtr doc, const char *xpath, const char *ns)
  {
    xmlXPathContextPtr xpathctx;
    xmlXPathObjectPtr xpathobj;
    xmlNodeSetPtr nodes;

    xpathctx = xmlXPathNewContext (doc);
    if (xpathctx == NULL)
      {
        debug ("unable to create new XPath context\n");
        return NULL;
      }

    if (ns != NULL)
      {
        xmlXPathRegisterNs (xpathctx, BAD_CAST "c", BAD_CAST ns);
      }

    xpathobj = xmlXPathEvalExpression (BAD_CAST xpath, xpathctx);
    xmlXPathFreeContext (xpathctx);
    if (xpathobj == NULL)
      {
        debug ("unable to evaluate xpath expression \"%s\"\n", xpath);
        return NULL;
      }

    if (xmlXPathNodeSetIsEmpty (xpathobj->nodesetval))
      {
        debug ("unable to get \"%s\"\n", xpath);
        xmlXPathFreeObject (xpathobj);
        return NULL;
      }

    nodes = xpathobj->nodesetval;

    xmlXPathFreeNodeSetList (xpathobj);

    return nodes;
  }

  xmlNodePtr xmldoc_get_node (xmlDocPtr doc, const char *xpath, const char *ns)
  {
    xmlNodePtr node = NULL;
    xmlNodeSetPtr nodes = xmldoc_get_nodeset (doc, xpath, ns);
    if (nodes != NULL)
      {
        node = nodes->nodeTab[0];
        xmlXPathFreeNodeSet (nodes);
      }

    return node;
  }

  bool xmlnode_attr_equal (xmlNodePtr node, const char *attr, const char *value)
  {
    xmlAttrPtr prop;

    for (prop = node->properties; prop != NULL; prop = prop->next)
      {
        if (prop->type == XML_ATTRIBUTE_NODE
            && strcmp ((char *) prop->name, attr) == 0
            && strcmp ((char *) prop->children->content, value) == 0)
          {
            return true;
          }
      }

    return false;
  }

  string xmlnode_attr_get (xmlNodePtr node, const char *attr)
  {
    xmlAttrPtr prop;
    string value = "";

    for (prop = node->properties; prop != NULL; prop = prop->next)
      {
        if (prop->type == XML_ATTRIBUTE_NODE
            && strcmp ((char *) prop->name, attr) == 0)

          {
            value = (char *) prop->children->content;
            break;
          }
      }

    return value;
  }

  ApvlvEPUB::ApvlvEPUB (const char *filename, bool check):ApvlvFile (filename,
								     check)
  {
    struct epub *epub;

    epub = epub_open (filename, 0);
    if (epub == NULL)
      {
        throw std::bad_alloc ();
      }

    mRoot = g_dir_make_tmp ("apvlv_epub_XXXXXXXX", NULL);

    gchar *container;
    gint len = epub_get_ocf_file (epub, "META-INF/container.xml", &container);
    if (len <= 0)
      {
        epub_close (epub);
        throw std::bad_alloc ();
      }

    string contentfile = container_get_contentfile (container, len);
    free (container);
    if (contentfile.empty())
      {
        epub_close (epub);
        throw std::bad_alloc ();
      }

    std::map <string, string> idfiles = content_get_media (epub, contentfile);
    if (idfiles.find("ncx") == idfiles.end())
      {
        epub_close (epub);
        throw std::bad_alloc ();
      }

    if (idfiles.find("cover") != idfiles.end())
      {
        mPages[0] = string(mRoot) + "/" + idfiles["cover"];
      }

    mIndex = ncx_get_index (epub, idfiles["ncx"]);

    epub_close (epub);
  }

  ApvlvEPUB::~ApvlvEPUB ()
  {
    if (mRoot != NULL)
      {
        rmrf (mRoot);
        g_free (mRoot);
        mRoot = NULL;
      }
    
    mPages.clear();
  }

  bool ApvlvEPUB::writefile (const char *filename)
  {
    return false;
  }

  bool ApvlvEPUB::pagesize (int page, int rot, double *x, double *y)
  {
    *x = HTML_DEFAULT_WIDTH;
    *y = HTML_DEFAULT_HEIGHT;
    return true;
  }

  int ApvlvEPUB::pagesum ()
  {
    return mPages.size();
  }

  bool ApvlvEPUB::pagetext (int, int, int, int, int, char **)
  {
    return false;
  }

  bool ApvlvEPUB::renderweb (int pn, int ix, int iy, double zm, int rot, GtkWidget *widget)
  {
    webkit_web_view_set_zoom_level (WEBKIT_WEB_VIEW (widget), zm);
    string uri = mPages[pn];
    
    off_t off = uri.find('#');
    debug("view: %p", widget);
    if (off >= 0)
      {
        mAnchor = uri.substr(off + 1, uri.length() - off);
        uri = uri.substr(0, off);
      }
    else
      {
        mAnchor = "";
      }

    string path = string(mRoot) + "/" + uri;
    gchar *pathuri = g_filename_to_uri (path.c_str(), NULL, NULL);
    webkit_web_view_load_uri (WEBKIT_WEB_VIEW (widget), pathuri);
    g_free (pathuri);
    
    return true;
  }

  ApvlvPoses *ApvlvEPUB::pagesearch (int pn, const char *str, bool reverse)
  {
    return NULL;
  }

  bool ApvlvEPUB::pageselectsearch (int pn, int ix, int iy,
                                    double zm, int rot, GdkPixbuf *pix,
                                    char *buffer, int sel, ApvlvPoses *poses)
  {
    return false;
  }

  ApvlvLinks *ApvlvEPUB::getlinks (int pn)
  {
    return NULL;
  }

  ApvlvFileIndex *ApvlvEPUB::new_index ()
  {
    return mIndex;
  }

  void ApvlvEPUB::free_index (ApvlvFileIndex *index)
  {
    delete index;
  }

  bool ApvlvEPUB::pageprint (int pn, cairo_t * cr)
  {
    return false;
  }

  string ApvlvEPUB::container_get_contentfile (const char *container, int len)
  {
    xmlDocPtr doc;
    xmlNodePtr node;
    string path = "";

    doc = xmlReadMemory (container, len, NULL, NULL, XML_PARSE_NOBLANKS | XML_PARSE_NSCLEAN);
    if (doc == NULL)
      {
        return path;
      }

    node = xmldoc_get_node (doc, "//c:container/c:rootfiles/c:rootfile/@full-path", "urn:oasis:names:tc:opendocument:xmlns:container");
    if (node == NULL)
      {
        xmlFreeDoc (doc);
        return path;
      }

    path =  (char *) node->children->content;

    xmlFreeDoc (doc);

    return path;
  }

  std::map <string, string> ApvlvEPUB::content_get_media (struct epub *epub, string contentfile)
  {
    std::map <string, string> idfiles;
    char *contentopf;
    xmlDocPtr doc;
    xmlNodeSetPtr nodeset;
    xmlNodePtr node;

    gint clen = epub_get_ocf_file (epub, contentfile.c_str(), &contentopf);
    if (clen <= 0)
      {
        return idfiles;
      }

    doc = xmlReadMemory (contentopf, clen, NULL, NULL, XML_PARSE_NOBLANKS | XML_PARSE_NSCLEAN);
    free (contentopf);
    if (doc == NULL)
      {
        return idfiles;
      }

    nodeset = xmldoc_get_nodeset (doc, "//c:package/c:manifest/c:item", "http://www.idpf.org/2007/opf");
    if (nodeset)
      {
        for (int i = 0; i < nodeset->nodeNr; ++ i)
          {
            node = nodeset->nodeTab[i];
            string href = xmlnode_attr_get (node, "href");
            if (href.empty())
              {
                continue;
              }

            if (contentfile.rfind("/") != string::npos)
              {
                string dirname = contentfile.substr (0, contentfile.rfind("/"));
                href = dirname + "/" + href;
              }
            string path = href;
            char *data;
            int dlen = epub_get_ocf_file (epub, href.c_str(), &data);
            if (dlen > 0)
              {
                string localpath = string(mRoot) + "/" + path;
                gchar *dname = g_path_get_dirname (localpath.c_str());
                g_mkdir_with_parents (dname, 0755);
                g_free (dname);
                g_file_set_contents (localpath.c_str(), data, dlen, NULL);
              }

            string id = xmlnode_attr_get (node, "id");
            if (id == "ncx" || id == "cover")
              {
                idfiles[id] = href;
              }
            else if (id.compare(0, 3, "ncx") == 0)
              {
                idfiles["ncx"] = href;
              }
          }
      }

    xmlXPathFreeNodeSet (nodeset);

    xmlFreeDoc (doc);
    return idfiles;
  }

  ApvlvFileIndex * ApvlvEPUB::ncx_get_index (struct epub *epub, string ncxfile)
  {
    ApvlvFileIndex *index = NULL;
    xmlDocPtr doc;
    xmlNodePtr map, node;

    char *tocncx;
    gint len = epub_get_ocf_file (epub, ncxfile.c_str(), &tocncx);
    if (len <= 0)
      {
        return FALSE;
      }

    doc = xmlReadMemory (tocncx, len, NULL, NULL, XML_PARSE_NOBLANKS | XML_PARSE_NSCLEAN);
    free (tocncx);
    if (doc == NULL)
      {
        return index;
      }

    map = xmldoc_get_node (doc, "//c:ncx/c:navMap", "http://www.daisy.org/z3986/2005/ncx/");
    if (map == NULL)
      {
        xmlFreeDoc (doc);
        return index;
      }

    index = new ApvlvFileIndex;
    index->title = "__cover__";
    index->page = 0;
    for (node = map->children; node != NULL; node = node->next)
      {
        if (node->type != XML_ELEMENT_NODE)
          {
            continue;
          }

        ApvlvFileIndex childindex = ncx_node_get_index (node, ncxfile);
        index->children.push_back (childindex);
      }

    xmlFreeDoc (doc);
    return index;
  }

  ApvlvFileIndex ApvlvEPUB::ncx_node_get_index (xmlNodePtr node, string ncxfile)
  {
    ApvlvFileIndex index;
    xmlNodePtr child;

    string pagestr = xmlnode_attr_get (node, "playOrder");
    if (!pagestr.empty())
      {
        int page = atoi(pagestr.c_str());
        if (mPages.find(page) != mPages.end())
          {
            page ++;
          }
        index.page = page;
      }

    for (child = node->children; child != NULL; child = child->next)
      {
        if (g_ascii_strcasecmp ((gchar *) child->name, "navLabel") == 0)
          {
            for (xmlNodePtr ln = child->children; ln != NULL; ++ ln)
              {
                if (g_ascii_strcasecmp ((gchar *) ln->name, "text") == 0)
                  {
                    index.title = string ((char *) ln->children->content);
                    break;
                  }
              }
          }

        if (g_ascii_strcasecmp ((gchar *) child->name, "content") == 0)
          {
            string srcstr = xmlnode_attr_get (child, "src");
            if (!srcstr.empty())
              {
                if (ncxfile.rfind("/") != string::npos)
                  {
                    string dirname = ncxfile.substr (0, ncxfile.rfind("/"));
                    srcstr = dirname + "/" + srcstr;
                  }
                mPages[index.page] = srcstr;
              }
          }

        if (g_ascii_strcasecmp ((gchar *) child->name, "navPoint") == 0)
          {
            ApvlvFileIndex childindex = ncx_node_get_index (child, ncxfile);
            index.children.push_back (childindex);
          }
      }

    return index;
  }
}

// Local Variables:
// mode: c++
// End:

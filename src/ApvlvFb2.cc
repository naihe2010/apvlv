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
/* @CPPFILE ApvlvFb2.cc
 *
 *  Author: Alf <naihe2010@126.com>
 */
/* @date Created: 2024/03/07 11:06:35 Alf*/

#include "ApvlvFb2.h"
#include "ApvlvUtil.h"

#include <cmath>
#include <libxml/tree.h>
#include <sstream>
#include <webkit2/webkit2.h>

namespace apvlv
{
const string stylesheet_content = ".block_c {\n"
                                  "  display: block;\n"
                                  "  font-size: 2.5em;\n"
                                  "  font-weight: normal;\n"
                                  "  line-height: 33.6pt;\n"
                                  "  text-align: center;\n"
                                  "  text-indent: 0;\n"
                                  "  margin: 17pt 0;\n"
                                  "  padding: 0;\n"
                                  "}\n"
                                  ".block_ {\n"
                                  "  display: block;\n"
                                  "  font-size: 1.5em;\n"
                                  "  font-weight: normal;\n"
                                  "  line-height: 33.6pt;\n"
                                  "  text-align: justify;\n"
                                  "  text-indent: 0;\n"
                                  "  margin: 17pt 0;\n"
                                  "  padding: 0;\n"
                                  "}\n"
                                  ".block_1 {\n"
                                  "  display: block;\n"
                                  "  line-height: 1.2;\n"
                                  "  text-align: justify;\n"
                                  "  margin: 0 0 7pt;\n"
                                  "  padding: 0;\n"
                                  "}\n";
const string title_template = "<?xml version='1.0' encoding='UTF-8'?>\n"
                              "<html xmlns=\"http://www.w3.org/1999/xhtml\" "
                              "lang=\"en\" xml:lang=\"en\">\n"
                              "  <head>\n"
                              "    <title></title>\n"
                              "    <link rel=\"stylesheet\" type=\"text/css\" "
                              "href=\"stylesheet.css\"/>\n"
                              "    <meta http-equiv=\"Content-Type\" "
                              "content=\"text/html; charset=utf-8\"/>\n"
                              "  </head>\n"
                              "  <body>\n"
                              "  <br />\n"
                              "  <br />\n"
                              "  <br />\n"
                              "  <br />\n"
                              "    %s\n"
                              "  </body>\n"
                              "</html>\n";
const string section_template = "<?xml version='1.0' encoding='UTF-8'?>\n"
                                "<html xmlns=\"http://www.w3.org/1999/xhtml\" "
                                "lang=\"en\" xml:lang=\"en\">\n"
                                "  <head>\n"
                                "    <title></title>\n"
                                "    <link rel=\"stylesheet\" "
                                "type=\"text/css\" href=\"stylesheet.css\"/>\n"
                                "    <meta http-equiv=\"Content-Type\" "
                                "content=\"text/html; charset=utf-8\"/>\n"
                                "  </head>\n"
                                "  <body>\n"
                                "    %s\n"
                                "  </body>\n"
                                "</html>\n";
ApvlvFB2::ApvlvFB2 (const char *filename, bool check)
    : ApvlvFile (filename, check)
{
  gchar *content;
  gsize length;
  GError *error = nullptr;
  if (g_file_get_contents (filename, &content, &length, &error) == FALSE)
    {
      if (error != nullptr)
        {
          debug ("get file: %s centents error: %s", filename, error->message);
          g_error_free (error);
        }

      throw bad_alloc ();
    }

  parse_fb2 (content, length);
  g_free (content);
}

ApvlvFB2::~ApvlvFB2 () {}

bool
ApvlvFB2::parse_fb2 (const char *content, size_t len)
{
  auto doc = xmlReadMemory (content, (int)len, nullptr, nullptr,
                            XML_PARSE_NOBLANKS | XML_PARSE_NSCLEAN);
  if (doc == nullptr)
    {
      return false;
    }

  auto fictionbook = doc->children;

  // Parse description/binary first
  for (auto node = xmlFirstElementChild (fictionbook); node != nullptr;
       node = xmlNextElementSibling (node))
    {
      if (strcmp ((const char *)node->name, "description") == 0)
        {
          parse_description (node);
        }
      else if (strcmp ((const char *)node->name, "binary") == 0)
        {
          parse_binary (node);
        }
    }

  // Then, parse body
  for (auto node = xmlFirstElementChild (fictionbook); node != nullptr;
       node = xmlNextElementSibling (node))
    {
      if (strcmp ((const char *)node->name, "body") == 0)
        {
          parse_body (node);
        }
    }

  xmlFreeDoc (doc);

  fb2_get_index ();
  return true;
}

bool
ApvlvFB2::parse_description (xmlNodePtr node)
{
  for (auto child = xmlFirstElementChild (node); child != nullptr;
       child = xmlNextElementSibling (child))
    {
      if (strcmp ((const char *)child->name, "title-info") == 0)
        {
          for (auto c = xmlFirstElementChild (child); c != nullptr;
               c = xmlNextElementSibling (c))
            {
              if (strcmp ((const char *)c->name, "coverpage") == 0)
                {
                  mCoverHref
                      = xmlnode_attr_get (xmlFirstElementChild (c), "href");
                  return true;
                }
            }
        }
    }

  return true;
}

bool
ApvlvFB2::parse_body (xmlNodePtr node)
{
  for (auto child = xmlFirstElementChild (node); child != nullptr;
       child = xmlNextElementSibling (child))
    {
      if (strcmp ((const char *)child->name, "title") == 0)
        {
          stringstream content;
          for (auto c = xmlFirstElementChild (child); c != nullptr;
               c = xmlNextElementSibling (c))
            {
              if (strcmp ((const char *)c->name, "empty-line") == 0)
                {
                  content << "<br />";
                }
              else if (strcmp ((const char *)c->name, "p") == 0)
                {
                  content << "<h1 class=\"block_c\"><span>";
                  content << (const char *)c->children[0].content;
                  content << "</span></h1>";
                  content << "<br />";
                }
            }

          auto htmlstr = g_strdup_printf (title_template.c_str (),
                                          content.str ().c_str ());
          appendTitle (htmlstr, "application/xhtml+xml");
          g_free (htmlstr);
        }
      else if (strcmp ((const char *)child->name, "section") == 0)
        {
          stringstream content;
          string title;
          for (auto c = xmlFirstElementChild (child); c != nullptr;
               c = xmlNextElementSibling (c))
            {
              if (strcmp ((const char *)c->name, "title") == 0)
                {
                  auto titlenode = xmlLastElementChild (c);
                  title = (const char *)titlenode->children[0].content;
                  content << "<h1 class=\"block_\"><span>";
                  content << title;
                  content << "</span></h1>";
                  content << "<br />";
                }
              else if (strcmp ((const char *)c->name, "p") == 0)
                {
                  content << "<p class=\"block_1\"><span>";
                  content << (const char *)c->children[0].content;
                  content << "</span></p>";
                  content << "<br />";
                }
            }
          auto htmlstr = g_strdup_printf (section_template.c_str (),
                                          content.str ().c_str ());
          appendSection (title, htmlstr, "application/xhtml+xml");
          g_free (htmlstr);
        }
    }

  return true;
}

bool
ApvlvFB2::parse_binary (xmlNodePtr node)
{
  auto idstr = xmlnode_attr_get (node, "id");
  if (mCoverHref.empty () || idstr == mCoverHref.substr (1))
    {
      auto mimetype = xmlnode_attr_get (node, "content-type");
      gsize len;
      auto content
          = g_base64_decode ((const char *)node->children[0].content, &len);
      string section;
      section.append ((char *)content, len);
      appendCoverpage (section, mimetype);
      g_free (content);
    }
  return true;
}

void
ApvlvFB2::appendCoverpage (const string &section, const string &mime)
{
  appendSection ("__cover__", section, mime);
}

void
ApvlvFB2::appendTitle (const string &section, const string &mime)
{
  appendSection ("TITLE", section, mime);
}

void
ApvlvFB2::appendSection (const string &title, const string &section,
                         const string &mime)
{
  char uri[10];
  snprintf (uri, sizeof uri, "%zd", mPages.size ());
  appendPage (uri, title, section, mime);
}

void
ApvlvFB2::appendPage (const string &uri, const string &title,
                      const string &section, const string &mime)
{
  srcPages[uri] = (int)mPages.size ();
  mPages.push_back (uri);
  titleSections.insert ({ uri, { title, section } });
  srcMimeTypes.insert ({ uri, mime });
}

bool
ApvlvFB2::fb2_get_index ()
{
  char pagenum[16];

  mIndex = { "", 0, "", FILE_INDEX_PAGE };
  for (int ind = 0; ind < (int)mPages.size (); ++ind)
    {
      snprintf (pagenum, sizeof pagenum, "%d", ind);
      if (mPages[ind] == "__cover__")
        continue;

      auto title = titleSections[mPages[ind]].first;
      auto chap = ApvlvFileIndex (title, ind, pagenum, FILE_INDEX_PAGE);
      mIndex.children.emplace_back (chap);
    }

  return true;
}

bool
ApvlvFB2::writefile (const char *filename)
{
  return false;
}

bool
ApvlvFB2::pagesize (int pn, int rot, double *px, double *py)
{
  return false;
}

int
ApvlvFB2::pagesum ()
{
  return (int)mPages.size ();
}

bool
ApvlvFB2::pageselectsearch (int pn, int ix, int iy, double zm, int rot,
                            GdkPixbuf *pix, char *buffer, int sel,
                            ApvlvPoses *poses)
{
  return false;
}

ApvlvPoses *
ApvlvFB2::pagesearch (int pn, const char *str, bool reverse)
{
  return nullptr;
}

ApvlvLinks *
ApvlvFB2::getlinks (int pn)
{
  return nullptr;
}

bool
ApvlvFB2::pagetext (int pn, gdouble x1, gdouble y1, gdouble x2, gdouble y2,
                    char **out)
{
  return false;
}

bool
ApvlvFB2::render (int, int, int, double, int, GdkPixbuf *, char *)
{
  return false;
}

bool
ApvlvFB2::renderweb (int pn, int ix, int iy, double zm, int rot,
                     GtkWidget *widget)
{
  char uri[0x100];
  webkit_web_view_set_zoom_level (WEBKIT_WEB_VIEW (widget), zm);
  snprintf (uri, sizeof uri, "apvlv:///%d", pn);
  webkit_web_view_load_uri (WEBKIT_WEB_VIEW (widget), uri);
  return true;
}

bool
ApvlvFB2::pageprint (int pn, cairo_t *cr)
{
  return false;
}

gchar *
ApvlvFB2::get_ocf_file (const gchar *uri, gssize *lenp)
{
  if (strcmp (uri, "stylesheet.css") == 0)
    {
      *lenp = (gssize)stylesheet_content.size ();
      return g_strdup (stylesheet_content.c_str ());
    }

  *lenp = (gssize)titleSections[uri].second.size ();
  auto content = g_malloc0 (*lenp);
  memcpy (content, titleSections[uri].second.data (), *lenp);
  return (gchar *)content;
}

}

// Local Variables:
// mode: c++
// End:

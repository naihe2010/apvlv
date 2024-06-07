/*
 * This file is part of the apvlv package
 *
 * Copyright (C) 2008 Alf.
 *
 * Contact: Alf <naihe2010@126.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2.0 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */
/* @CPPFILE ApvlvUtil.cc
 *
 *  Author: Alf <naihe2010@126.com>
 */
/* @date Created: 2008/09/30 00:00:00 Alf */

#include "ApvlvUtil.h"
#include "ApvlvParams.h"

#include <glib/gstdio.h>
#include <gtk/gtk.h>

#ifndef WIN32
#include <sys/wait.h>
#endif
#ifdef WIN32
#include <windows.h>
#endif

#include <algorithm>
#include <iostream>
#include <libxml/tree.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>
#include <string>
using namespace std;

namespace apvlv
{

#ifdef WIN32
string helppdf = "~\\Startup.pdf";
string mainmenubar_glade = "~\\main_menubar.glade";
string iniexam = "~\\apvlvrc.example";
string iconreg = "~\\icons\\reg.png";
string icondir = "~\\icons\\dir.png";
string iconpdf = "~\\icons\\pdf.png";
string inifile = "~\\_apvlvrc";
string sessionfile = "~\\_apvlvinfo";
#else
string helppdf = string (DOCDIR) + "/Startup.pdf";
string mainmenubar_glade = string (DOCDIR) + "/main_menubar.glade";
string iniexam = string (DOCDIR) + "/apvlvrc.example";
string iconreg = string (PIXMAPDIR) + "/icons/reg.png";
string icondir = string (PIXMAPDIR) + "/icons/dir.png";
string iconpdf = string (PIXMAPDIR) + "/icons/pdf.png";
string inifile = "~/.apvlvrc";
string sessionfile = "~/.cache/apvlvinfo";
#endif

// Converts the path given to a absolute path.
// Warning: The string is returned a new allocated buffer, NEED TO BE g_free
char *
absolutepath (const char *path)
{
  char abpath[PATH_MAX];

  if (g_path_is_absolute (path))
    {
      return g_strdup (path);
    }

  if (*path == '~' && *(path + 1) == PATH_SEP_C)
    {
      const gchar *home;

#ifdef WIN32
      home = g_win32_get_package_installation_directory_of_module (nullptr);
#else
      home = getenv ("HOME");
      if (home == nullptr)
        {
          home = g_get_home_dir ();
        }
#endif

      if (home != nullptr)
        {
          g_snprintf (abpath, sizeof abpath, "%s%s", home, ++path);
        }
      else
        {
          debug ("Can't find home directory, use current");
          g_snprintf (abpath, sizeof abpath, "%s", path + 2);
        }
    }
  else
    {
      const gchar *pwd;

      pwd = g_get_current_dir ();
      if (pwd != nullptr)
        {
          g_snprintf (abpath, sizeof abpath, "%s/%s", pwd, path);
        }
      else
        {
          debug ("Can't find current directory, use current");
          g_snprintf (abpath, sizeof abpath, "%s", path);
        }
    }

  return g_strdup (abpath);
}

// replace a widget with a new widget
// return the parent widget
GtkWidget *
replace_widget (GtkWidget *owid, GtkWidget *nwid)
{
  GtkWidget *parent = gtk_widget_get_parent (owid);
  debug ("parent: %p, owid: %p, nwid: %p", parent, owid, nwid);
  gtk_container_remove (GTK_CONTAINER (parent), owid);
  if (GTK_IS_BOX (parent))
    {
      gtk_box_pack_start (GTK_BOX (parent), nwid, TRUE, TRUE, 0);
    }
  else
    {
      gtk_container_add (GTK_CONTAINER (parent), nwid);
    }
  gtk_widget_show_all (parent);
  return parent;
}

void
apvlv_widget_set_background (GtkWidget *wid)
{
  auto inverted = gParams->valueb ("inverted");
  auto background = gParams->values ("background");
  if (inverted && *background == '\0')
    {
      background = "black";
    }
  if (*background != '\0')
    {
#if GTK_CHECK_VERSION(3, 22, 0) and 0 /* this impl can not work now */
      gchar *cssstr = g_strdup_printf ("%s {"
                                       " background-color: %s;"
                                       "}",
                                       gtk_widget_get_name (wid), background);
      debug ("css provider: %s", cssstr);
      auto provider = gtk_css_provider_new ();
      gtk_css_provider_load_from_data (provider, cssstr, -1, NULL);
      g_free (cssstr);
      auto context = gtk_widget_get_style_context (wid);
      gtk_style_context_add_provider (context, GTK_STYLE_PROVIDER (provider),
                                      GTK_STYLE_PROVIDER_PRIORITY_USER);
#else
      GdkRGBA rgba;
      gdk_rgba_parse (&rgba, background);
      gtk_widget_override_background_color (wid, GTK_STATE_FLAG_NORMAL, &rgba);
#endif
    }
}

void
logv (const char *level, const char *file, int line, const char *func,
      const char *ms, ...)
{
  char p[0x1000], temp[0x100];
  va_list vap;

  g_snprintf (temp, sizeof temp, "[%s] %s: %d: %s(): ", level, file, line,
              func);

  va_start (vap, ms);
  vsnprintf (p, sizeof p, ms, vap);
  va_end (vap);

  cerr << temp << p << endl;
}

int
apvlv_system (const char *str)
{
#ifndef WIN32
  int ret;
  pid_t pid;
  int status;

  pid = fork ();

  ret = -1;
  if (pid < 0)
    {
      errp ("Can't fork\n");
    }
  else if (pid == 0)
    {
      gchar **argv;

      while (!isalnum (*str))
        str++;

      argv = g_strsplit_set (str, " \t", 0);
      if (argv == nullptr)
        {
          exit (1);
        }

      debug ("Exec path: (%s) argument [%d]\n", argv[0], g_strv_length (argv));
      ret = execvp (argv[0], argv);
      g_strfreev (argv);
      errp ("Exec error\n");
    }
  else
    {
      ret = wait4 (pid, &status, 0, nullptr);
    }

  return ret;
#else
  return WinExec (str, SW_NORMAL);
#endif
}

bool
apvlv_text_to_pixbuf_buffer (GString *text, int width, int height,
                             double zoomrate, unsigned char *buffer,
                             size_t buffer_size, int *o_width, int *o_height)
{
  int l_width = int (width * zoomrate);
  int l_height = int (height * zoomrate);
  int stride = cairo_format_stride_for_width (CAIRO_FORMAT_RGB24, l_width);
  g_return_val_if_fail (size_t (stride * l_height) <= buffer_size, false);

  auto surface = cairo_image_surface_create_for_data (
      buffer, CAIRO_FORMAT_RGB24, l_width, l_height, stride);
  if (surface == nullptr)
    {
      return false;
    }

  auto cr = cairo_create (surface);
  if (cr == nullptr)
    {
      cairo_surface_destroy (surface);
      return false;
    }

  /* init the background */
  cairo_set_source_rgb (cr, 0.95, 0.95, 0.95);
  cairo_rectangle (cr, 0, 0, l_width, l_height);
  cairo_fill (cr);

  cairo_select_font_face (cr, "Sans", CAIRO_FONT_SLANT_NORMAL,
                          CAIRO_FONT_WEIGHT_NORMAL);
  cairo_set_font_size (cr, 20 * zoomrate);
  cairo_set_source_rgb (cr, 0.0, 0.0, 0.0);

  auto layout = pango_cairo_create_layout (cr);

  auto wunits = pango_units_from_double (l_width);
  auto hunits = pango_units_from_double (l_height);
  pango_layout_set_width (layout, wunits);
  pango_layout_set_height (layout, hunits);
  pango_layout_set_wrap (layout, PANGO_WRAP_WORD_CHAR);
  pango_layout_set_spacing (layout, 10);
  pango_layout_set_indent (layout, 40);

  pango_layout_set_text (layout, text->str, int (text->len));

  pango_cairo_update_layout (cr, layout);
  pango_cairo_show_layout (cr, layout);

  pango_layout_get_pixel_size (layout, o_width, o_height);
  debug ("txt page size: %d:%d", *o_width, *o_height);

  cairo_surface_destroy (surface);
  cairo_destroy (cr);
  return true;
}

xmlNodeSetPtr
xmldoc_get_nodeset (xmlDocPtr doc, const char *xpath, const char *pre,
                    const char *ns)
{
  xmlXPathContextPtr xpathctx;
  xmlXPathObjectPtr xpathobj;
  xmlNodeSetPtr nodes;

  xpathctx = xmlXPathNewContext (doc);
  if (xpathctx == nullptr)
    {
      debug ("unable to create new XPath context\n");
      return nullptr;
    }

  if (ns != nullptr)
    {
      xmlXPathRegisterNs (xpathctx, BAD_CAST pre, BAD_CAST ns);
    }

  xpathobj = xmlXPathEvalExpression (BAD_CAST xpath, xpathctx);
  xmlXPathFreeContext (xpathctx);
  if (xpathobj == nullptr)
    {
      debug ("unable to evaluate xpath expression \"%s\"\n", xpath);
      return nullptr;
    }

  if (xmlXPathNodeSetIsEmpty (xpathobj->nodesetval))
    {
      debug ("unable to get \"%s\"\n", xpath);
      xmlXPathFreeObject (xpathobj);
      return nullptr;
    }

  nodes = xpathobj->nodesetval;

  xmlXPathFreeNodeSetList (xpathobj);

  return nodes;
}

xmlNodePtr
xmldoc_get_node (xmlDocPtr doc, const char *xpath, const char *pre,
                 const char *ns)
{
  xmlNodePtr node = nullptr;
  xmlNodeSetPtr nodes = xmldoc_get_nodeset (doc, xpath, pre, ns);
  if (nodes != nullptr)
    {
      node = nodes->nodeTab[0];
      xmlXPathFreeNodeSet (nodes);
    }

  return node;
}

string
xmlnode_attr_get (xmlNodePtr node, const char *attr)
{
  xmlAttrPtr prop;
  string value;

  for (prop = node->properties; prop != nullptr; prop = prop->next)
    {
      if (prop->type == XML_ATTRIBUTE_NODE
          && strcmp ((char *)prop->name, attr) == 0)
        {
          value = (char *)prop->children->content;
          break;
        }
    }

  return value;
}

string
filename_ext (const char *filename)
{
  auto pointp = strrchr (filename, '.');
  if (pointp == nullptr)
    return "";

  string ext = pointp;
  transform (ext.begin (), ext.end (), ext.begin (), ::tolower);
  return ext;
}

}

// Local Variables:
// mode: c++
// End:

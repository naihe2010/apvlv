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
/* @CPPFILE ApvlvFb2.h
 *
 *  Author: Alf <naihe2010@126.com>
 */
/* @date Created: 2024/03/07 11:05:10 Alf*/

#ifndef _APVLV_FB2_H_
#define _APVLV_FB2_H_

#include "ApvlvFile.h"
#include <libxml/tree.h>

namespace apvlv
{
class ApvlvFB2 : public ApvlvFile
{
public:
  explicit ApvlvFB2 (const char *filename, bool check = true);

  ~ApvlvFB2 () override;

  bool writefile (const char *filename) override;

  bool pagesize (int page, int rot, double *x, double *y) override;

  int pagesum () override;

  bool pagetext (int, gdouble, gdouble, gdouble, gdouble, char **) override;

  bool render (int, int, int, double, int, GdkPixbuf *, char *) override;

  bool renderweb (int pn, int ix, int iy, double zm, int rot,
                  GtkWidget *widget) override;

  bool pageselectsearch (int, int, int, double, int, GdkPixbuf *, char *, int,
                         ApvlvPoses *) override;

  ApvlvPoses *pagesearch (int pn, const char *s, bool reverse) override;

  ApvlvLinks *getlinks (int pn) override;

  ApvlvFileIndex *new_index () override;

  void free_index (ApvlvFileIndex *) override;

  bool pageprint (int pn, cairo_t *cr) override;

  gchar *get_ocf_file (const gchar *path, gssize *) override;

  DISPLAY_TYPE
  get_display_type () override { return DISPLAY_TYPE_HTML; }

private:
  map<string, pair<string, string> > titleSections;
  string mCoverHref;

  bool parse_fb2 (const char *, size_t len);
  bool parse_description (xmlNodePtr node);
  bool parse_body (xmlNodePtr node);
  bool parse_binary (xmlNodePtr node);
  void appendCoverpage (const string &section, const string &mime);
  void appendTitle (const string &section, const string &mime);
  void appendSection (const string &title, const string &section,
                      const string &mime);
  void appendPage (const string &uri, const string &title,
                   const string &section, const string &mime);
  ApvlvFileIndex *fb2_get_index ();
};

}
#endif

/* Local Variables: */
/* mode: c++ */
/* End: */

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

#include <QXmlStreamReader>

#include "ApvlvFile.h"

namespace apvlv
{
class ApvlvFB2 : public ApvlvFile
{
public:
  explicit ApvlvFB2 (const string &filename, bool check = true);

  ~ApvlvFB2 () override = default;

  bool writefile (const char *filename) override;

  bool pagesize (int page, int rot, double *x, double *y) override;

  int pagesum () override;

  bool pagetext (int, double, double, double, double, char **) override;

  bool render (int, int, int, double, int, QImage *) override;

  bool render (int pn, int ix, int iy, double zm, int rot,
               QWebEngineView *webview) override;

  unique_ptr<ApvlvPoses> pagesearch (int pn, const char *s,
                                     bool reverse) override;

  unique_ptr<ApvlvLinks> getlinks (int pn) override;

  bool pageprint (int pn, QPrinter *cr) override;

  optional<QByteArray> get_ocf_file (const string &path) override;

  DISPLAY_TYPE
  get_display_type () override { return DISPLAY_TYPE_HTML; }

private:
  map<string, pair<string, string> > titleSections;
  string mCoverHref;

  bool parse_fb2 (const char *content, size_t length);
  bool parse_description (const char *content, size_t length);
  bool parse_body (const char *content, size_t length);
  bool parse_binary (const char *content, size_t length);
  void appendCoverpage (const string &section, const string &mime);
  void appendTitle (const string &section, const string &mime);
  void appendSection (const string &title, const string &section,
                      const string &mime);
  void appendPage (const string &uri, const string &title,
                   const string &section, const string &mime);
  bool fb2_get_index ();
};

}
#endif

/* Local Variables: */
/* mode: c++ */
/* End: */

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
/* @CPPFILE ApvlvEpub.h
 *
 *  Author: Alf <naihe2010@126.com>
 */
/* @date Created: 2018/04/19 13:50:49 Alf*/

#ifndef _APVLV_EPUB_H_
#define _APVLV_EPUB_H_

#include <QXmlStreamReader>
#include <map>
#include <memory>
#include <quazip.h>

#include "ApvlvFile.h"

namespace apvlv
{
class ApvlvEPUB : public ApvlvFile
{
  FILE_TYPE_DECLARATION (ApvlvEPUB);

public:
  explicit ApvlvEPUB (const string &filename, bool check = true);
  ~ApvlvEPUB () override;

  bool writefile (const char *filename) override;

  bool pagesize (int page, int rot, double *x, double *y) override;

  int pagesum () override;

  bool pagetext (int, double, double, double, double, char **) override;

  bool render (int pn, int ix, int iy, double zm, int rot,
               ApvlvWebview *widget) override;

  unique_ptr<ApvlvPoses> pagesearch (int pn, const char *str,
                                     bool reverse) override;

  ApvlvSearchMatches searchPage (int pn, const string &text, bool is_case,
                                 bool is_reg) override;

  unique_ptr<ApvlvLinks> getlinks (int pn) override;

  bool pageprint (int pn, QPrinter *cr) override;

  optional<QByteArray> get_ocf_file (const string &path) override;

private:
  optional<QByteArray> get_zip_file_contents (const QString &name);

  static string container_get_contentfile (const char *container, int len);

  bool content_get_media (const string &contentfile);

  bool ncx_set_index (const string &ncxfile);

  void ncx_node_set_index (QXmlStreamReader *xml, const string &element_name,
                           const string &ncxfile, ApvlvFileIndex &index);

  unique_ptr<QuaZip> mQuaZip;
  std::map<string, string> idSrcs;
};

}

#endif

/* Local Variables: */
/* mode: c++ */
/* End: */

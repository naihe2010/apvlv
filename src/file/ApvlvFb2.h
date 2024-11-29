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

#ifndef _APVLV_FB2_H_
#define _APVLV_FB2_H_

#include <QXmlStreamReader>

#include "ApvlvFile.h"

namespace apvlv
{
class ApvlvFB2 : public File
{
  FILE_TYPE_DECLARATION (ApvlvFB2);

public:
  bool load (const std::string &filename) override;

  ~ApvlvFB2 () override = default;

  int sum () override;

  bool pageRenderToWebView (int pn, double zm, int rot,
                            WebView *webview) override;

  std::optional<QByteArray> pathContent (const std::string &path) override;

private:
  std::map<std::string, std::pair<std::string, std::string>> titleSections;
  std::string mCoverHref;

  bool parseFb2 (const char *content, size_t length);
  bool parseDescription (const char *content, size_t length);
  bool parseBody (const char *content, size_t length);
  bool parseBinary (const char *content, size_t length);
  void appendCoverpage (const std::string &section, const std::string &mime);
  void appendTitle (const std::string &section, const std::string &mime);
  void appendSection (const std::string &title, const std::string &section,
                      const std::string &mime);
  void appendPage (const std::string &uri, const std::string &title,
                   const std::string &section, const std::string &mime);
  bool generateIndex ();
};

}
#endif

/* Local Variables: */
/* mode: c++ */
/* End: */

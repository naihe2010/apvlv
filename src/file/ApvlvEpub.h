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

#ifndef _APVLV_EPUB_H_
#define _APVLV_EPUB_H_

#include <QXmlStreamReader>
#include <map>
#include <memory>
#include <quazip.h>
#include <string>

#include "ApvlvFile.h"

namespace apvlv
{
class ApvlvEPUB : public File
{
  FILE_TYPE_DECLARATION (ApvlvEPUB);

public:
  bool load (const std::string &filename) override;
  ~ApvlvEPUB () override;

  int sum () override;

  bool pageRenderToWebView (int pn, double zm, int rot,
                            WebView *widget) override;

  std::unique_ptr<WordListRectangle> pageSearch (int pn,
                                                 const char *s) override;

  std::optional<QByteArray> pathContent (const std::string &path) override;

private:
  std::optional<QByteArray> getZipFileContents (const QString &name);

  static std::string containerGetContentfile (const char *container, int len);

  bool contentGetMedia (const std::string &contentfile);

  bool ncxSetIndex (const std::string &ncxfile);

  void ncxNodeSetIndex (QXmlStreamReader *xml, const std::string &element_name,
                        const std::string &ncxfile, FileIndex &index);

  std::unique_ptr<QuaZip> mQuaZip;
  std::map<std::string, std::string> idSrcs;
};

}

#endif

/* Local Variables: */
/* mode: c++ */
/* End: */

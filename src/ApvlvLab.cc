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
/* @CPPFILE ApvlvLab.cc
 *
 *  Author: Alf <naihe2010@126.com>
 */

#include "ApvlvLab.h"
#include "ApvlvUtil.h"
#include "ApvlvWebViewWidget.h"

namespace apvlv
{
using namespace std;

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
bool
ApvlvLab::load (const string &filename)
{
  return false;
}

ApvlvLab::~ApvlvLab () = default;

bool
ApvlvLab::pageRenderToWebView (int pn, double zm, int rot, WebView *webview)
{
  webview->setZoomFactor (zm);
  QUrl url = QString ("apvlv:///") + QString::number (pn);
  webview->load (url);
  return true;
}

}

// Local Variables:
// mode: c++
// End:

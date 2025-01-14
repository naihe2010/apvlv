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

#include <QFile>
#include <cmath>
#include <sstream>

#include "ApvlvFb2.h"
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

FILE_TYPE_DEFINITION ("Web", ApvlvFB2, { ".fb2" });

bool
ApvlvFB2::load (const string &filename)
{
  QFile file (QString::fromLocal8Bit (filename));
  if (!file.open (QFile::ReadOnly | QFile::Text))
    {
      return false;
    }

  auto bytes = file.readAll ();
  parseFb2 (bytes.constData (), bytes.length ());
  return true;
}

bool
ApvlvFB2::parseFb2 (const char *content, size_t length)
{
  parseDescription (content, length);
  parseBinary (content, length);
  parseBody (content, length);

  generateIndex ();

  return true;
}

bool
ApvlvFB2::parseDescription (const char *content, size_t length)
{
  vector<string> keys{ "FictionBook", "description", "title-info", "coverpage",
                       "image" };
  auto value = xmlContentGetAttributeValue (content, length, keys, "href");
  mCoverHref = value;
  return true;
}

bool
ApvlvFB2::parseBody (const char *content, size_t length)
{
  vector<string> keys = { "FictionBook", "body" };
  auto optxml = xmlContentGetElement (content, length, keys);
  if (!optxml)
    return false;

  auto xml = optxml->get ();
  while (!xml->atEnd ()
         && !(xml->isEndElement () && xml->name ().toString () == "body"))
    {
      if (xml->isStartElement () && xml->name () == QString ("title"))
        {
          stringstream ss;
          while (!xml->atEnd ()
                 && !(xml->isEndElement ()
                      && xml->name ().toString () == "title"))
            {
              if (xml->isStartElement ())
                {
                  if (xml->name () == QString ("empty-line"))
                    {
                      ss << "<br />";
                    }
                  else if (xml->name () == QString ("p"))
                    {
                      ss << "<h1 class=\"block_c\"><span>";
                      auto xmltext = xml->readElementText ().trimmed ();
                      ss << xmltext.toStdString ();
                      ss << "</span></h1>";
                      ss << "<br />";
                    }
                }

              xml->readNext ();
            }

          auto htmlstr = templateBuild (title_template, "%s", ss.str ());
          appendTitle (htmlstr, "application/xhtml+xml");
        }
      else if (xml->isStartElement () && xml->name ().toString () == "section")
        {
          stringstream ss;
          string title;
          while (!xml->atEnd ()
                 && !(xml->isEndElement ()
                      && xml->name ().toString () == "section"))
            {
              if (xml->isStartElement ())
                {
                  if (xml->name ().toString () == "title")
                    {
                      auto xmltext
                          = xml->readElementText (
                                   QXmlStreamReader::IncludeChildElements)
                                .trimmed ();
                      title = xmltext.toStdString ();
                      ss << "<h1 class=\"block_\"><span>";
                      ss << title;
                      ss << "</span></h1>";
                      ss << "<br />";
                    }
                  else if (xml->name ().toString () == "p")
                    {
                      ss << "<p class=\"block_1\"><span>";
                      auto xmltext = xml->readElementText ().trimmed ();
                      ss << xmltext.toStdString ();
                      ss << "</span></p>";
                    }
                }

              xml->readNext ();
            }

          auto htmlstr = templateBuild (section_template, "%s", ss.str ());
          appendSection (title, htmlstr, "application/xhtml+xml");
        }

      xml->readNext ();
    }

  return true;
}

bool
ApvlvFB2::parseBinary (const char *content, size_t length)
{
  string idstr;
  vector<string> keys = { "FictionBook", "binary" };
  auto optxml = xmlContentGetElement (content, length, keys);
  if (!optxml)
    return false;

  auto xml = optxml->get ();
  idstr = xmlStreamGetAttributeValue (xml, "id");

  if (mCoverHref.empty () || idstr == mCoverHref.substr (1))
    {
      string mimetype = xmlStreamGetAttributeValue (xml, "content-type");
      auto contents = xml->readElementText ().toStdString ();
      QByteArray b64contents{ contents.c_str (),
                              (qsizetype)contents.length () };
      auto bytes = QByteArray::fromBase64 (b64contents);
      auto section = bytes.toStdString ();
      appendCoverpage (section, mimetype);
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
  stringstream uri;
  uri << mPages.size ();
  appendPage (uri.str (), title, section, mime);
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
ApvlvFB2::generateIndex ()
{
  stringstream pagenum;

  mIndex = { "", 0, getFilename (), FileIndexType::FILE };
  for (int ind = 0; ind < (int)mPages.size (); ++ind)
    {
      pagenum << ind;
      if (mPages[ind] == "__cover__")
        continue;

      auto title = titleSections[mPages[ind]].first;
      auto chap = FileIndex (title, ind, pagenum.str (), FileIndexType::PAGE);
      mIndex.mChildrenIndex.emplace_back (chap);
    }

  return true;
}

int
ApvlvFB2::sum ()
{
  return (int)mPages.size ();
}

bool
ApvlvFB2::pageRenderToWebView (int pn, double zm, int rot, WebView *webview)
{
  webview->setZoomFactor (zm);
  QUrl url = QString ("apvlv:///") + QString::number (pn);
  webview->load (url);
  return true;
}

optional<QByteArray>
ApvlvFB2::pathContent (const string &uri)
{
  if (uri == "stylesheet.css")
    {
      auto byte_array = QByteArray::fromStdString (stylesheet_content);
      return byte_array;
    }

  auto byte_array = QByteArray::fromStdString (titleSections[uri].second);
  return byte_array;
}

}

// Local Variables:
// mode: c++
// End:

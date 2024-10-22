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

#include <QDomDocument>
#include <QXmlStreamReader>
#include <filesystem>
#include <quazipfile.h>

#include "ApvlvEpub.h"
#include "ApvlvUtil.h"
#include "ApvlvWebViewWidget.h"

namespace apvlv
{
FILE_TYPE_DEFINITION (ApvlvEPUB, { ".epub" });

using namespace std;

bool
ApvlvEPUB::load (const string &filename)
{
  mQuaZip = make_unique<QuaZip> (QString::fromLocal8Bit (filename));
  if (mQuaZip->open (QuaZip::mdUnzip) == false)
    {
      return false;
    }

  auto filenames = mQuaZip->getFileNameList ();
  if (!filenames.contains ("META-INF/container.xml"))
    {
      return false;
    }

  auto optcontainer = getZipFileContents ("META-INF/container.xml");
  if (!optcontainer)
    {
      return false;
    }

  string contentfile = containerGetContentfile (optcontainer->constData (),
                                                optcontainer->length ());
  if (contentfile.empty ())
    {
      return false;
    }

  if (contentGetMedia (contentfile) == false)
    {
      return false;
    }

  ncxSetIndex (idSrcs["ncx"]);
  return true;
}

ApvlvEPUB::~ApvlvEPUB () { mQuaZip->close (); }

int
ApvlvEPUB::sum ()
{
  return int (mPages.size ());
}

bool
ApvlvEPUB::pageRenderToWebView (int pn, double zm, int rot, WebView *webview)
{
  webview->setZoomFactor (zm);
  QUrl epuburi = QString ("apvlv:///") + QString::fromLocal8Bit (mPages[pn]);
  webview->load (epuburi);
  return true;
}

unique_ptr<WordListRectangle>
ApvlvEPUB::pageSearch (int pn, const char *s)
{
  auto qpath = QString::fromLocal8Bit (mPages[pn]);
  auto content = getZipFileContents (qpath);
  auto html = content->toStdString ();
  auto pos = html.find (s);
  if (pos == string::npos)
    return nullptr;

  auto wordlist = make_unique<WordListRectangle> ();
  do
    {
      WordRectangle word{};
      word.word = s;
      wordlist->push_back (word);
      pos = html.find (s, pos + 1);
    }
  while (pos != string::npos);

  return wordlist;
}

optional<QByteArray>
ApvlvEPUB::pathContent (const string &path)
{
  auto optcontent = getZipFileContents (QString::fromLocal8Bit (path));
  return optcontent;
}

optional<QByteArray>
ApvlvEPUB::getZipFileContents (const QString &name)
{
  if (mQuaZip->setCurrentFile (name) == false)
    return nullopt;

  auto zipfile = make_unique<QuaZipFile> (mQuaZip.get ());
  zipfile->open (QIODevice::ReadOnly);
  auto qarray = zipfile->readAll ();
  zipfile->close ();
  return qarray;
}

string
ApvlvEPUB::containerGetContentfile (const char *container, int len)
{
  vector<string> names{ "container", "rootfiles", "rootfile" };
  return xmlContentGetAttributeValue (container, len, names, "full-path");
}

bool
ApvlvEPUB::contentGetMedia (const string &contentfile)
{
  string cover_id = "cover";

  auto optcontent = getZipFileContents (QString::fromLocal8Bit (contentfile));
  if (!optcontent)
    {
      return false;
    }

  vector<string> metas{ "package", "metadata" };
  auto optcover = xmlContentGetElement (optcontent->constData (),
                                        optcontent->length (), metas);
  if (optcover)
    {
      auto xml = optcover->get ();
      while (
          !xml->atEnd ()
          && !(xml->isEndElement () && xml->name ().toString () == "metadata"))
        {
          xml->readNext ();
          if (xml->isStartElement () && xml->name ().toString () == "meta")
            {
              auto attrs = xml->attributes ();
              for (auto const &attr : attrs)
                {
                  if (attr.name ().toString () == "name"
                      && attr.value ().toString () == "cover")
                    {
                      cover_id = xml->attributes ()
                                     .value ("content")
                                     .toString ()
                                     .toStdString ();
                    }
                }
            }
        }
    }

  vector<string> items = { "package", "manifest", "item" };
  auto optxml = xmlContentGetElement (optcontent->constData (),
                                      optcontent->length (), items);
  if (!optxml)
    return false;

  auto xml = optxml->get ();
  while (!xml->atEnd () && xml->name ().toString () == "item")
    {
      if (xml->isStartElement ())
        {
          string href = xmlStreamGetAttributeValue (xml, "href");
          if (href.empty ())
            {
              xml->readNextStartElement ();
              continue;
            }

          if (contentfile.rfind ('/') != string::npos)
            {
              string dirname = contentfile.substr (0, contentfile.rfind ('/'));
              href = dirname + "/" + href;
            }

          string id = xmlStreamGetAttributeValue (xml, "id");
          string type = xmlStreamGetAttributeValue (xml, "media-type");
          if (id == cover_id)
            {
              idSrcs["cover"] = href;
              srcMimeTypes[href] = type;
            }
          else
            {
              idSrcs[id] = href;
              srcMimeTypes[href] = type;
            }
        }

      xml->readNextStartElement ();
    }

  vector<string> names{ "package", "spine", "itemref" };
  optxml = xmlContentGetElement (optcontent->constData (),
                                 optcontent->length (), names);
  if (!optxml)
    return false;

  xml = optxml->get ();
  while (!xml->atEnd () && xml->name ().toString () == "itemref")
    {
      if (xml->isStartElement ())
        {
          string id = xmlStreamGetAttributeValue (xml, "idref");
          mPages.push_back (idSrcs[id]);
          srcPages[idSrcs[id]] = static_cast<int> (mPages.size () - 1);
        }
      xml->readNextStartElement ();
    }

  return true;
}

bool
ApvlvEPUB::ncxSetIndex (const string &ncxfile)
{
  auto opttoc = getZipFileContents (QString::fromLocal8Bit (ncxfile));
  if (!opttoc)
    {
      return false;
    }

  vector<string> names{ "ncx", "navMap" };
  auto optxml
      = xmlContentGetElement (opttoc->constData (), opttoc->length (), names);

  if (!optxml)
    {
      return false;
    }

  mIndex = { "__cover__", 0, getFilename (), FileIndexType::FILE };

  auto xml = optxml->get ();
  ncxNodeSetIndex (xml, "navMap", ncxfile, mIndex);

  return true;
}

void
ApvlvEPUB::ncxNodeSetIndex (QXmlStreamReader *xml, const string &element_name,
                            const string &ncxfile, FileIndex &index)
{
  while (!xml->atEnd ()
         && !(xml->isEndElement ()
              && xml->name ().toString ().toStdString () == element_name))
    {
      if (xml->isStartElement ())
        {
          if (xml->name ().toString () == "navLabel")
            {
              while (!(xml->isEndElement ()
                       && xml->name ().toString () == "navLabel"))
                {
                  xml->readNextStartElement ();
                  if (xml->name ().toString () == "text")
                    {
                      auto text = xml->readElementText (
                          QXmlStreamReader::ReadElementTextBehaviour::
                              SkipChildElements);
                      index.title = text.toStdString ();
                      break;
                    }
                }

              xml->readNextStartElement ();
            }

          if (xml->name ().toString () == "content")
            {
              string srcstr = xmlStreamGetAttributeValue (xml, "src");
              if (srcstr.empty ())
                continue;

              if (ncxfile.find ('/') != string::npos)
                {
                  auto ncxdir
                      = filesystem::path (ncxfile).parent_path ().string ();
                  srcstr = string (ncxdir) + '/' + srcstr;
                }

              index.path = srcstr;

              auto href = srcstr;
              if (srcstr.find ('#') != string::npos)
                {
                  index.anchor = srcstr.substr (srcstr.find ('#'));
                  href = srcstr.substr (0, srcstr.find ('#'));
                }

              for (decltype (mPages.size ()) ind = 0; ind < mPages.size ();
                   ++ind)
                {
                  if (mPages[ind] == href)
                    {
                      index.page = int (ind);
                      break;
                    }
                }

              xml->readNextStartElement ();
            }

          if (xml->name ().toString () == "navPoint")
            {
              xml->readNextStartElement ();
              auto childindex = FileIndex{};
              ncxNodeSetIndex (xml, "navPoint", ncxfile, childindex);
              index.mChildrenIndex.emplace_back (childindex);
            }
        }

      xml->readNextStartElement ();
    }
}
}

// Local Variables:
// mode: c++
// End:

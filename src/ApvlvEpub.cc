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
/* @date Created: 2018/04/19 13:51:04 Alf*/

#include <QDomDocument>
#include <QXmlStreamReader>
#include <filesystem>
#include <quazipfile.h>

#include "ApvlvEpub.h"
#include "ApvlvHtm.h"
#include "ApvlvInfo.h"
#include "ApvlvUtil.h"

namespace apvlv
{
ApvlvEPUB::ApvlvEPUB (const string &filename, bool check)
    : ApvlvFile (filename, check)
{
  mQuaZip = make_unique<QuaZip> (QString::fromStdString (filename));
  if (mQuaZip->open (QuaZip::mdUnzip) == false)
    {
      throw std::bad_alloc ();
    }

  auto filenames = mQuaZip->getFileNameList ();
  if (!filenames.contains ("META-INF/container.xml"))
    {
      throw std::bad_alloc ();
    }

  auto optcontainer = get_zip_file_contents ("META-INF/container.xml");
  if (!optcontainer)
    {
      throw std::bad_alloc ();
    }

  string contentfile = container_get_contentfile (optcontainer->constData (),
                                                  optcontainer->length ());
  if (contentfile.empty ())
    {
      throw std::bad_alloc ();
    }

  if (content_get_media (contentfile) == false)
    {
      throw std::bad_alloc ();
    }

  ncx_set_index (idSrcs["ncx"]);
}

ApvlvEPUB::~ApvlvEPUB () { mQuaZip->close (); }

bool
ApvlvEPUB::writefile (const char *filename)
{
  return false;
}

bool
ApvlvEPUB::pagesize (int page, int rot, double *x, double *y)
{
  *x = HTML_DEFAULT_WIDTH;
  *y = HTML_DEFAULT_HEIGHT;
  return true;
}

int
ApvlvEPUB::pagesum ()
{
  return int (mPages.size ());
}

bool
ApvlvEPUB::pagetext (int, double, double, double, double, char **)
{
  return false;
}

bool
ApvlvEPUB::render (int pn, int ix, int iy, double zm, int rot,
                   QWebEngineView *webview)
{
  webview->setZoomFactor (zm);
  QUrl epuburi = QString ("apvlv:///") + QString::fromStdString (mPages[pn]);
  webview->load (epuburi);
  return true;
}

unique_ptr<ApvlvPoses>
ApvlvEPUB::pagesearch (int pn, const char *str, bool reverse)
{
  return nullptr;
}

unique_ptr<ApvlvLinks>
ApvlvEPUB::getlinks (int pn)
{
  return nullptr;
}

bool
ApvlvEPUB::pageprint (int pn, QPrinter *cr)
{
  return false;
}

optional<QByteArray>
ApvlvEPUB::get_ocf_file (const string &path)
{
  auto optcontent = get_zip_file_contents (QString::fromStdString (path));
  return optcontent;
}

optional<QByteArray>
ApvlvEPUB::get_zip_file_contents (const QString &name)
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
ApvlvEPUB::container_get_contentfile (const char *container, int len)
{
  vector<string> names{ "container", "rootfiles", "rootfile" };
  return xml_content_get_attribute_value (container, len, names, "full-path");
}

bool
ApvlvEPUB::content_get_media (const string &contentfile)
{
  string cover_id = "cover";

  auto optcontent
      = get_zip_file_contents (QString::fromStdString (contentfile));
  if (!optcontent)
    {
      return false;
    }

  vector<string> metas{ "package", "metadata" };
  auto optcover = xml_content_get_element (optcontent->constData (),
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
  auto optxml = xml_content_get_element (optcontent->constData (),
                                         optcontent->length (), items);
  if (!optxml)
    return false;

  auto xml = optxml->get ();
  while (!xml->atEnd () && xml->name ().toString () == "item")
    {
      if (xml->isStartElement ())
        {
          string href = xml_stream_get_attribute_value (xml, "href");
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

          string id = xml_stream_get_attribute_value (xml, "id");
          string type = xml_stream_get_attribute_value (xml, "media-type");
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
  optxml = xml_content_get_element (optcontent->constData (),
                                    optcontent->length (), names);
  if (!optxml)
    return false;

  xml = optxml->get ();
  while (!xml->atEnd () && xml->name ().toString () == "itemref")
    {
      if (xml->isStartElement ())
        {
          string id = xml_stream_get_attribute_value (xml, "idref");
          mPages.push_back (idSrcs[id]);
          srcPages[idSrcs[id]] = static_cast<int> (mPages.size () - 1);
        }
      xml->readNextStartElement ();
    }

  return true;
}

bool
ApvlvEPUB::ncx_set_index (const string &ncxfile)
{
  auto opttoc = get_zip_file_contents (QString::fromStdString (ncxfile));
  if (!opttoc)
    {
      return false;
    }

  vector<string> names{ "ncx", "navMap" };
  auto optxml = xml_content_get_element (opttoc->constData (),
                                         opttoc->length (), names);

  if (!optxml)
    {
      return false;
    }

  mIndex = { "__cover__", 0, "", FILE_INDEX_PAGE };

  auto xml = optxml->get ();
  ncx_node_set_index (xml, "navMap", ncxfile, mIndex);

  return true;
}

void
ApvlvEPUB::ncx_node_set_index (QXmlStreamReader *xml,
                               const string &element_name,
                               const string &ncxfile, ApvlvFileIndex &index)
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
              string srcstr = xml_stream_get_attribute_value (xml, "src");
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
              auto childindex = ApvlvFileIndex{};
              ncx_node_set_index (xml, "navPoint", ncxfile, childindex);
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

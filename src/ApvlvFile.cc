/*
 * This file is part of the apvlv package
 * Copyright (C) <2008>  <Alf>
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
/* @CPPFILE File.cc
 *
 *  Author: Alf <naihe2010@126.com>
 */

#include <QBuffer>
#include <QPainter>
#include <QPrintDialog>
#include <QPrinter>
#include <algorithm>
#include <filesystem>
#include <functional>
#include <iostream>
#include <optional>
#include <sstream>
#include <utility>

#include "ApvlvFile.h"
#include "ApvlvUtil.h"
#include "ApvlvWebViewWidget.h"

namespace apvlv
{

using namespace std;

const string html_template = "<?xml version='1.0' encoding='UTF-8'?>\n"
                             "<html xmlns=\"http://www.w3.org/1999/xhtml\" "
                             "lang=\"en\" xml:lang=\"en\">\n"
                             "  <head>\n"
                             "    <title></title>\n"
                             "    <meta http-equiv=\"Content-Type\" "
                             "content=\"text/html; charset=utf-8\"/>\n"
                             "  </head>\n"
                             "  <body>\n"
                             "    <div content>"
                             "      <image src=%s />"
                             "    </div>"
                             "  </body>\n"
                             "</html>\n";

map<string, vector<string>> FileFactory::mSupportMimeTypes;
map<string, FileFactory::ExtClassList> FileFactory::mSupportClass;

const map<string, vector<string>> &
FileFactory::supportMimeTypes ()
{
  return mSupportMimeTypes;
}

vector<string>
FileFactory::supportFileExts ()
{
  unordered_set<string> extSet;
  for (const auto &pair : mSupportMimeTypes)
    {
      extSet.insert (pair.second.begin (), pair.second.end ());
    }
  vector<string> exts (extSet.begin (), extSet.end ());
  return exts;
}

ostream &
FileFactory::typeEngineDescription (ostream &os)
{
  os << "Engines: " << endl;
  for (auto &pair : mSupportClass)
    {
      string ext = pair.first;
      string engines;
      std::ranges::for_each (pair.second, [&engines] (ExtClass &cls) {
        engines.append (" ");
        engines.append (cls.first);
      });
      os << "\t" << pair.first << ":\t\t" << engines << endl;
    }
  os << endl;
  return os;
}

int
FileFactory::registerClass (const string &name, const function<File *()> &fun,
                            initializer_list<string> exts)
{
  mSupportMimeTypes.insert ({ name, exts });
  std::ranges::for_each (exts, [=] (const string &t) {
    auto iter = mSupportClass.find (t);
    if (iter != mSupportClass.end ())
      {
        auto &cls_list = iter->second;
        cls_list.emplace_back (name, fun);
      }
    else
      {
        auto cls_list = ExtClassList{ { name, fun } };
        mSupportClass.insert ({ t, cls_list });
      }
  });
  return static_cast<int> (mSupportMimeTypes.size ());
}

optional<FileFactory::ExtClass>
FileFactory::findMatchClass (const std::string &filename)
{
  auto ext = filenameExtension (filename);
  if (ext.empty ())
    return nullopt;

  if (mSupportClass.find (ext) == mSupportClass.end ())
    return nullopt;

  auto cls_list = mSupportClass[ext];
  if (cls_list.size () == 1)
    return cls_list[0];

  auto cls_name
      = ApvlvParams::instance ()->getGroupStringOrDefault (ext, "engine", "");
  if (cls_name.empty ())
    return cls_list[0];

  for (auto const &cls : cls_list)
    {
      if (strcasecmp (cls.first.c_str (), cls_name.c_str ()) == 0)
        return cls;
    }

  return cls_list[0];
}

unique_ptr<File>
FileFactory::loadFile (const string &filename)
{
  auto cls = findMatchClass (filename);
  if (!cls.has_value ())
    {
      qWarning () << "no engine to open " << filename;
      return nullptr;
    }

  auto file = unique_ptr<File> (cls->second ());
  if (file->setFilename (filename))
    return file;
  else
    {
      qWarning () << "open " << QString::fromLocal8Bit (filename) << " error";
      return nullptr;
    }
}

File::~File ()
{
  mPages.clear ();
  srcPages.clear ();
  srcMimeTypes.clear ();
}

unique_ptr<SearchFileMatch>
File::grepFile (const string &seq, bool is_case, bool is_regex,
                atomic<bool> &is_abort)
{
  vector<SearchPageMatch> page_matches;
  auto pageSum = sum ();
  for (auto pn = 0; pn < pageSum; ++pn)
    {
      if (is_abort.load () == true)
        {
          qDebug () << "grep " << QString::fromLocal8Bit (mFilename)
                    << " abort before page " << pn;
          return nullptr;
        }

      auto size = pageSizeF (pn, 0);
      string content;
      if (pageText (pn, { 0, 0, size.width, size.height }, content) == false)
        continue;

      istringstream iss{ content };
      string line;
      SearchMatchList matches;
      while (getline (iss, line))
        {
          if (is_abort.load () == true)
            {
              qDebug () << "grep " << QString::fromLocal8Bit (mFilename)
                        << " abort at page " << pn;
              return nullptr;
            }

          auto founds = apvlv::grep (line, seq, is_case, is_regex);
          for (auto const &found : founds)
            {
              SearchMatch match{ line.substr (found.first, found.second), line,
                                 found.first, found.second };
              matches.push_back (match);
            }
        }
      if (!matches.empty ())
        {
          page_matches.push_back ({ pn, matches });
        }
    }

  if (page_matches.empty ())
    return nullptr;

  auto file_match = make_unique<SearchFileMatch> ();
  file_match->filename = getFilename ();
  file_match->page_matches = std::move (page_matches);
  return file_match;
}

bool
File::pageRenderToImage (int pn, double zm, int rot, QImage *img)
{
  return false;
}

bool
File::pageRenderToWebView (int pn, double zm, int rot, WebView *webview)
{
  webview->setZoomFactor (zm);
  QUrl pdfuri = QString ("apvlv:///%1-%2-%3-%4.html")
                    .arg (pn)
                    .arg (zm)
                    .arg (rot)
                    .arg (rand ());
  webview->load (pdfuri);
  return true;
}

string
File::pathMimeType (const string &path)
{
  if (srcMimeTypes.find (path) != srcMimeTypes.end ())
    return srcMimeTypes[path];
  else if (QString::fromLocal8Bit (path).endsWith (".png"))
    return "image/png";
  else
    return "text/html";
}

int
File::pathPageNumber (const string &path)
{
  if (srcPages.find (path) != srcPages.end ())
    return srcPages[path];
  return -1;
}

optional<QByteArray>
File::pathContent (const string &path)
{
  auto words = QString::fromLocal8Bit (path).split ("-");
  int pn = words[0].toInt ();
  double zm = words[1].toDouble ();
  int rot = words[2].toInt ();

  if (QString::fromLocal8Bit (path).endsWith (".html"))
    return pathContentHtml (pn, zm, rot);
  else
    return pathContentPng (pn, zm, rot);
}

optional<QByteArray>
File::pathContentHtml (int pn, double zm, int rot)
{
  auto src = QString ("apvlv:///%1-%2-%3-%4.png")
                 .arg (pn)
                 .arg (zm)
                 .arg (rot)
                 .arg (rand ());
  auto html = QString::asprintf (html_template.c_str (),
                                 src.toStdString ().c_str ());
  return QByteArray::fromStdString (html.toStdString ());
}

optional<QByteArray>
File::pathContentPng (int pn, double zm, int rot)
{
  QImage image;
  if (pageRenderToImage (pn, zm, rot, &image) == false)
    return nullopt;

  QByteArray array;
  QBuffer buffer (&array);
  buffer.open (QIODevice::WriteOnly);
  image.save (&buffer, "PNG");
  buffer.close ();
  return array;
}

bool
File::print (int page)
{
  QPrinter printer;
  QPrintDialog dialog(&printer);
  if (dialog.exec() != QDialog::Accepted) return false;
  
  QPainter painter(&printer);
  int startPage = (page >= 0) ? page : 0;
  int endPage = (page >= 0) ? page : sum() - 1;
  
  for (int pn = startPage; pn <= endPage; ++pn) {
    if (pn > startPage) printer.newPage();
    
    QImage image;
    if (pageRenderToImage(pn, 1.0, 0, &image)) {
      QRect rect = painter.viewport();
      QSize size = image.size();
      size.scale(rect.size(), Qt::KeepAspectRatio);
      painter.drawImage(0, 0, image.scaled(size));
    }
  }
  return true;
}

}

// Local Variables:
// mode: c++
// End:

/*
 * This file is part of the apvlv package
 *
 * Copyright (C) 2008 Alf.
 *
 * Contact: Alf <naihe2010@126.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2.0 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */
/* @CPPFILE ApvlvUtil.cc
 *
 *  Author: Alf <naihe2010@126.com>
 */

#include <QByteArray>
#include <QCoreApplication>
#include <QDir>
#include <QProcessEnvironment>
#include <QXmlStreamReader>
#include <memory>
#include <optional>
#include <string>

#include "ApvlvUtil.h"

namespace apvlv
{

string helppdf;
string iniexam;
string icondir;
string iconfile;
string iconpage;
string translations;

string inifile;
string sessionfile;
string logfile;

static void
get_xdg_or_home_ini (const QString &appdir)
{
  auto sysenv = QProcessEnvironment::systemEnvironment ();
  auto xdgdir = sysenv.value ("XDG_CONFIG_DIR").toStdString ();
  auto homedir = sysenv.value ("HOME").toStdString ();

  if (homedir.empty ())
    {
      homedir = QDir::homePath ().toStdString ();
    }

  inifile = appdir.toStdString () + "/.apvlvrc";

  if (!xdgdir.empty ())
    {
      inifile = xdgdir + "/apvlv/apvlvrc";
    }
  else if (!homedir.empty ())
    {
      inifile = homedir + "/.config/apvlv/apvlvrc";
      if (!filesystem::is_regular_file (inifile))
        {
          inifile = homedir + "/.apvlvrc";
        }
    }
}

static void
get_xdg_or_cache_sessionpath (const QString &appdir)
{
  auto sysenv = QProcessEnvironment::systemEnvironment ();
  auto xdgdir = sysenv.value ("XDG_CACHE_HOME").toStdString ();
  auto homedir = sysenv.value ("HOME").toStdString ();
  if (homedir.empty ())
    {
      homedir = QDir::homePath ().toStdString ();
    }

  sessionfile = appdir.toStdString () + "/apvlvinfo";
  if (!xdgdir.empty ())
    {
      sessionfile = xdgdir + "/apvlvinfo";
    }
  else if (!homedir.empty ())
    {
      sessionfile = homedir + "/.cache/apvlvinfo";
    }
}

void
getRuntimePaths ()
{
  auto dirpath = QDir (QCoreApplication::applicationDirPath ());
  dirpath.cdUp ();
  auto prefix = dirpath.path ().toStdString ();
  auto apvlvdir = prefix + "/share/doc/apvlv";

  helppdf = apvlvdir + "/Startup.pdf";
  icondir = apvlvdir + "/icons/dir.png";
  iconfile = apvlvdir + "/icons/pdf.png";
  iconpage = apvlvdir + "/icons/reg.png";
  translations = apvlvdir + "/translations";

#ifndef WIN32
  iniexam = string (SYSCONFDIR) + "/apvlvrc.example";
#else
  iniexam = apvlvdir + "/apvlvrc.example";
#endif

  get_xdg_or_home_ini (dirpath.path ());
  get_xdg_or_cache_sessionpath (dirpath.path ());
}

optional<unique_ptr<QXmlStreamReader> >
xml_content_get_element (const char *content, size_t length,
                         const vector<string> &names)
{
  auto bytes = QByteArray{ content, (qsizetype)length };
  auto xml = make_unique<QXmlStreamReader> (bytes);
  ptrdiff_t state = 0;
  while (!xml->atEnd ())
    {
      if (xml->isStartElement ())
        {
          auto name = xml->name ().toString ().toStdString ();
          // qDebug ("xml element name: %s", name.c_str ());
          auto iter = find (names.begin (), names.end (), name);
          if (iter == names.end ())
            {
              xml->readNextStartElement ();
              continue;
            }

          auto pos = distance (names.begin (), iter);
          if (state == pos)
            {
              state++;
            }

          if (state == static_cast<ptrdiff_t> (names.size ()))
            {
              return xml;
            }
        }

      xml->readNextStartElement ();
    }

  return nullopt;
}

string
xml_stream_get_attribute_value (QXmlStreamReader *xml, const string &key)
{
  auto attrs = xml->attributes ();
  for (auto &attr : attrs)
    {
      if (attr.name ().toString ().toStdString () == key)
        return attr.value ().toString ().toStdString ();
    }

  return "";
}

string
xml_content_get_attribute_value (const char *content, size_t length,
                                 const vector<string> &names,
                                 const string &key)
{
  auto optxml = xml_content_get_element (content, length, names);
  if (!optxml)
    return "";

  auto xml = optxml->get ();
  return xml_stream_get_attribute_value (xml, key);
}

string
filename_ext (const string &filename)
{
  auto pointp = filename.rfind ('.');
  if (pointp == string::npos)
    return "";

  string ext = filename.substr (pointp);
  transform (ext.begin (), ext.end (), ext.begin (), ::tolower);
  return ext;
}

}

// Local Variables:
// mode: c++
// End:

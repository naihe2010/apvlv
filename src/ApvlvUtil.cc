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
#include <QRegularExpression>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>

#include "ApvlvUtil.h"

namespace apvlv
{

using namespace std;

string HelpPdf;
string IniExam;
string IconDir;
string IconFile;
string IconPage;
string Translations;

string IniFile;
string SessionFile;
string LogFile;
string NotesDir;

static void
getXdgOrHomeIni (const QString &appdir)
{
  auto sysenv = QProcessEnvironment::systemEnvironment ();
  auto xdgdir = sysenv.value ("XDG_CONFIG_DIR").toStdString ();
  auto homedir = sysenv.value ("HOME").toStdString ();

  if (homedir.empty ())
    {
      homedir = QDir::homePath ().toStdString ();
    }

  IniFile = appdir.toStdString () + "/.apvlvrc";

  if (!xdgdir.empty ())
    {
      IniFile = xdgdir + "/apvlv/apvlvrc";
    }
  else if (!homedir.empty ())
    {
      IniFile = homedir + "/.config/apvlv/apvlvrc";
      if (!filesystem::is_regular_file (IniFile))
        {
          IniFile = homedir + "/.apvlvrc";
        }
    }
}

static void
getXdgOrCachePath (const QString &appdir)
{
  auto sysenv = QProcessEnvironment::systemEnvironment ();
  auto xdgdir = sysenv.value ("XDG_CACHE_HOME").toStdString ();
  auto homedir = sysenv.value ("HOME").toStdString ();
  if (homedir.empty ())
    {
      homedir = QDir::homePath ().toStdString ();
    }

  SessionFile = appdir.toStdString () + "/apvlvinfo";
  if (!xdgdir.empty ())
    {
      SessionFile = xdgdir + "/apvlvinfo";
    }
  else if (!homedir.empty ())
    {
      SessionFile = homedir + "/.cache/apvlvinfo";
    }
}

void
getRuntimePaths ()
{
  auto dirpath = QDir (QCoreApplication::applicationDirPath ());
  dirpath.cdUp ();
  auto prefix = dirpath.path ().toStdString ();
  auto apvlvdir = prefix + "/share/doc/apvlv";

  HelpPdf = apvlvdir + "/Startup.pdf";
  IconDir = apvlvdir + "/icons/dir.png";
  IconFile = apvlvdir + "/icons/pdf.png";
  IconPage = apvlvdir + "/icons/reg.png";
  Translations = apvlvdir + "/translations";

#ifndef WIN32
  IniExam = string (SYSCONFDIR) + "/apvlvrc.example";
#else
  IniExam = apvlvdir + "/apvlvrc.example";
#endif

  getXdgOrHomeIni (dirpath.path ());
  getXdgOrCachePath (dirpath.path ());

  auto homedir = QDir::homePath ().toStdString ();
  NotesDir = homedir + "/" + "ApvlvNotes";
}

optional<unique_ptr<QXmlStreamReader>>
xmlContentGetElement (const char *content, size_t length,
                      const vector<string> &names)
{
  auto bytes = QByteArray{ content, (qsizetype)length };
  auto xml = make_unique<QXmlStreamReader> (bytes);
  std::ptrdiff_t state = 0;
  while (!xml->atEnd ())
    {
      if (xml->isStartElement ())
        {
          auto name = xml->name ().toString ().toStdString ();
          auto iter = std::ranges::find (names, name);
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
xmlStreamGetAttributeValue (QXmlStreamReader *xml, const string &key)
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
xmlContentGetAttributeValue (const char *content, size_t length,
                             const vector<string> &names, const string &key)
{
  auto optxml = xmlContentGetElement (content, length, names);
  if (!optxml)
    return "";

  auto xml = optxml->get ();
  return xmlStreamGetAttributeValue (xml, key);
}

string
filenameExtension (const string &filename)
{
  auto pointp = filename.rfind ('.');
  if (pointp == string::npos)
    return "";

  string ext = filename.substr (pointp);
  std::ranges::transform (ext, ext.begin (), ::tolower);
  return ext;
}

void
imageArgb32ToRgb32 (QImage &image, int left, int top, int right, int bottom)
{
  for (auto x = left; x < right; ++x)
    {
      for (auto y = top; y < bottom; ++y)
        {
          auto c = image.pixelColor (x, y);
          double ra = double (c.alpha ()) / 255.0;
          auto nr = static_cast<int> (c.red () * ra + 255 * (1.0 - ra));
          auto ng = static_cast<int> (c.green () * ra + 255 * (1.0 - ra));
          auto nb = static_cast<int> (c.blue () * ra + 255 * (1.0 - ra));
          auto pc = QColor::fromRgb (nr, ng, nb, 255);
          image.setPixelColor (x, y, pc);
        }
    }
}

string
templateBuild (string_view temp, string_view token, string_view real)
{
  auto pos = temp.find (token);
  if (pos == string::npos)
    return string (temp);

  auto first = temp.substr (0, pos);
  auto second = temp.substr (pos + token.length ());
  auto outstr = string (first);
  outstr += string (real);
  outstr += string (second);
  return outstr;
}

qint64
parseFormattedDataSize (const QString &sizeStr)
{
  QString cleanStr = sizeStr.simplified ().toUpper ();

  QRegularExpression re ("^(\\d+(?:\\.\\d+)?)(\\s*)(B?|KB?|MB?|GB?|TB?)$");
  QRegularExpressionMatch match = re.match (cleanStr);

  if (match.hasMatch ())
    {
      double number = match.captured (1).toDouble ();
      QString unit = match.captured (3);
      if (unit.isEmpty ())
        return static_cast<qint64> (number);

      switch (unit[0].unicode ())
        {
        case 'T':
          return static_cast<qint64> (number * 1024 * 1024 * 1024 * 1024);
        case 'G':
          return static_cast<qint64> (number * 1024 * 1024 * 1024);
        case 'M':
          return static_cast<qint64> (number * 1024 * 1024);
        case 'K':
          return static_cast<qint64> (number * 1024);
        default:
          return static_cast<qint64> (number);
        }
    }

  return -1;
}

qint64
filesystemTimeToMSeconds (std::filesystem::file_time_type ftt)
{
  auto epoch = ftt.time_since_epoch () + chrono::seconds{ 6437664000 };
  auto milliseconds = chrono::duration_cast<chrono::seconds> (epoch);
  return static_cast<qint64> (milliseconds.count ());
}

}

// Local Variables:
// mode: c++
// End:

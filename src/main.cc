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
/*@CPPFILE main.cpp Apvlv start at here
 *
 *  Author: Alf <naihe2010@126.com>
 */

#include <filesystem>

#include <QApplication>
#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QDir>
#include <QLocale>
#include <QMessageBox>
#include <QTranslator>
#include <QWebEngineUrlScheme>

#include "ApvlvInfo.h"
#include "ApvlvLog.h"
#include "ApvlvParams.h"
#include "ApvlvUtil.h"
#include "ApvlvView.h"

using namespace std;
using namespace apvlv;

#if defined WIN32 && defined NDEBUG
#pragma comment(linker, "/subsystem:windows")
#pragma comment(linker, "/ENTRY:mainCRTStartup")
#endif

static void
registerUrlScheme ()
{
  QWebEngineUrlScheme scheme ("apvlv");
  scheme.setSyntax (QWebEngineUrlScheme::Syntax::Path);
  QWebEngineUrlScheme::registerScheme (scheme);
}

static void
usage_exit ()
{
  fprintf (stdout,
           "%s [options] paths\n"
           "\n"
           "Options: \n"
           "%s\n"
           "\n"
           "Arguments: \n"
           "%s\n"
           "\n"
           "Please send bug report to %s\n",
           PACKAGE_NAME,
           "\t-h                display this and exit\n"
           "\t-v                display version info and exit\n"
           "\t-c [file]         set user configuration file\n",
           "\t paths            document path list", PACKAGE_BUGREPORT);
  exit (0);
}

static void
version_exit ()
{
  fprintf (stdout,
           "%s %s-%s\n"
           "Please send bug report to %s\n"
           "\n",
           PACKAGE_NAME, PACKAGE_VERSION, RELEASE, PACKAGE_BUGREPORT);
  exit (0);
}

static list<string>
parseCommandLine (const QCoreApplication &app)
{
  QCommandLineParser parser;

  auto versionOption = QCommandLineOption (QStringList () << "v" << "version",
                                           QObject::tr ("version number"));
  parser.addOption (versionOption);

  auto helpOption = QCommandLineOption (QStringList () << "h" << "help",
                                        QObject::tr ("help information"));
  parser.addOption (helpOption);

  auto configFileOption
      = QCommandLineOption (QStringList () << "c" << "config-file",
                            QObject::tr ("config file"), "config");
  parser.addOption (configFileOption);

  auto logFileOption = QCommandLineOption (QStringList () << "l" << "log-file",
                                           QObject::tr ("log file"), "log");
  parser.addOption (logFileOption);

  parser.addPositionalArgument ("path", QObject::tr ("document path"));

  parser.process (app);

  if (parser.isSet (helpOption))
    {
      usage_exit ();
    }
  if (parser.isSet (versionOption))
    {
      version_exit ();
    }
  if (parser.isSet (configFileOption))
    {
      auto value = parser.value (configFileOption);
      inifile = filesystem::absolute (value.toStdString ()).string ();
    }
  if (parser.isSet (logFileOption))
    {
      auto value = parser.value (logFileOption);
      logfile = filesystem::absolute (value.toStdString ()).string ();
    }

  /*
   * load the global sys conf file
   * */
  auto sysIni = string (SYSCONFDIR) + "/apvlvrc";
  auto params = ApvlvParams::instance ();
  params->loadFile (sysIni);

  /*
   * load the user conf file
   * */
  qDebug () << "using config: " << inifile;
  ApvlvParams::instance ()->loadFile (inifile);

  list<string> paths;
  auto pathlist = parser.positionalArguments ();
  for (const auto &path : pathlist)
    {
      paths.emplace_back (path.toStdString ());
    }

  return paths;
}

static void
loadTranslator (QTranslator &translator)
{
  map<string, string> lanuage_translator{ { "Chinese", "zh_CN" } };
  auto lan = QLocale::system ().language ();
  auto lanstr = QLocale::languageToString (lan).toStdString ();
  if (lanuage_translator.find (lanstr) != lanuage_translator.end ())
    {
      auto lantrans = lanuage_translator[lanstr];
      if (!translator.load (QString::fromLocal8Bit (lantrans),
                            QString::fromLocal8Bit (translations)))
        {
          qCritical ("Load i18n file failed, using English");
        }
      else
        {
          QCoreApplication::installTranslator (&translator);
        }
    }
}

int
main (int argc, char *argv[])
{
  registerUrlScheme ();

  QApplication app (argc, argv);

  getRuntimePaths ();

  QTranslator translator;
  loadTranslator (translator);

  ApvlvInfo::instance ()->loadFile (sessionfile);

  auto paths = parseCommandLine (app);

  ApvlvLog sLog (QString::fromLocal8Bit (logfile));

  string path = helppdf;
  if (!paths.empty ())
    {
      path = paths.front ();
      paths.pop_front ();
    }
  if (!filesystem::is_regular_file (path))
    {
      qFatal ("File '%s' is not readable.\n", path.c_str ());
      return 1;
    }

  ApvlvView sView (nullptr);
  if (!sView.newtab (path))
    {
      exit (1);
    }

  while (!paths.empty ())
    {
      path = paths.front ();
      paths.pop_front ();
      auto apath = filesystem::absolute (path).string ();
      if (!sView.newtab (apath))
        {
          qCritical ("Can't open document: %s", apath.c_str ());
        }
    }

  QApplication::exec ();

  return 0;
}

// Local Variables:
// mode: c++
// End:

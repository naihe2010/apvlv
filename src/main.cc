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
/* @date Created: 2008/09/30 00:00:00 Alf */

#ifndef WIN32
#include <getopt.h>
#endif
#include <QApplication>
#include <QDir>
#include <QFile>
#include <QLocale>
#include <QMessageBox>
#include <QTranslator>
#include <QWebEngineUrlScheme>

#include "ApvlvInfo.h"
#include "ApvlvLog.h"
#include "ApvlvParams.h"
#include "ApvlvUtil.h"
#include "ApvlvView.h"

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
  // scheme.setFlags (QWebEngineUrlScheme::SecureScheme);
  QWebEngineUrlScheme::registerScheme (scheme);
}

#ifndef WIN32
static void
usage_exit ()
{
  fprintf (stdout,
           "%s Usage:\n"
           "%s\n"
           "Please send bug report to %s\n",
           PACKAGE_NAME,
           "\t-h                display this and exit\n"
           "\t-v                display version info and exit\n"
           "\t-c [file]         set user configuration file\n",
           PACKAGE_BUGREPORT);
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
#endif

static int
parse_options (int argc, char *argv[])
{
#ifdef WIN32
  gParams->loadfile (inifile);
  return 1;
#else
  int c, index;
  static struct option long_options[]
      = { { "config", required_argument, nullptr, 'c' },
          { "help", no_argument, nullptr, 'h' },
          { "log-file", required_argument, nullptr, 'l' },
          { "version", no_argument, nullptr, 'v' },
          { nullptr, 0, nullptr, 0 } };

  index = 0;
  while ((c = getopt_long (argc, argv, "c:hl:v", long_options, &index)) != -1)
    {
      switch (c)
        {
        case 'c':
          inifile = filesystem::absolute (optarg).string ();
          break;

        case 'l':
          logfile = filesystem::absolute (optarg).string ();
          break;

        case 'h':
          usage_exit ();
          return -1;

        case 'v':
          version_exit ();
          return -1;

        default:
          qCritical ("no command line options");
          return -1;
        }
    }

  qDebug ("using config: %s", inifile.c_str ());

  /*
   * load the global sys conf file
   * */
  auto sysIni = string (SYSCONFDIR) + "/apvlvrc";
  gParams->loadfile (sysIni);

  /*
   * load the user conf file
   * */
  gParams->loadfile (inifile);

  return optind;
#endif
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
      if (!translator.load (QString::fromStdString (lantrans),
                            QString::fromStdString (translations)))
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

  ApvlvParams sParams;
  gParams = &sParams;

  ApvlvInfo sInfo (sessionfile);
  gInfo = &sInfo;

  int opt = parse_options (argc, argv);
  if (opt < 0)
    {
      qCritical ("Parse options failed.\n");
      return 1;
    }

  ApvlvLog sLog (QString::fromStdString (logfile));

  string path;
  if (opt > 0 && argc > opt)
    {
      path = argv[opt];
      opt++;
    }
  else
    {
      path = helppdf;
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

  while (opt < argc)
    {
      auto apath = filesystem::absolute (argv[opt++]).string ();
      if (!sView.loadfile (apath))
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

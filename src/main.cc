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

#include "ApvlvView.h"
#include "ApvlvCmds.h"
#include "ApvlvParams.h"
#include "ApvlvInfo.h"
#include "ApvlvUtil.h"

#include <locale.h>
#ifndef WIN32
#include <getopt.h>
#endif
#include <gtk/gtk.h>
#include <iostream>

using namespace apvlv;

#if defined WIN32 && defined NDEBUG
#pragma comment (linker, "/subsystem:windows")
#pragma comment (linker, "/ENTRY:mainCRTStartup")
#endif

#ifndef WIN32
static void
usage_exit ()
{
  fprintf (stdout, "%s Usage:\n"
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
  fprintf (stdout, "%s %s-%s\n"
           "Please send bug report to %s\n"
           "\n", PACKAGE_NAME, PACKAGE_VERSION, RELEASE, PACKAGE_BUGREPORT);
  exit (0);
}
#endif

static int
parse_options (int argc, char *argv[])
{
  gchar *ini;

#ifdef WIN32
  ini = absolutepath (inifile.c_str ());
  if (ini != NULL)
    {
      gParams->loadfile (ini);
      g_free (ini);
    }
  return 1;
#else
  int c, index;
  static struct option long_options[] =
  {
    {"config", required_argument, NULL, 'c'},
    {"help", no_argument, NULL, 'h'},
    {"version", no_argument, NULL, 'v'},
    {0, 0, 0, 0}
  };

  index = 0;
  ini = NULL;
  while ((c = getopt_long (argc, argv, "c:hv", long_options, &index)) != -1)
    {
      switch (c)
        {
        case 'c':
          ini = absolutepath (optarg);
          break;

        case 'h':
          usage_exit ();
          return -1;

        case 'v':
          version_exit ();
          return -1;

        default:
          errp ("no command line options");
          return -1;
        }
    }

  if (ini == NULL)
    {
      ini = absolutepath (inifile.c_str ());
    }

  /*
   * load the global sys conf file
   * */
  gchar *sysini = g_strdup_printf ("%s/%s", SYSCONFDIR, "apvlvrc");
  if (sysini)
    {
      gParams->loadfile (sysini);
      g_free (sysini);
    }

  /*
   * load the user conf file
   * */
  if (ini != NULL)
    {
      gParams->loadfile (ini);
      g_free (ini);
    }

  return optind;
#endif
}

int
main (int argc, char *argv[])
{
  setlocale (LC_ALL, "");

  ApvlvCmds sCmds;
  gCmds = &sCmds;

  ApvlvParams sParams;
  gParams = &sParams;

  gchar *infopath = absolutepath (sessionfile.c_str ());
  ApvlvInfo sInfo (infopath);
  g_free (infopath);
  gInfo = &sInfo;

#ifdef _WIN32
  gchar *temp = absolutepath (iconpdf.c_str ());
  iconpdf = temp;
  g_free (temp);
  temp = absolutepath (icondir.c_str ());
  icondir = temp;
  g_free (temp);
  temp = absolutepath (iconreg.c_str ());
  iconreg = temp;
  g_free (temp);
#endif

  int opt = parse_options (argc, argv);
  if (opt < 0)
    {
      errp ("Parse options failed.\n");
      return 1;
    }

  gchar *path;
  bool ishelppdf = false;
  if (opt > 0 && argc > opt)
    {
      path = argv[opt];
      opt++;
    }
  else
    {
      path = (gchar *) helppdf.c_str ();
      ishelppdf = true;
    }

  gchar *rpath = g_locale_to_utf8 (path, -1, NULL, NULL, NULL);
  if (rpath == NULL)
    {
      errp ("Convert path: '%s' to utf8 failed.\n", path);
      return 1;
    }

  path = absolutepath (rpath);
  g_free (rpath);
  if (path == NULL)
    {
      errp ("Convert '%s' to absolute path failed.\n", rpath);
      return 1;
    }

  if (g_file_test (path, G_FILE_TEST_IS_REGULAR) == FALSE)
    {
      errp ("File '%s' is not readble.\n", path);
      g_free (path);
      return 1;
    }

  gtk_init (&argc, &argv);

  ApvlvView sView (path);
  if (ishelppdf)
    {
      helppdf = path;
    }
  g_free (path);

  gView = &sView;

  while (opt < argc)
    {
      path = absolutepath (argv[opt++]);
      if (path == NULL)
        {
          continue;
        }

      if (gView->loadfile (path) == false)
        {
          errp ("Can't open document: %s", path);
        }

      g_free (path);
    }

  gtk_main ();

  return 0;
}

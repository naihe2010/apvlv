/*
* This file is part of the apvlv package
*
* Copyright (C) 2008 Alf.
*
* Contact: Alf <naihe2010@gmail.com>
*
* This library is free software; you can redistribute it and/or
* modify it under the terms of the GNU Lesser General Public License
* as published by the Free Software Foundation; either version 2.1 of
* the License, or (at your option) any later version.
*
* This library is distributed in the hope that it will be useful, but
* WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
* Lesser General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public
* License along with this library; if not, write to the Free Software
* Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
* 02110-1301 USA
*
*/
/*@CPPFILE main.cpp Apvlv start at this.
*
*  Author: Alf <naihe2010@gmail.com>
*/
/* @date Created: 2008/09/30 00:00:00 Alf */

#include "ApvlvView.hpp"
#include "ApvlvCmds.hpp"
#include "ApvlvParams.hpp"
#include "ApvlvUtil.hpp"

#include <iostream>

#include <locale.h>
#include <gtk/gtk.h>

using namespace apvlv;

#if defined WIN32 && defined NDEBUG
#pragma comment (linker, "/subsystem:windows")
#pragma comment (linker, "/ENTRY:mainCRTStartup")
#endif

int
main (int argc, char *argv[])
{
  setlocale (LC_ALL, "");

  ApvlvCmds sCmds;
  gCmds = &sCmds;

  ApvlvParams sParams;
  gParams = &sParams;

  gchar *ini = absolutepath (inifile.c_str ());
  if (ini != NULL)
  {
    gboolean exist = g_file_test (ini, G_FILE_TEST_IS_REGULAR);
    if (!exist)
      {
        filecpy (inifile.c_str (), iniexam.c_str ());
      }
    else
      {
        gParams->loadfile (ini);
      }
    g_free (ini);
  }
  //  param.debug ();


  ApvlvView sView (argc, argv);
  gView = &sView;

  gchar *path;
  if (argc > 1)
    {
      for (unsigned int i=argc-1; i>0; --i)
        {
          path = absolutepath (argv[i]);
          gView->loadfile (path);
          g_free (path);
        }
    }
  else
    {
      path = absolutepath (helppdf.c_str ());
      gView->loadfile (path);
      g_free (path);
    }

  gView->show ();

  return 0;
}

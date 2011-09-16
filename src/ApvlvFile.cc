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
/* @CFILE ApvlvFile.cpp xxxxxxxxxxxxxxxxxxxxxxxxxx.
*
*  Author: Alf <naihe2010@126.com>
*/
/* @date Created: 2009/11/20 19:38:30 Alf*/

#include "ApvlvFile.h"
#include "ApvlvPdf.h"
#include "ApvlvUtil.h"
#include "ApvlvView.h"

#ifdef APVLV_WITH_UMD
 #include "ApvlvUmd.h"
#endif
#ifdef APVLV_WITH_DJVU
 #include "ApvlvDjvu.h"
#endif
#ifdef APVLV_WITH_HTML
 #include "ApvlvHtm.h"
#endif

#include <glib.h>

#include <sys/stat.h>
#include <iostream>
#include <fstream>

namespace apvlv
{
#ifndef MAX
#define MAX(a,b)        ((a) > (b) ? (a) : (b))
#endif

ApvlvFile::ApvlvFile (const char *filename, bool check)
{
  mIndex = NULL;

  mRawdata = NULL;
  mRawdataSize = 0;
}

ApvlvFile::~ApvlvFile ()
{
  if (mRawdata != NULL)
    {
      delete[]mRawdata;
      mRawdata = NULL;
    }
}

ApvlvFile *ApvlvFile::newfile (const char *filename, bool check)
{
  ApvlvFile *file = NULL;
  static const char *type_phrase[] =
  {
    ".PDF",
    ".umd",
    ".djvu"
  };

  size_t i;
  for (i = 0; i < 3; ++ i)
    {
      if (strcasecmp (filename + strlen (filename) - strlen (type_phrase[i]),
                      type_phrase[i]) == 0)
        {
          break;
        }
    }

  if (i == 3)
    {
      debug ("not a valid file: %s, treate as a PDF file", filename);
      i = 0;
    }

  try
    {
      switch (i)
        {
        case 0:
          file = new ApvlvPDF (filename);
          break;

        case 1:
#ifdef HAVE_LIBUMD
          file = new ApvlvUMD (filename);
#else
          file = NULL;
#endif
          break;

        case 2:
#ifdef HAVE_DJVU
          file = new ApvlvDJVU (filename);
#else
          file = NULL;
#endif
          break;

        default:
          ;
        }
    }

  catch (bad_alloc e)
    {
      delete file;
      file = NULL;
    }

  return file;
}

}

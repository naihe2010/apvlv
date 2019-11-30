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
/* @CPPFILE ApvlvFile.cc
 *
 *  Author: Alf <naihe2010@126.com>
 */
/* @date Created: 2009/11/20 19:38:30 Alf*/

#include "ApvlvFile.h"
#include "ApvlvPdf.h"
#include "ApvlvHtm.h"
#include "ApvlvEpub.h"
#include "ApvlvUtil.h"
#include "ApvlvView.h"

#ifdef APVLV_WITH_DJVU
#include "ApvlvDjvu.h"
#endif
#ifdef APVLV_WITH_TXT
#include "ApvlvTxt.h"
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
	".pdf",
        ".html",
        ".htm",
        ".epub",
	".djv",
	".djvu",
	".txt",
      };

    size_t i;
    for (i = 0; i < 8; ++ i)
      {
	if (strcasecmp (filename + strlen (filename) - strlen (type_phrase[i]),
			type_phrase[i]) == 0)
	  {
	    break;
	  }
      }

    if (i == 8)
      {
	debug ("not a valid file: %s, treate as a PDF file", filename);
	i = 0;
      }

    file = NULL;
    try
      {
	switch (i)
	  {
	  case 0:
	    file = new ApvlvPDF (filename);
	    break;

	  case 1:
	  case 2:
            file = new ApvlvHTML (filename);
            break;

          case 3:
            file = new ApvlvEPUB (filename);
            break;

          case 4:
          case 5:
#ifdef APVLV_WITH_DJVU
	    file = new ApvlvDJVU (filename);
#endif
	    break;

	  case 6:
#ifdef APVLV_WITH_TXT
	    file = new ApvlvTXT (filename);
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

  bool ApvlvFile::render (int pn, int ix, int iy, double zm, int rot,
			  GdkPixbuf * pix, char *buffer)
  {
    return false;
  }

  bool ApvlvFile::renderweb (int pn, int, int, double, int, GtkWidget *widget)
  {
    return false;
  }
}

// Local Variables:
// mode: c++
// End:

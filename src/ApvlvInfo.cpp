/*
* This file is part of the apvlv package
* Copyright (C) <2008>  <Alf>
*
* Contact: Alf <naihe2010@gmail.com>
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
/* @CFILE ApvlvInfo.cpp 
*
*  Author: Alf <naihe2010@gmail.com>
*/
/* @date Created: 2010/02/23 15:00:42 Alf*/

#include "ApvlvInfo.hpp"

#include <stdlib.h>
#include <string.h>

#include <fstream>
#include <sstream>

namespace apvlv
{
  ApvlvInfo *gInfo = NULL;

    ApvlvInfo::ApvlvInfo (const char *filename)
  {
    mFileName = filename;

    mFileHead = NULL;
    mFileMax = 10;

    ifstream is (mFileName.c_str (), ios::in);
    if (is.is_open ())
      {
	string line;
	char *p;

	while (getline (is, line))
	  {
	    p = strchr (line.c_str (), '\t');
	    if (p == NULL)
	      {
		continue;
	      }

	    while (!isdigit (*p))
	      {
		p++;
	      }
	    int page = atoi (p);

	    p = strchr (p + 1, '\t');
	    if (p == NULL)
	      {
		continue;
	      }

	    while (!isdigit (*p))
	      {
		p++;
	      }
	    double rate = atof (p);

	    p = strchr (p + 1, '\t');
	    while (isspace (*p))
	      {
		p++;
	      }

	    infofile *fp = new infofile;
	    fp->page = page;
	    fp->rate = rate;
	    fp->file = p;
	    mFileHead = g_slist_insert_before (mFileHead, mFileHead, fp);
	  }

	mFileHead = g_slist_reverse (mFileHead);

	is.close ();
      }
  }

  ApvlvInfo::~ApvlvInfo ()
  {
    while (mFileHead != NULL)
      {
	infofile *fp = (infofile *) (mFileHead->data);
	delete fp;
	mFileHead = g_slist_next (mFileHead);
      }
    g_slist_free (mFileHead);
  }

  bool ApvlvInfo::update ()
  {
    ofstream os (mFileName.c_str (), ios::out);
    if (!os.is_open ())
      {
	return false;
      }

    int i;
    GSList *lfp;
    infofile *fp;
    for (i = 0, lfp = mFileHead;
	 i < mFileMax && lfp != NULL; ++i, lfp = g_slist_next (lfp))
      {
	fp = (infofile *) (lfp->data);
	if (fp)
	  {
	    os << "'" << i << "\t";
	    os << fp->page << "\t";
	    os << fp->rate << "\t";
	    os << fp->file << endl;
	  }
      }

    os.close ();
    return true;
  }

  infofile *ApvlvInfo::file (int id)
  {
    infofile *fp = (infofile *) g_slist_nth_data (mFileHead, id);
    return fp;
  }

  infofile *ApvlvInfo::file (const char *filename)
  {
    GSList *lfp;
    infofile *fp;

    for (lfp = mFileHead; lfp != NULL; lfp = g_slist_next (lfp))
      {
	fp = (infofile *) (lfp->data);
	if (fp->file == filename)
	  {
	    break;
	  }
      }

    if (lfp == NULL)
      {
	fp = new infofile;
	fp->page = 0;
	fp->rate = 0.0;
	fp->file = filename;
	mFileHead = g_slist_insert_before (mFileHead, mFileHead, fp);
      }
    else
      {
	mFileHead = g_slist_remove (mFileHead, fp);
	mFileHead = g_slist_insert_before (mFileHead, mFileHead, fp);
      }

    return fp;
  }

  bool ApvlvInfo::file (int page, double rate, const char *filename)
  {
    infofile *fp;

    fp = file (filename);
    if (fp == NULL)
      {
	return false;
      }

    fp->page = page;
    fp->rate = rate;
    update ();

    return true;
  }
};

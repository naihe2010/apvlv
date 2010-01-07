/*
 * This file is part of the apvlv package
 *
 * Copyright (C) 2008 Alf.
 *
 * Contact: Alf <naihe2010@gmail.com>
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
/* @CPPFILE ApvlvDoc.cpp
 *
 *  Author: Alf <naihe2010@gmail.com>
 */
/* @date Created: 2008/09/30 00:00:00 Alf */

#include "ApvlvUtil.hpp"
#include "ApvlvParams.hpp"
#include "ApvlvView.hpp"
#include "ApvlvDoc.hpp"

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <sys/stat.h>

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <gtk/gtkprintoperation.h>

#include <iostream>
#include <fstream>
#include <sstream>

namespace apvlv
{
  static GtkPrintSettings *settings = NULL;

    ApvlvDoc::ApvlvDoc (int w, int h, const char *zm, bool cache)
  {
    mCurrentCache1 = mCurrentCache2 = NULL;

    mReady = false;

    mAdjInchg = false;

    mAutoScrollPage = gParams->valueb ("autoscrollpage");
    mAutoScrollDoc = gParams->valueb ("autoscrolldoc");;
    mContinuous = gParams->valueb ("continuous");;

    mZoominit = false;

    mLines = 50;
    mChars = 80;

    mProCmd = 0;

    mRotatevalue = 0;

    mFile = NULL;

    mSearchResults = NULL;
    mSearchStr = "";

    GtkWidget *vbox, *ebox;

    if (mContinuous && gParams->valuei ("continuouspad") > 0)
      {
	vbox = gtk_vbox_new (FALSE, gParams->valuei ("continuouspad"));
      }
    else
      {
	vbox = gtk_vbox_new (FALSE, 0);
      }
    gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (mScrollwin),
					   vbox);

    mImage1 = gtk_image_new ();
    ebox = gtk_event_box_new ();
    gtk_container_add (GTK_CONTAINER (ebox), mImage1);
    g_signal_connect (G_OBJECT (ebox), "button-press-event",
		      G_CALLBACK (apvlv_doc_button_event), this);
    g_signal_connect (G_OBJECT (ebox), "button-release-event",
		      G_CALLBACK (apvlv_doc_button_event), this);
    gtk_box_pack_start (GTK_BOX (vbox), ebox, TRUE, TRUE, 0);
    if (mAutoScrollPage && mContinuous)
      {
	mImage2 = gtk_image_new ();
	ebox = gtk_event_box_new ();
	gtk_container_add (GTK_CONTAINER (ebox), mImage2);
	g_signal_connect (G_OBJECT (ebox), "button-press-event",
			  G_CALLBACK (apvlv_doc_button_event), this);
	g_signal_connect (G_OBJECT (ebox), "button-release-event",
			  G_CALLBACK (apvlv_doc_button_event), this);
	gtk_box_pack_start (GTK_BOX (vbox), ebox, TRUE, TRUE, 0);
      }

    g_signal_connect (G_OBJECT (mVaj), "value-changed",
		      G_CALLBACK (apvlv_doc_on_mouse), this);

    mStatus = new ApvlvDocStatus (this);

    gtk_box_pack_start (GTK_BOX (mVbox), mScrollwin, FALSE, FALSE, 0);
    gtk_box_pack_end (GTK_BOX (mVbox), mStatus->widget (), FALSE, FALSE, 0);

    setsize (w, h);

    setzoom (zm);
  }

  ApvlvDoc::~ApvlvDoc ()
  {
    if (mCurrentCache1)
      delete mCurrentCache1;
    if (mCurrentCache2)
      delete mCurrentCache2;

    if (mFilestr != helppdf)
      {
	savelastposition ();
      }
    mPositions.clear ();

    delete mStatus;
  }

  void ApvlvDoc::blankarea (int x1, int y1, int x2, int y2, guchar * buffer,
			    int width, int height)
  {
    debug ("x1: %d, y1: %d, x2: %d, y2:%d", x1, y1, x2, y2);
    if (x2 > width)
      {
	x2 = width;
      }
    if (y2 > height)
      {
	y2 = height;
      }

    for (gint y = y1; y < y2; y++)
      {
	for (gint x = x1; x < x2; x++)
	  {
	    gint p = (gint) (y * width * 3 + (x * 3));
	    buffer[p + 0] = 0xff - buffer[p + 0];
	    buffer[p + 1] = 0xff - buffer[p + 1];
	    buffer[p + 2] = 0xff - buffer[p + 2];
	  }
      }
  }

  void ApvlvDoc::blank (int bx, int by)
  {
    debug ("bx: %d, by: %d", bx, by);
    if (bx < 0)
      {
	bx = 0;
      }
    if (by < 0)
      {
	by = 0;
      }

    ApvlvDocCache *cache = mCurrentCache1;
    gint rate = cache->getheight () / mLines;

    if (by + rate >= cache->getheight ())
      {
	if (mCurrentCache2 == NULL)
	  {
	    by = cache->getheight () - rate;
	  }
	else
	  {
	    cache = mCurrentCache2;
	    rate = cache->getheight () / mLines;
	  }
      }

    if (bx + 10 >= cache->getwidth ())
      {
	bx = cache->getwidth () - 10;
      }

    mCurx = bx;
    mCury = by;

    if (mInVisual == VISUAL_V)
      {
	debug ("");
	mBlankx2 = bx;
	mBlanky2 = by;
	if (cache == mCurrentCache2)
	  {
	    by -=
	      mCurrentCache1->getheight () + gParams->valuei ("continouspad");
	  }
	guchar *buffer = cache->getdata (true);
	GdkPixbuf *pixbuf = cache->getbuf (true);

	if (buffer && pixbuf)
	  {
	    gint y1 = mBlanky1, y2 = mBlanky2;
	    if (y1 > y2)
	      {
		y1 = mBlanky2;
		y2 = mBlanky1;
	      }
	    gint x1 = mBlankx1, x2 = mBlankx2;
	    if (x1 > x2)
	      {
		x1 = mBlankx2;
		x2 = mBlankx1;
	      }
	    if (y1 == y2)
	      {
		blankarea (x1, y1, x2 + 10, y2 + rate, buffer,
			   cache->getwidth (), cache->getheight ());
	      }
	    else
	      {
		blankarea (x1, y1, cache->getwidth (), y1 + rate, buffer,
			   cache->getwidth (), cache->getheight ());
		blankarea (0, y1 + rate, cache->getwidth (), y2, buffer,
			   cache->getwidth (), cache->getheight ());
		blankarea (0, y2, x2 + 10, y2 + rate, buffer,
			   cache->getwidth (), cache->getheight ());
	      }

	    if (cache == mCurrentCache1)
	      {
		gtk_image_set_from_pixbuf (GTK_IMAGE (mImage1), pixbuf);
		if (mCurrentCache2 != NULL)
		  {
		    mCurrentCache2->getdata (true);
		    GdkPixbuf *p = mCurrentCache2->getbuf (true);
		    gtk_image_set_from_pixbuf (GTK_IMAGE (mImage2), p);
		  }
	      }
	    else
	      {
		mCurrentCache1->getdata (true);
		GdkPixbuf *p = mCurrentCache1->getbuf (true);
		gtk_image_set_from_pixbuf (GTK_IMAGE (mImage1), p);
		gtk_image_set_from_pixbuf (GTK_IMAGE (mImage2), pixbuf);
	      }
	  }
      }
    else if (mInVisual == VISUAL_CTRL_V)
      {
	debug ("");
	mBlankx2 = bx;
	mBlanky2 = by;
	if (cache == mCurrentCache2)
	  {
	    by -=
	      mCurrentCache1->getheight () + gParams->valuei ("continouspad");
	  }
	guchar *buffer = cache->getdata (true);
	GdkPixbuf *pixbuf = cache->getbuf (true);

	if (buffer && pixbuf)
	  {
	    gint y1 = mBlanky1, y2 = mBlanky2;
	    if (y1 > y2)
	      {
		y1 = mBlanky2;
		y2 = mBlanky1;
	      }
	    gint x1 = mBlankx1, x2 = mBlankx2;
	    if (x1 > x2)
	      {
		x1 = mBlankx2;
		x2 = mBlankx1;
	      }
	    blankarea (x1, y1, x2 + 10, y2 + rate, buffer, cache->getwidth (),
		       cache->getheight ());

	    if (cache == mCurrentCache1)
	      {
		gtk_image_set_from_pixbuf (GTK_IMAGE (mImage1), pixbuf);
		if (mCurrentCache2 != NULL)
		  {
		    mCurrentCache2->getdata (true);
		    GdkPixbuf *p = mCurrentCache2->getbuf (true);
		    gtk_image_set_from_pixbuf (GTK_IMAGE (mImage2), p);
		  }
	      }
	    else
	      {
		mCurrentCache1->getdata (true);
		GdkPixbuf *p = mCurrentCache1->getbuf (true);
		gtk_image_set_from_pixbuf (GTK_IMAGE (mImage1), p);
		gtk_image_set_from_pixbuf (GTK_IMAGE (mImage2), pixbuf);
	      }
	  }
      }
    else
      {
	debug ("");
	if (cache == mCurrentCache2)
	  {
	    by -=
	      mCurrentCache1->getheight () + gParams->valuei ("continouspad");
	  }
	guchar *buffer = cache->getdata (true);
	GdkPixbuf *pixbuf = cache->getbuf (true);

	if (buffer && pixbuf)
	  {
	    blankarea (bx, by, bx + 10, by + rate, buffer, cache->getwidth (),
		       cache->getheight ());

	    if (cache == mCurrentCache1)
	      {
		gtk_image_set_from_pixbuf (GTK_IMAGE (mImage1), pixbuf);
		if (mCurrentCache2 != NULL)
		  {
		    mCurrentCache2->getdata (true);
		    GdkPixbuf *p = mCurrentCache2->getbuf (true);
		    gtk_image_set_from_pixbuf (GTK_IMAGE (mImage2), p);
		  }
	      }
	    else
	      {
		mCurrentCache1->getdata (true);
		GdkPixbuf *p = mCurrentCache1->getbuf (true);
		gtk_image_set_from_pixbuf (GTK_IMAGE (mImage1), p);
		gtk_image_set_from_pixbuf (GTK_IMAGE (mImage2), pixbuf);
	      }
	  }
      }
  }

  void ApvlvDoc::togglevisual (int key)
  {
    if (gParams->valueb ("visualmode") == false)
      {
	return;
      }

    if (mInVisual == VISUAL_NONE)
      {
	mBlankx1 = mCurx;
	mBlanky1 = mCury;
      }

    int type = key == 'v' ? VISUAL_V : VISUAL_CTRL_V;
    if (mInVisual == type)
      {
	mInVisual = VISUAL_NONE;
      }
    else
      {
	mInVisual = type;
      }
    blank (mCurx, mCury);
  }

  int ApvlvDoc::yank (int times)
  {
    char *txt1, *txt2, *txt3;

    gint y1 = mBlanky1, y2 = mBlanky2;
    if (y1 > y2)
      {
	y1 = mBlanky2;
	y2 = mBlanky1;
      }
    gint x1 = mBlankx1, x2 = mBlankx2;
    if (x1 > x2)
      {
	x1 = mBlankx2;
	x2 = mBlankx1;
      }
    if (y1 == y2 || mInVisual == VISUAL_CTRL_V)
      {
	mFile->pagetext (mPagenum, x1, y1, x2 + 10, y2 + mVrate, &txt1);
      }
    else
      {
	mFile->pagetext (mPagenum, x1, y1, (int) mCurrentCache1->getwidth (),
			 (int) (y1 + mVrate), &txt1);
	mFile->pagetext (mPagenum, 0, y1 + mVrate,
			 (int) mCurrentCache1->getwidth (), y2, &txt2);
	mFile->pagetext (mPagenum, 0, y2, x2 + 10, y2 + mVrate, &txt3);
      }

    GtkClipboard *cb = gtk_clipboard_get (NULL);
    string text = "";
    if (txt1 != NULL)
      {
	text += txt1;
	g_free (txt1);
      }
    if (txt2 != NULL)
      {
	text += txt2;
	g_free (txt2);
      }
    if (txt3 != NULL)
      {
	text += txt3;
	g_free (txt3);
      }
    gtk_clipboard_set_text (cb, text.c_str (), text.length ());

    mInVisual = VISUAL_NONE;
    blank (mCurx, mCury);

    return 0;
  }

  returnType ApvlvDoc::subprocess (int ct, guint key)
  {
    guint procmd = mProCmd;
    mProCmd = 0;
    switch (procmd)
      {
      case 'm':
	markposition (key);
	break;

      case '\'':
	jump (key);
	break;

      case 'z':
	if (key == 'i')
	  {
	    char temp[0x10];
	    g_snprintf (temp, sizeof temp, "%f", mZoomrate * 1.1);
	    setzoom (temp);
	  }
	else if (key == 'o')
	  {
	    char temp[0x10];
	    g_snprintf (temp, sizeof temp, "%f", mZoomrate / 1.1);
	    setzoom (temp);
	  }
	else if (key == 'h')
	  {
	    setzoom ("fitheight");
	  }
	else if (key == 'w')
	  {
	    setzoom ("fitwidth");
	  }
	break;

      default:
	return NO_MATCH;
	break;
      }

    return MATCH;
  }

  returnType ApvlvDoc::process (int ct, guint key)
  {
    if (mProCmd != 0)
      {
	return subprocess (ct, key);
      }

    switch (key)
      {
      case GDK_Page_Down:
      case CTRL ('f'):
	nextpage (ct);
	break;
      case GDK_Page_Up:
      case CTRL ('b'):
	prepage (ct);
	break;
      case CTRL ('d'):
	halfnextpage (ct);
	break;
      case CTRL ('u'):
	halfprepage (ct);
	break;
      case ':':
      case '/':
      case '?':
	gView->promptcommand (key);
	return NEED_MORE;
      case 'H':
	scrollto (0.0);
	blank (0, 0);
	break;
      case 'M':
	scrollto (0.5);
	blank (0, mVaj->upper / 2);
	break;
      case 'L':
	scrollto (1.0);
	blank (0, mVaj->upper);
	break;
      case '0':
	scrollleft (mChars);
	break;
      case '$':
	scrollright (mChars);
	break;
      case CTRL ('p'):
      case GDK_Up:
      case 'k':
	scrollup (ct);
	break;
      case CTRL ('n'):
      case CTRL ('j'):
      case GDK_Down:
      case 'j':
	scrolldown (ct);
	break;
      case GDK_BackSpace:
      case GDK_Left:
      case CTRL ('h'):
      case 'h':
	scrollleft (ct);
	break;
      case GDK_space:
      case GDK_Right:
      case CTRL ('l'):
      case 'l':
	scrollright (ct);
	break;
      case 'R':
	reload ();
	break;
      case 'o':
	gView->open ();
	break;
      case 'O':
	gView->opendir ();
	break;
      case CTRL (']'):
	gotolink (ct);
	break;
      case CTRL ('t'):
	returnlink (ct);
	break;
      case 'r':
	rotate (ct);
	break;
      case 'G':
	markposition ('\'');
	showpage (ct - 1);
	break;
      case 'm':
      case '\'':
      case 'z':
	mProCmd = key;
	return NEED_MORE;
	break;
      case 'n':
	markposition ('\'');
	search ("");
	break;
      case 'N':
	markposition ('\'');
	search ("", true);
	break;
      case CTRL ('v'):
      case 'v':
	togglevisual (key);
	break;
      case ('y'):
	yank (ct);
	break;
      default:
	return NO_MATCH;
	break;
      }

    return MATCH;
  }

  ApvlvDoc *ApvlvDoc::copy ()
  {
    char rate[16];
    g_snprintf (rate, sizeof rate, "%f", mZoomrate);
    ApvlvDoc *ndoc = new ApvlvDoc (mWidth, mHeight, rate, usecache ());
    ndoc->loadfile (mFilestr, false);
    ndoc->showpage (mPagenum, scrollrate ());
    return ndoc;
  }

  void ApvlvDoc::setzoom (const char *z)
  {
    if (z != NULL)
      {
	if (strcasecmp (z, "normal") == 0)
	  {
	    mZoommode = NORMAL;
	    mZoomrate = 1.2;
	  }
	if (strcasecmp (z, "fitwidth") == 0)
	  {
	    mZoommode = FITWIDTH;
	  }
	if (strcasecmp (z, "fitheight") == 0)
	  {
	    mZoommode = FITHEIGHT;
	  }
	else
	  {
	    double d = atof (z);
	    if (d > 0)
	      {
		mZoommode = CUSTOM;
		mZoomrate = d;
	      }
	  }
      }

    if (mFile != NULL)
      {
	mFile->pagesize (0, mRotatevalue, &mPagex, &mPagey);

	if (mZoommode == FITWIDTH)
	  {
	    mZoomrate = ((double) (mWidth - 26)) / mPagex;
	  }
	else if (mZoommode == FITHEIGHT)
	  {
	    mZoomrate = ((double) (mHeight - 26)) / mPagey;
	  }

	refresh ();
      }
  }

  bool ApvlvDoc::savelastposition ()
  {
    if (filename () == NULL || gParams->valueb ("noinfo"))
      {
	return false;
      }

    gchar *path = absolutepath (sessionfile.c_str ());
    ofstream os (path, ios::app);
    g_free (path);

    if (os.is_open ())
      {
	os << ">";
	os << filename () << "\t";
	os << mPagenum << "\t";
	os << scrollrate ();
	os << "\n";
	os.close ();
	return true;
      }
    return false;
  }

  bool ApvlvDoc::loadlastposition ()
  {
    bool ret = false;
    int pagen = 0;
    double scrollr = 0.00;

    if (gParams->valueb ("noinfo"))
      {
	showpage (0);
	return false;
      }

    char *path = absolutepath (sessionfile.c_str ());
    ifstream os (path, ios::in);
    g_free (path);

    if (os.is_open ())
      {
	string line;

	while ((getline (os, line)) != NULL)
	  {
	    const char *p = line.c_str ();

	    if (*p == '>')
	      {
		stringstream ss (++p);

		string files;
		ss >> files;

		if (files == mFilestr)
		  {
		    ss >> pagen >> scrollr;
		    ret = true;
		  }
	      }
	  }
	os.close ();
      }

    // correctly check
    mScrollvalue = scrollr >= 0 ? scrollr : 0;
    showpage (pagen >= 0 ? pagen : 0);

    // Warning
    // I can't think a better way to scroll correctly when
    // the page is not be displayed correctly
    g_timeout_add (50, apvlv_doc_first_scroll_cb, this);

    return ret;
  }

  bool ApvlvDoc::reload ()
  {
    savelastposition ();
    return loadfile (mFilestr, false);
  }

  bool ApvlvDoc::loadfile (string & filename, bool check)
  {
    return loadfile (filename.c_str (), check);
  }

  bool ApvlvDoc::usecache ()
  {
    return false;
  }

  void ApvlvDoc::usecache (bool use)
  {
    if (use)
      {
	warnp ("No pthread, can't support cache!!!");
	warnp
	  ("If you really have pthread, please recomplie the apvlv and try again.");
      }
  }

  bool ApvlvDoc::loadfile (const char *filename, bool check)
  {
    if (check)
      {
	if (strcmp (filename, mFilestr.c_str ()) == 0)
	  {
	    return false;
	  }
      }

    mReady = false;

    mFile = ApvlvFile::newfile (filename);

    debug ("mFile = %p", mFile);
    if (mFile != NULL)
      {
	mFilestr = filename;

	if (mFile->pagesum () <= 1)
	  {
	    debug ("pagesum () = %d", mFile->pagesum ());
	    mContinuous = false;
	    mAutoScrollDoc = false;
	    mAutoScrollPage = false;
	  }

	debug ("pagesum () = %d", mFile->pagesum ());

	if (mCurrentCache1 != NULL)
	  {
	    delete mCurrentCache1;
	    mCurrentCache2 = NULL;
	  }
	mCurrentCache1 = new ApvlvDocCache (mFile);

	if (mCurrentCache2 != NULL)
	  {
	    delete mCurrentCache2;
	    mCurrentCache2 = NULL;
	  }

	if (mContinuous == true)
	  {
	    mCurrentCache2 = new ApvlvDocCache (mFile);
	  }

	loadlastposition ();

	mStatus->show ();

	setactive (true);

	mReady = true;
	mInVisual = VISUAL_NONE;
      }

    return mFile == NULL ? false : true;
  }

  int ApvlvDoc::convertindex (int p)
  {
    if (mFile != NULL)
      {
	int c = mFile->pagesum ();

	if (p >= 0 && p < c)
	  {
	    return p;
	  }
	else if (p >= c && mAutoScrollDoc)
	  {
	    return p % c;
	  }
	else if (p < 0 && mAutoScrollDoc)
	  {
	    return c + p;
	  }
	else
	  {
	    return -1;
	  }
      }
    return -1;
  }

  void ApvlvDoc::markposition (const char s)
  {
    ApvlvDocPosition adp = { mPagenum, scrollrate () };
    mPositions[s] = adp;
  }

  void ApvlvDoc::jump (const char s)
  {
    ApvlvDocPositionMap::iterator it;
    it = mPositions.find (s);
    if (it != mPositions.end ())
      {
	ApvlvDocPosition adp = it->second;
	markposition ('\'');
	showpage (adp.pagenum, adp.scrollrate);
      }
  }

  void ApvlvDoc::showpage (int p, double s)
  {
    int rp = convertindex (p);
    if (rp < 0)
      return;

    debug ("show page: %d", rp);
    mAdjInchg = true;

    if (mAutoScrollPage && mContinuous && !mAutoScrollDoc)
      {
	int rp2 = convertindex (p + 1);
	if (rp2 < 0)
	  {
	    if (rp == mPagenum + 1)
	      {
		return;
	      }
	    else
	      {
		rp--;
	      }
	  }
      }

    mPagenum = rp;

    if (mZoominit == false)
      {
	mZoominit = true;
	setzoom (NULL);
	debug ("zoom rate: %f", mZoomrate);
      }

    refresh ();

    scrollto (s);
    blank (0, mCurrentCache1->getheight () * s);
  }

  void ApvlvDoc::nextpage (int times)
  {
    showpage (mPagenum + times);
  }

  void ApvlvDoc::prepage (int times)
  {
    showpage (mPagenum - times);
  }

  void ApvlvDoc::refresh ()
  {
    if (mFile == NULL)
      return;

    mCurrentCache1->set (mPagenum, mZoomrate, mRotatevalue, false);
    GdkPixbuf *buf = mCurrentCache1->getbuf (true);
    gtk_image_set_from_pixbuf (GTK_IMAGE (mImage1), buf);

    if (mAutoScrollPage && mContinuous)
      {
	mCurrentCache2->set (convertindex (mPagenum + 1), mZoomrate,
			     mRotatevalue, false);
	buf = mCurrentCache2->getbuf (true);
	gtk_image_set_from_pixbuf (GTK_IMAGE (mImage2), buf);
      }

    mStatus->show ();
  }

  void ApvlvDoc::halfnextpage (int times)
  {
    double sr = scrollrate ();
    int rtimes = times / 2;

    if (times % 2 != 0)
      {
	if (sr > 0.5)
	  {
	    sr = 0;
	    rtimes += 1;
	  }
	else
	  {
	    sr = 1;
	  }
      }

    showpage (mPagenum + rtimes, sr);
  }

  void ApvlvDoc::halfprepage (int times)
  {
    double sr = scrollrate ();
    int rtimes = times / 2;

    if (times % 2 != 0)
      {
	if (sr < 0.5)
	  {
	    sr = 1;
	    rtimes += 1;
	  }
	else
	  {
	    sr = 0;
	  }
      }

    showpage (mPagenum - rtimes, sr);
  }

  void ApvlvDoc::markselection ()
  {
    debug ("mSelect: %d.", mSearchSelect);
    debug ("zoomrate: %f", mZoomrate);

    ApvlvPos rect = (*mSearchResults)[mSearchSelect];

    // Caculate the correct position
    //debug ("pagex: %f, pagey: %f, x1: %f, y1: %f, x2: %f, y2: %f", mPagex, mPagey, rect->x1, rect->y1, rect->x2, rect->y2);
    gint x1 = (gint) ((rect.x1) * mZoomrate);
    gint x2 = (gint) ((rect.x2) * mZoomrate);
    gint y1 = (gint) ((mPagey - rect.y2) * mZoomrate);
    gint y2 = (gint) ((mPagey - rect.y1) * mZoomrate);
    debug ("x1: %d, y1: %d, x2: %d, y2: %d", x1, y1, x2, y2);

    // make the selection at the page center
    gdouble val = ((y1 + y2) - mVaj->page_size) / 2;
    debug ("upper: %f, lower: %f, page_size: %f, val: %f",
	   mVaj->upper, mVaj->lower, mVaj->page_size, val);
    if (val + mVaj->page_size > mVaj->upper - mVaj->lower - 5)
      {
	debug ("set value: %f",
	       mVaj->upper - mVaj->lower - mVaj->page_size - 5);
	gtk_adjustment_set_value (mVaj, mVaj->upper - mVaj->lower - mVaj->page_size - 5);	/* just for avoid the auto scroll page */
      }
    else if (val > 5)
      {
	debug ("set value: %f", val);
	gtk_adjustment_set_value (mVaj, val);
      }
    else
      {
	debug ("set value: %f", mVaj->lower + 5);
	gtk_adjustment_set_value (mVaj, mVaj->lower + 5);	/* avoid auto scroll page */
      }

    val = ((x1 + x2) - mHaj->page_size) / 2;
    if (val + mHaj->page_size > mHaj->upper)
      {
	gtk_adjustment_set_value (mHaj, mHaj->upper);
      }
    else if (val > 0)
      {
	gtk_adjustment_set_value (mHaj, val);
      }
    else
      {
	gtk_adjustment_set_value (mHaj, mHaj->lower);
      }

    mCurrentCache1->set (mPagenum, mZoomrate, mRotatevalue);
    guchar *pagedata = mCurrentCache1->getdata (true);
    GdkPixbuf *pixbuf = mCurrentCache1->getbuf (true);

    mFile->pageselectsearch (mPagenum, mCurrentCache1->getwidth (),
			     mCurrentCache1->getheight (), mZoomrate,
			     mRotatevalue, pixbuf, (char *) pagedata,
			     mSearchSelect, mSearchResults);
    gtk_image_set_from_pixbuf (GTK_IMAGE (mImage1), pixbuf);
    debug ("helight num: %d", mPagenum);
  }

  bool ApvlvDoc::continuous ()
  {
    return mContinuous;
  }

  void ApvlvDoc::scrollup (int times)
  {
    if (!mReady)
      return;

    if (gParams->valueb ("visualmode") == false)
      {
	ApvlvCore::scrollup (times);
	return;
      }

    gint opage = mPagenum, npage = mPagenum;
    gdouble sub = mVaj->upper - mVaj->lower;
    mVrate = sub / mLines;

    gint ny1 = mCury - mVrate * times;
    if (ny1 < mVaj->value)
      {
	ApvlvCore::scrollup (times);
	npage = mPagenum;
      }

    if (npage == opage)
      {
	blank (mCurx, ny1);
      }
    else
      {
      }
  }

  void ApvlvDoc::scrolldown (int times)
  {
    if (!mReady)
      return;

    if (gParams->valueb ("visualmode") == false)
      {
	ApvlvCore::scrolldown (times);
	return;
      }

    gdouble sub = mVaj->upper - mVaj->lower;
    mVrate = sub / mLines;

    gint opage = mPagenum, npage = mPagenum;
    gint ny1 = mCury + mVrate * times;
    if (ny1 > mVaj->page_size)
      {
	ApvlvCore::scrolldown (times);
	npage = mPagenum;
      }

    if (npage == opage)
      {
	blank (mCurx, ny1);
      }
    else
      {
      }
  }

  void ApvlvDoc::scrollleft (int times)
  {
    if (!mReady)
      return;

    if (gParams->valueb ("visualmode") == false)
      {
	ApvlvCore::scrollleft (times);
	return;
      }

    gdouble sub = mHaj->upper - mHaj->lower;
    mHrate = sub / mChars;

    gint nx1 = mCurx - mHrate * times;
    if (nx1 < mHaj->upper - mHaj->page_size)
      {
	ApvlvCore::scrollleft (times);
      }
    blank (nx1, mCury);
  }

  void ApvlvDoc::scrollright (int times)
  {
    if (!mReady)
      return;

    if (gParams->valueb ("visualmode") == false)
      {
	ApvlvCore::scrollright (times);
	return;
      }

    gdouble sub = mHaj->upper - mHaj->lower;
    mHrate = sub / mChars;

    gint nx1 = mCurx + mHrate * times;
    if (nx1 > mHaj->page_size)
      {
	ApvlvCore::scrollright (times);
      }
    blank (nx1, mCury);
  }

  bool ApvlvDoc::needsearch (const char *str, bool reverse)
  {
    if (mFile == NULL)
      return false;

    // search a different string
    if (strlen (str) > 0 && strcmp (str, mSearchStr.c_str ()) != 0)
      {
	debug ("different string.");
	mSearchSelect = 0;
	mSearchStr = str;
	return true;
      }

    else if (mSearchResults == NULL)
      {
	debug ("no result.");
	mSearchSelect = 0;
	return true;
      }

    // same string, but need to search next page
    else
      if (((mSearchReverse == reverse)
	   && mSearchSelect == mSearchResults->size () - 1)
	  || ((mSearchReverse != reverse) && mSearchSelect == 0))
      {
	debug
	  ("same, but need next string: S: %d, s: %d, sel: %d, max: %d.",
	   mSearchReverse, reverse, mSearchSelect, mSearchResults->size ());
	mSearchSelect = 0;
	return true;
      }

    // same string, not need search, but has zoomed
    else
      {
	debug
	  ("same, not need next string. sel: %d, max: %u",
	   mSearchSelect, mSearchResults->size ());
	if (mSearchReverse == reverse)
	  {
	    mSearchSelect++;
	  }
	else
	  {
	    mSearchSelect--;
	  }

	markselection ();
	return false;
      }
  }

  void ApvlvDoc::search (const char *str, bool reverse)
  {
    if (!needsearch (str, reverse))
      {
	return;
      }

    if (mSearchResults != NULL)
      {
	delete mSearchResults;
	mSearchResults = NULL;
      }

    bool wrap = gParams->valueb ("wrapscan");

    int i = mPagenum;
    int sum = mFile->pagesum (), from = i;
    bool search = false;
    while (1)
      {
	if (*str != 0 || search)
	  {
	    mSearchResults =
	      mFile->pagesearch ((i + sum) % sum, mSearchStr.c_str (),
				 reverse);
	    mSearchReverse = reverse;
	    if (mSearchResults != NULL)
	      {
		showpage ((i + sum) % sum, 0.5);
		markselection ();
		break;
	      }
	  }

	search = true;

	if (!reverse && i < (wrap ? (from + sum) : (sum - 1)))
	  {
	    debug ("wrap: %d, i++:", wrap, i, i + 1);
	    i++;
	  }
	else if (reverse && i > (wrap ? (from - sum) : 0))
	  {
	    debug ("wrap: %d, i--:", wrap, i, i - 1);
	    i--;
	  }
	else
	  {
	    break;
	  }
      }
  }

  bool ApvlvDoc::totext (const char *file)
  {
    if (mFile == NULL)
      return false;

    char *txt;
    bool ret = mFile->pagetext (mPagenum, 0, 0, mCurrentCache1->getwidth (),
				mCurrentCache1->getheight (), &txt);
    if (ret == true)
      {
	g_file_set_contents (file, txt, -1, NULL);
	return true;
      }
    return false;
  }

  void ApvlvDoc::setactive (bool act)
  {
    mStatus->active (act);
    mActive = act;
  }

  bool ApvlvDoc::rotate (int ct)
  {
    // just hack
    if (ct == 1)
      ct = 90;

    if (ct % 90 != 0)
      {
	warnp ("Not a 90 times value, ignore.");
	return false;
      }

    mRotatevalue += ct;
    while (mRotatevalue < 0)
      {
	mRotatevalue += 360;
      }
    refresh ();
    return true;
  }

  void ApvlvDoc::gotolink (int ct)
  {
    ApvlvLinks *links1 = mCurrentCache1->getlinks ();
    ApvlvLinks *links2 = mCurrentCache2 ? mCurrentCache2->getlinks () : NULL;

    int siz = links1 ? links1->size () : 0;
    siz += links2 ? links2->size () : 0;

    ct--;

    if (ct >= 0 && ct < siz)
      {
	markposition ('\'');

	ApvlvDocPosition p = { mPagenum, scrollrate () };
	mLinkPositions.push_back (p);

	if (links1 == NULL)
	  {
	    showpage ((*links2)[ct].mPage);
	  }
	else
	  {
	    if (ct < (int) links1->size ())
	      {
		showpage ((*links1)[ct].mPage);
	      }
	    else
	      {
		showpage ((*links2)[ct - links1->size ()].mPage);
	      }
	  }
      }
  }

  void ApvlvDoc::returnlink (int ct)
  {
    debug ("Ctrl-t %d", ct);
    if (ct <= (int) mLinkPositions.size () && ct > 0)
      {
	markposition ('\'');
	ApvlvDocPosition p = { 0, 0 };
	while (ct-- > 0)
	  {
	    p = mLinkPositions[mLinkPositions.size () - 1];
	    mLinkPositions.pop_back ();
	  }
	showpage (p.pagenum, p.scrollrate);
      }
  }

  bool ApvlvDoc::print (int ct)
  {
#ifdef WIN32
    return false;
#else
    bool ret = false;
    GtkPrintOperation *print = gtk_print_operation_new ();

    gtk_print_operation_set_allow_async (print, TRUE);
    gtk_print_operation_set_show_progress (print, TRUE);

    PrintData *data = new PrintData;
    data->file = mFile;
    data->frmpn = mPagenum;
    data->endpn = mPagenum + (ct > 0 ? ct : 1) - 1;
    if ((int) data->endpn >= mFile->pagesum ())
      {
	data->endpn = mFile->pagesum () - 1;
      }

    g_signal_connect (G_OBJECT (print), "begin-print",
		      G_CALLBACK (begin_print), data);
    g_signal_connect (G_OBJECT (print), "draw-page", G_CALLBACK (draw_page),
		      data);
    g_signal_connect (G_OBJECT (print), "end-print", G_CALLBACK (end_print),
		      data);
    if (settings != NULL)
      {
	gtk_print_operation_set_print_settings (print, settings);
      }
    int r =
      gtk_print_operation_run (print, GTK_PRINT_OPERATION_ACTION_PRINT_DIALOG,
			       GTK_WINDOW (gView->widget ()), NULL);
    if (r == GTK_PRINT_OPERATION_RESULT_APPLY)
      {
	if (settings != NULL)
	  {
	    g_object_unref (settings);
	  }
	settings = gtk_print_operation_get_print_settings (print);
	ret = true;
      }
    g_object_unref (print);
    return ret;
#endif
  }

  void ApvlvDoc::apvlv_doc_on_mouse (GtkAdjustment * adj, ApvlvDoc * doc)
  {
    if (doc->mAdjInchg)
      {
	doc->mAdjInchg = false;
	return;
      }

    if (adj->upper - adj->lower == adj->page_size + adj->value)
      {
	doc->scrolldown (1);
      }
    else if (adj->value == 0)
      {
	doc->scrollup (1);
      }
  }

  gboolean ApvlvDoc::apvlv_doc_first_scroll_cb (gpointer data)
  {
    ApvlvDoc *doc = (ApvlvDoc *) data;
    return doc->scrollto (doc->mScrollvalue) == TRUE ? FALSE : TRUE;
  }

  gboolean ApvlvDoc::apvlv_doc_first_copy_cb (gpointer data)
  {
    ApvlvDoc *doc = (ApvlvDoc *) data;
    doc->loadfile (doc->mFilestr, false);
    return FALSE;
  }

#ifndef WIN32
  void
    ApvlvDoc::begin_print (GtkPrintOperation * operation,
			   GtkPrintContext * context, PrintData * data)
  {
    gtk_print_operation_set_n_pages (operation,
				     data->endpn - data->frmpn + 1);
  }

  void
    ApvlvDoc::draw_page (GtkPrintOperation * operation,
			 GtkPrintContext * context,
			 gint page_nr, PrintData * data)
  {
    cairo_t *cr = gtk_print_context_get_cairo_context (context);
    data->file->pageprint (data->frmpn + page_nr, cr);
    PangoLayout *layout = gtk_print_context_create_pango_layout (context);
    pango_cairo_show_layout (cr, layout);
    g_object_unref (layout);
  }

  void
    ApvlvDoc::end_print (GtkPrintOperation * operation,
			 GtkPrintContext * context, PrintData * data)
  {
    delete data;
  }
#endif

  void
    ApvlvDoc::apvlv_doc_button_event (GtkEventBox * box,
				      GdkEventButton * button, ApvlvDoc * doc)
  {
    if (button->button == 1)
      {
	if (button->type == GDK_BUTTON_PRESS)
	  {
	    doc->mBlankx1 = button->x;
	    doc->mBlanky1 = button->y;
	    doc->mInVisual = VISUAL_V;
	  }
	else if (button->type == GDK_BUTTON_RELEASE)
	  {
	    doc->mBlankx2 = button->x;
	    doc->mBlanky2 = button->y;
	    doc->blank (doc->mBlankx2, doc->mBlanky2);
	  }
      }
    else if (button->button == 3)
      {
	GtkWidget *menu, *item;

	menu = gtk_menu_new ();
	gtk_menu_attach_to_widget (GTK_MENU (menu), GTK_WIDGET (box), NULL);

	item = gtk_image_menu_item_new_with_label ("Copy to Clipboard");
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
	gtk_widget_show (item);

	g_signal_connect (item, "activate",
			  G_CALLBACK (apvlv_doc_copytoclipboard_cb), doc);

	gtk_menu_popup (GTK_MENU (menu), NULL, NULL, NULL, NULL, 0, 0);
      }
  }

  void
    ApvlvDoc::apvlv_doc_copytoclipboard_cb (GtkMenuItem * item,
					    ApvlvDoc * doc)
  {
    doc->yank (1);
  }

  ApvlvDocCache::ApvlvDocCache (ApvlvFile * file)
  {
    mFile = file;
    mPagenum = -1;
    mData = NULL;
    mSize = 0;
    mBuf = NULL;
    mLinks = NULL;
  }

  void ApvlvDocCache::set (guint p, double zm, guint rot, bool delay)
  {
    mPagenum = p;
    mZoom = zm;
    mRotate = rot;

    if (mData != NULL)
      {
	delete[]mData;
	mData = NULL;
      }
    if (mBuf != NULL)
      {
	g_object_unref (mBuf);
	mBuf = NULL;
      }
    if (mLinks != NULL)
      {
	delete mLinks;
	mLinks = NULL;
      }

    load (this);
  }

  void ApvlvDocCache::load (ApvlvDocCache * ac)
  {
    int c = ac->mFile->pagesum ();

    if (ac->mPagenum < 0 || ac->mPagenum >= c)
      {
	debug ("no this page: %d", ac->mPagenum);
	return;
      }

    double tpagex, tpagey;
    ac->mFile->pagesize (ac->mPagenum, ac->mRotate, &tpagex, &tpagey);

    ac->mWidth = MAX ((tpagex * ac->mZoom + 0.5), 1);
    ac->mHeight = MAX ((tpagey * ac->mZoom + 0.5), 1);

    // this is very import to get the double times size data
    // the 2ed chunk data will be for output
    ac->mSize = ac->mWidth * ac->mHeight * 3;
    guchar *dat = new guchar[2 * ac->mSize];

    GdkPixbuf *bu = gdk_pixbuf_new_from_data (dat, GDK_COLORSPACE_RGB,
					      FALSE,
					      8,
					      ac->mWidth, ac->mHeight,
					      3 * ac->mWidth,
					      NULL, NULL);
    ac->mFile->render (ac->mPagenum, ac->mWidth, ac->mHeight, ac->mZoom,
		       ac->mRotate, bu, (char *) dat);
    // backup the pixbuf data
    memcpy (dat + ac->mSize, dat, ac->mSize);

    if (ac->mLinks)
      {
	delete ac->mLinks;
      }
    ac->mLinks = ac->mFile->getlinks (ac->mPagenum);
    debug ("has mLinkMappings: %p", ac->mLinks);

    ac->mData = dat;
    ac->mBuf = bu;
  }

  ApvlvDocCache::~ApvlvDocCache ()
  {
    if (mLinks)
      {
	delete mLinks;
      }

    if (mData != NULL)
      delete[]mData;
    if (mBuf != NULL)
      g_object_unref (mBuf);
  }

  guint ApvlvDocCache::getpagenum ()
  {
    return mPagenum;
  }

  /*
   * get the cache data
   * @param: wait, if not wait, not wait the buffer be prepared
   * @return: the buffer
   * */
  guchar *ApvlvDocCache::getdata (bool wait)
  {
    memcpy (mData, mData + mSize, mSize);
    return mData;
  }

  /*
   * get the cache GdkPixbuf
   * @param: wait, if not wait, not wait the pixbuf be prepared
   * @return: the buffer
   * */
  GdkPixbuf *ApvlvDocCache::getbuf (bool wait)
  {
    return mBuf;
  }

  double ApvlvDocCache::getwidth ()
  {
    return mWidth;
  }

  double ApvlvDocCache::getheight ()
  {
    return mHeight;
  }

  ApvlvLinks *ApvlvDocCache::getlinks ()
  {
    return mLinks;
  }

  ApvlvDocStatus::ApvlvDocStatus (ApvlvDoc * doc)
  {
    mDoc = doc;
    for (int i = 0; i < AD_STATUS_SIZE; ++i)
      {
	mStlab[i] = gtk_label_new ("");
	gtk_box_pack_start (GTK_BOX (mHbox), mStlab[i], FALSE, FALSE, 0);
      }
  }

  ApvlvDocStatus::~ApvlvDocStatus ()
  {
  }

  void ApvlvDocStatus::active (bool act)
  {
    GdkColor c;

    if (act)
      {
	c.red = 300;
	c.green = 300;
	c.blue = 300;
      }
    else
      {
	c.red = 30000;
	c.green = 30000;
	c.blue = 30000;
      }

    for (unsigned int i = 0; i < AD_STATUS_SIZE; ++i)
      {
	gtk_widget_modify_fg (mStlab[i], GTK_STATE_NORMAL, &c);
      }
  }

  void ApvlvDocStatus::setsize (int w, int h)
  {
    int sw[AD_STATUS_SIZE];
    sw[0] = w >> 1;
    sw[1] = sw[0] >> 1;
    sw[2] = sw[1] >> 1;
    sw[3] = sw[1] >> 1;
    for (unsigned int i = 0; i < AD_STATUS_SIZE; ++i)
      {
	gtk_widget_set_usize (mStlab[i], sw[i], h);
      }
  }

  void ApvlvDocStatus::show ()
  {
    if (mDoc->filename ())
      {
	gint pn = mDoc->pagenumber ();

	if (mDoc->continuous () && mDoc->scrollrate () > 0.5)
	  {
	    pn++;
	  }

	char temp[AD_STATUS_SIZE][256];
	gchar *bn;
	bn = g_path_get_basename (mDoc->filename ());
	g_snprintf (temp[0], sizeof temp[0], "%s", bn);
	g_snprintf (temp[1], sizeof temp[1], "%d/%d", pn,
		    mDoc->file ()->pagesum ());
	g_snprintf (temp[2], sizeof temp[2], "%d%%",
		    (int) (mDoc->zoomvalue () * 100));
	g_snprintf (temp[3], sizeof temp[3], "%d%%",
		    (int) (mDoc->scrollrate () * 100));
	for (unsigned int i = 0; i < AD_STATUS_SIZE; ++i)
	  {
	    gtk_label_set_text (GTK_LABEL (mStlab[i]), temp[i]);
	  }
	g_free (bn);
      }
  }
}

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
#include <glib/poppler.h>

#include <libdjvu/ddjvuapi.h>

#include <iostream>
#include <fstream>
#include <sstream>

namespace apvlv
{
#ifdef HAVE_LIBDJVU
  void handle_ddjvu_messages (ddjvu_context_t * ctx, int wait)
  {
    const ddjvu_message_t *msg;
    if (wait)
        ddjvu_message_wait (ctx);
    while ((msg = ddjvu_message_peek (ctx)))
      {
	debug ("tag: %d", msg->m_any.tag);
	switch (msg->m_any.tag)
	  {
	  case DDJVU_ERROR:
	    break;
	    case DDJVU_INFO:break;
	    case DDJVU_PAGEINFO:break;
	    default:break;
	  }
	ddjvu_message_pop (ctx);
      }
  }
#endif

  enum
  { AD_NONE, AD_PDF, AD_DJVU };

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

    mPDFDoc = NULL;
    mDjvuDoc = NULL;

    mDocType = AD_NONE;

    mSearchResults = NULL;
    mSearchStr = "";

    GtkWidget *vbox;

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
    gtk_box_pack_start (GTK_BOX (vbox), mImage1, TRUE, TRUE, 0);
    if (mAutoScrollPage && mContinuous)
      {
	mImage2 = gtk_image_new ();
	gtk_box_pack_start (GTK_BOX (vbox), mImage2, TRUE, TRUE, 0);
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

    if (mPDFDoc)
      {
	debug ("Free the PopplerDocument");
	debug ("And, Maybe there is some bugs in poppler-glib libiray");
	g_object_unref (mPDFDoc);
      }
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
	  zoomin ();
	else if (key == 'o')
	  zoomout ();
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
	break;
      case 'M':
	scrollto (0.5);
	break;
      case 'L':
	scrollto (1.0);
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

  void *ApvlvDoc::getdoc ()
  {
    return mDocType == AD_PDF ? (void *) mPDFDoc :
      mDocType == AD_DJVU ? (void *) mDjvuDoc : NULL;
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


    mPDFDoc = file_to_popplerdoc (filename);
    if (mPDFDoc != NULL)
      {
	mDocType = AD_PDF;
      }

#ifdef HAVE_LIBDJVU
    if (mDocType == AD_NONE)
      {
	mDjvuContext = ddjvu_context_create ("apvlv");
	if (mDjvuContext)
	  {
	    mDjvuDoc =
	      ddjvu_document_create_by_filename (mDjvuContext, filename,
						 FALSE);
	  }

	if (mDjvuDoc != NULL)
	  {
	    if (ddjvu_document_get_type (mDjvuDoc) != DDJVU_DOCTYPE_UNKNOWN)
	      {
		debug ("type: %d", ddjvu_document_get_type (mDjvuDoc));
		mDocType = AD_DJVU;
	      }
	    else
	      {
		ddjvu_document_release (mDjvuDoc);
		ddjvu_context_release (mDjvuContext);
		mDjvuDoc = NULL;
	      }
	  }
      }
#endif
    if (mDocType != AD_NONE)
      {
	mFilestr = filename;

	if (pagesum () <= 1)
	  {
	    debug ("pagesum () = %d", pagesum ());
	    mContinuous = false;
	    mAutoScrollDoc = false;
	    mAutoScrollPage = false;
	  }

	debug ("pagesum () = %d", pagesum ());

	if (mCurrentCache1 != NULL)
	  {
	    delete mCurrentCache1;
	    mCurrentCache2 = NULL;
	  }
	mCurrentCache1 = new ApvlvDocCache (this);

	if (mCurrentCache2 != NULL)
	  {
	    delete mCurrentCache2;
	    mCurrentCache2 = NULL;
	  }

	if (mContinuous == true)
	  {
	    mCurrentCache2 = new ApvlvDocCache (this);
	  }

	loadlastposition ();

	mStatus->show ();

	setactive (true);

	mReady = true;
      }

    return mDocType == AD_NONE ? false : true;
  }

  gint ApvlvDoc::pagesum ()
  {
    return mPDFDoc ? poppler_document_get_n_pages (mPDFDoc) :
      mDjvuDoc ? ddjvu_document_get_pagenum (mDjvuDoc) : 0;
  }

  int ApvlvDoc::convertindex (int p)
  {
    if (mDocType != AD_NONE)
      {
	int c = pagesum ();

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

	if (mDocType == AD_PDF)
	  {
	    PopplerPage *mPage = poppler_document_get_page (mPDFDoc, rp);
	    getpagesize (mPage, &mPagex, &mPagey);
	  }
#ifdef HAVE_LIBDJVU
	else if (mDocType == AD_DJVU)
	  {
	    ddjvu_status_t t;
	    ddjvu_pageinfo_t info[1];
	    while ((t =
		    ddjvu_document_get_pageinfo (mDjvuDoc, 0,
						 info)) < DDJVU_JOB_OK)
	      {
		handle_ddjvu_messages (mDjvuContext, true);
	      }

	    if (t == DDJVU_JOB_OK)
	      {
		mPagex = info->width;
		mPagey = info->height;
		debug ("djvu page 1: %f-%f", mPagex, mPagey);
	      }
	  }
#endif

	switch (mZoommode)
	  {
	  case NORMAL:
	    mZoomrate = 1.2;
	    break;
	  case FITWIDTH:
	    mZoomrate = ((double) (mWidth - 26)) / mPagex;
	    debug ("mWidth: %d, zoom rate: %f", mWidth, mZoomrate);
	    break;
	  case FITHEIGHT:
	    mZoomrate = ((double) (mHeight - 26)) / mPagey;
	    break;
	  case CUSTOM:
	    break;
	  default:
	    break;
	  }

	debug ("zoom rate: %f", mZoomrate);
      }

    refresh ();

    scrollto (s);
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
    if (mDocType == AD_NONE)
      return;

    mCurrentCache1->set (mPagenum, false);
    GdkPixbuf *buf = mCurrentCache1->getbuf (true);
    gtk_image_set_from_pixbuf (GTK_IMAGE (mImage1), buf);
    if (mAutoScrollPage && mContinuous)
      {
	mCurrentCache2->set (convertindex (mPagenum + 1), false);
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
    GList *selist = g_list_nth (mSearchResults, mSearchSelect);
    PopplerRectangle *rect = (PopplerRectangle *) selist->data;

    debug ("zoomrate: %f", mZoomrate);

    // Caculate the correct position
    //debug ("pagex: %f, pagey: %f, x1: %f, y1: %f, x2: %f, y2: %f", mPagex, mPagey, rect->x1, rect->y1, rect->x2, rect->y2);
    gint x1 = (gint) ((rect->x1) * mZoomrate);
    gint x2 = (gint) ((rect->x2) * mZoomrate);
    gint y1 = (gint) ((mPagey - rect->y2) * mZoomrate);
    gint y2 = (gint) ((mPagey - rect->y1) * mZoomrate);
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

    mCurrentCache1->set (mPagenum);
    guchar *pagedata = mCurrentCache1->getdata (true);
    GdkPixbuf *pixbuf = mCurrentCache1->getbuf (true);

    // heightlight the selection
    for (gint y = y1; y < y2; y++)
      {
	for (gint x = x1; x < x2; x++)
	  {
	    gint p = (gint) (y * 3 * mPagex * mZoomrate + (x * 3));
	    pagedata[p + 0] = 0xff - pagedata[p + 0];
	    pagedata[p + 1] = 0xff - pagedata[p + 0];
	    pagedata[p + 2] = 0xff - pagedata[p + 0];
	  }
      }

    // change the back color of the selection
    for (selist = mSearchResults; selist != NULL;
	 selist = g_list_next (selist))
      {
	PopplerRectangle *rect = (PopplerRectangle *) selist->data;

	// Caculate the correct position
	x1 = (gint) ((rect->x1) * mZoomrate);
	x2 = (gint) ((rect->x2) * mZoomrate);
	y1 = (gint) ((mPagey - rect->y2) * mZoomrate);
	y2 = (gint) ((mPagey - rect->y1) * mZoomrate);

	for (gint y = y1; y < y2; y++)
	  {
	    for (gint x = x1; x < x2; x++)
	      {
		gint p = (gint) (y * 3 * mPagex * mZoomrate + (x * 3));
		pagedata[p + 0] = 0xff - pagedata[p + 0];
		pagedata[p + 1] = 0xff - pagedata[p + 0];
		pagedata[p + 2] = 0xff - pagedata[p + 0];
	      }
	  }
      }

    gtk_image_set_from_pixbuf (GTK_IMAGE (mImage1), pixbuf);
    debug ("helight num: %d", mPagenum);
  }

  GList *ApvlvDoc::searchpage (int num, bool reverse)
  {
    debug ("search num: %d", num);
    PopplerPage *page;

    if (mDocType == AD_NONE
	|| (page = poppler_document_get_page (mPDFDoc, num)) == NULL)
      {
	return NULL;
      }

    GList *list = poppler_page_find_text (page, mSearchStr.c_str ());
    if (reverse)
      {
	list = g_list_reverse (list);
      }

    mSearchReverse = reverse;

    return list;
  }

  bool ApvlvDoc::continuous ()
  {
    return mContinuous;
  }

  bool ApvlvDoc::needsearch (const char *str, bool reverse)
  {
    if (mDocType == AD_NONE)
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
	   && mSearchSelect == g_list_length (mSearchResults) - 1)
	  || ((mSearchReverse != reverse) && mSearchSelect == 0))
      {
	debug
	  ("same, but need next string: S: %d, s: %d, sel: %d, max: %d.",
	   mSearchReverse, reverse, mSearchSelect,
	   g_list_length (mSearchResults));
	mSearchSelect = 0;
	return true;
      }

    // same string, not need search, but has zoomed
    else
      {
	debug
	  ("same, not need next string. sel: %d, max: %u",
	   mSearchSelect, g_list_length (mSearchResults));
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
	while (mSearchResults != NULL)
	  {
	    PopplerRectangle *rect =
	      (PopplerRectangle *) mSearchResults->data;
	    g_free (rect);
	    mSearchResults = g_list_next (mSearchResults);
	  }
	g_list_free (mSearchResults);
	mSearchResults = NULL;
      }

    int i =
      strlen (str) > 0 ? mPagenum : reverse ? mPagenum - 1 : mPagenum + 1;
    int sum = pagesum (), from = i;
    while (1)
      {
	mSearchResults = searchpage ((i + sum) % sum, reverse);
	if (mSearchResults != NULL)
	  {
	    showpage ((i + sum) % sum, 0.5);
	    markselection ();
	    break;
	  }

	if (!reverse && i < from + sum)
	  {
	    i++;
	  }
	else if (reverse && i > from - sum)
	  {
	    i--;
	  }
	else
	  {
	    break;
	  }
      }
  }

  void ApvlvDoc::getpagesize (PopplerPage * p, double *x, double *y)
  {
    if (mRotatevalue == 90 || mRotatevalue == 270)
      {
	poppler_page_get_size (p, y, x);
      }
    else
      {
	poppler_page_get_size (p, x, y);
      }
  }

  void ApvlvDoc::getpagesize (int pn, double *x, double *y)
  {
#ifdef HAVE_LIBDJVU
    if (mDocType == AD_DJVU)
      {
	ddjvu_status_t t;
	ddjvu_pageinfo_t info[1];
	while ((t =
		ddjvu_document_get_pageinfo (mDjvuDoc, 0,
					     info)) < DDJVU_JOB_OK)
	  {
	    handle_ddjvu_messages (mDjvuContext, true);
	  }

	if (t == DDJVU_JOB_OK)
	  {
	    *x = info->width;
	    *y = info->height;
	    debug ("djvu page %d: %f-%f", pn, *x, *y);
	  }
      }
#endif
  }

  bool ApvlvDoc::getpagetext (PopplerPage * page, char **text)
  {
    double x, y;
    getpagesize (page, &x, &y);
    PopplerRectangle rect = { 0, 0, x, y };
    *text = poppler_page_get_text (page, POPPLER_SELECTION_WORD, &rect);
    if (*text != NULL)
      {
	return true;
      }
    return false;
  }

  bool ApvlvDoc::totext (const char *file)
  {
    if (mDocType == AD_NONE)
      return false;

    PopplerPage *page = mCurrentCache1->getpage ();
    char *txt;
    bool ret = getpagetext (page, &txt);
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
    int nn = -1;

    PopplerLinkMapping *map =
      (PopplerLinkMapping *) g_list_nth_data (mCurrentCache1->getlinks (),
					      ct - 1);

    debug ("Ctrl-] %d", ct);
    if (map)
      {
	PopplerAction *act = map->action;
	if (act && *(PopplerActionType *) act == POPPLER_ACTION_GOTO_DEST)
	  {
	    PopplerDest *pd = ((PopplerActionGotoDest *) act)->dest;
	    if (pd->type == POPPLER_DEST_NAMED)
	      {
		PopplerDest *destnew = poppler_document_find_dest (mPDFDoc,
								   pd->
								   named_dest);
		if (destnew != NULL)
		  {
		    nn = destnew->page_num - 1;
		    debug ("nn: %d", nn);
		    poppler_dest_free (destnew);
		  }
	      }
	    else
	      {
		nn = pd->page_num - 1;
		debug ("nn: %d", nn);
	      }
	  }
      }

    if (nn >= 0)
      {
	markposition ('\'');

	ApvlvDocPosition p = { mPagenum, scrollrate () };
	mLinkPositions.push_back (p);

	showpage (nn);
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
    data->doc = mPDFDoc;
    data->frmpn = mPagenum;
    data->endpn = mPagenum;

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
    PopplerPage *page =
      poppler_document_get_page (data->doc, data->frmpn + page_nr);
    poppler_page_render_for_printing (page, cr);

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

  ApvlvDocCache::ApvlvDocCache (ApvlvDoc * dc)
  {
    mDoc = dc;
    mPagenum = -1;
    mData = NULL;
    mBuf = NULL;
    mLinkMappings = NULL;
  }

  void ApvlvDocCache::set (guint p, bool delay)
  {
    mPagenum = p;
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
    if (mLinkMappings != NULL)
      {
	g_list_free (mLinkMappings);
	mLinkMappings = NULL;
      }

    load (this);
  }

  void ApvlvDocCache::load (ApvlvDocCache * ac)
  {
    int c = ac->mDoc->pagesum ();

    if (ac->mDoc->doctype () == AD_PDF)
      {
	PopplerPage *tpage;

	if (ac->mPagenum < 0
	    || ac->mPagenum >= c
	    || (tpage =
		poppler_document_get_page ((PopplerDocument *) ac->
					   mDoc->getdoc (),
					   ac->mPagenum)) == NULL)
	  {
	    debug ("no this page: %d", ac->mPagenum);
	    return;
	  }

	double tpagex, tpagey;
	ac->mDoc->getpagesize (tpage, &tpagex, &tpagey);

	int ix = (int) (tpagex * ac->mDoc->zoomvalue ()), iy =
	  (int) (tpagey * ac->mDoc->zoomvalue ());

	guchar *dat = new guchar[ix * iy * 3];

	GdkPixbuf *bu = gdk_pixbuf_new_from_data (dat, GDK_COLORSPACE_RGB,
						  FALSE,
						  8,
						  ix, iy,
						  3 * ix,
						  NULL, NULL);

	poppler_page_render_to_pixbuf (tpage, 0, 0, ix, iy,
				       ac->mDoc->zoomvalue (),
				       ac->mDoc->getrotate (), bu);

	if (ac->mLinkMappings)
	  {
	    poppler_page_free_link_mapping (ac->mLinkMappings);
	  }
	ac->mLinkMappings =
	  g_list_reverse (poppler_page_get_link_mapping (tpage));
	debug ("has mLinkMappings: %p", ac->mLinkMappings);

	ac->mPage = tpage;
	ac->mData = dat;
	ac->mBuf = bu;
      }
#ifdef HAVE_LIBDJVU
    else if (ac->mDoc->doctype () == AD_DJVU)
      {
	ddjvu_page_t *tpage;

	if (ac->mPagenum < 0
	    || ac->mPagenum >= c
	    || (tpage =
		ddjvu_page_create_by_pageno ((ddjvu_document_t *) ac->
					     mDoc->getdoc (),
					     ac->mPagenum)) == NULL)
	  {
	    debug ("no this page: %d", ac->mPagenum);
	    return;
	  }

	double tpagex, tpagey;
	ac->mDoc->getpagesize (ac->mPagenum, &tpagex, &tpagey);

	int ix = (int) (tpagex * ac->mDoc->zoomvalue ()), iy =
	  (int) (tpagey * ac->mDoc->zoomvalue ());

	char *dat = new char[ix * iy * 3];

	GdkPixbuf *bu =
	  gdk_pixbuf_new_from_data ((guchar *) dat, GDK_COLORSPACE_RGB,
				    FALSE,
				    8,
				    ix, iy,
				    3 * ix,
				    NULL, NULL);

	ddjvu_rect_t prect[1] = { {0, 0, tpagex, tpagey} };
	ddjvu_rect_t rrect[1] = { {0, 0, ix, iy} };
	debug ("for fender: ");
	ddjvu_format_t *format =
	  ddjvu_format_create (DDJVU_FORMAT_RGB24, 0, NULL);
	ddjvu_format_set_row_order (format, TRUE);
	while (ddjvu_page_render
	       (tpage, DDJVU_RENDER_COLOR, prect, rrect, format, 3 * ix,
		dat) == FALSE)
	  {
	    debug ("fender failed");
	  }

	/*  
	   if (ac->mLinkMappings)
	   {
	   poppler_page_free_link_mapping (ac->mLinkMappings);
	   }
	   ac->mLinkMappings =
	   g_list_reverse (poppler_page_get_link_mapping (tpage));
	   debug ("has mLinkMappings: %p", ac->mLinkMappings); */

	debug ("for fender: ");
	ac->mPage = (PopplerPage *) tpage;
	ac->mData = (guchar *) dat;
	ac->mBuf = bu;
      }
#endif
  }

  ApvlvDocCache::~ApvlvDocCache ()
  {
    if (mLinkMappings)
      {
	poppler_page_free_link_mapping (mLinkMappings);
      }

    if (mData != NULL)
      delete[]mData;
    if (mBuf != NULL)
      g_object_unref (mBuf);
  }

  PopplerPage *ApvlvDocCache::getpage ()
  {
    return mPage;
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

  GList *ApvlvDocCache::getlinks ()
  {
    return mLinkMappings;
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
	g_snprintf (temp[1], sizeof temp[1], "%d/%d", pn, mDoc->pagesum ());
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

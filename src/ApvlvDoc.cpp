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

#ifdef HAVE_LIBDJVU
#include <libdjvu/ddjvuapi.h>
#endif

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

    try
    {
      mFile = new ApvlvPDF (filename);
    }

    catch (bad_alloc e)
    {
      delete mFile;

      try
      {
	mFile = new ApvlvDJVU (filename);
      }
      catch (bad_alloc e)
      {
	debug ("");
	mFile = NULL;
	delete mFile;
      }
    }

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
      }

    return mFile == NULL ? false : true;
  }

  bool ApvlvDoc::writefile (const char *name)
  {
    if (mFile != NULL)
      {
	return mFile->writefile (name ? name : filename ());
      }
    return false;
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

	mFile->pagesize (0, mRotatevalue, &mPagex, &mPagey);

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
    ApvlvPos rect = (*mSearchResults)[mSearchSelect];

    debug ("zoomrate: %f", mZoomrate);

    // Caculate the correct position
    //debug ("pagex: %f, pagey: %f, x1: %f, y1: %f, x2: %f, y2: %f", mPagex, mPagey, rect->x1, rect->y1, rect->x2, rect->y2);
    gint x1 = (gint) ((rect.x) * mZoomrate);
    gint x2 = (gint) ((rect.w) * mZoomrate);
    gint y1 = (gint) ((mPagey - rect.h) * mZoomrate);
    gint y2 = (gint) ((mPagey - rect.y) * mZoomrate);
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
    for (ApvlvPoses::const_iterator itr = mSearchResults->begin ();
	 itr != mSearchResults->end (); itr++)
      {
	// Caculate the correct position
	x1 = (gint) ((itr->x) * mZoomrate);
	x2 = (gint) ((itr->w) * mZoomrate);
	y1 = (gint) ((mPagey - itr->h) * mZoomrate);
	y2 = (gint) ((mPagey - itr->y) * mZoomrate);

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

  bool ApvlvDoc::continuous ()
  {
    return mContinuous;
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

    int i =
      strlen (str) > 0 ? mPagenum : reverse ? mPagenum - 1 : mPagenum + 1;
    int sum = mFile->pagesum (), from = i;
    while (1)
      {
	mSearchResults = mFile->pagesearch ((i + sum) % sum, reverse);
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

  bool ApvlvDoc::totext (const char *file)
  {
    if (mFile == NULL)
      return false;

    char *txt;
    bool ret = mFile->pagetext (mPagenum, &txt);
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
    ApvlvLinks *links = mCurrentCache1->getlinks ();

    if (ct > 0 && ct < (int) links->size ())
      {
	markposition ('\'');

	ApvlvDocPosition p = { mPagenum, scrollrate () };
	mLinkPositions.push_back (p);

	showpage ((*links)[ct].mPage);
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
#if 0
    cairo_t *cr = gtk_print_context_get_cairo_context (context);
    PopplerPage *page =
      poppler_document_get_page (data->doc, data->frmpn + page_nr);
    poppler_page_render_for_printing (page, cr);

    PangoLayout *layout = gtk_print_context_create_pango_layout (context);
    pango_cairo_show_layout (cr, layout);
    g_object_unref (layout);
#endif
  }

  void
    ApvlvDoc::end_print (GtkPrintOperation * operation,
			 GtkPrintContext * context, PrintData * data)
  {
    delete data;
  }
#endif

  ApvlvDocCache::ApvlvDocCache (ApvlvFile * file)
  {
    mFile = file;
    mPagenum = -1;
    mData = NULL;
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

    int ix = (int) (tpagex * ac->mZoom), iy = (int) (tpagey * ac->mZoom);

    guchar *dat = new guchar[ix * iy * 3];

    GdkPixbuf *bu = gdk_pixbuf_new_from_data (dat, GDK_COLORSPACE_RGB,
					      FALSE,
					      8,
					      ix, iy,
					      3 * ix,
					      NULL, NULL);
    ac->mFile->render (ac->mPagenum, ix, iy, ac->mZoom, ac->mRotate, bu,
		       (char *) dat);

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

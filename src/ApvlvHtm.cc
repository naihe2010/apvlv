/*
 * This file is part of the apvlv package
 * Copyright (C) <2010>  <Alf>
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
/* @CPPFILE ApvlvHtm.cc
 *
 *  Author: Alf <naihe2010@126.com>
 */
/* @date Created: 2011/09/16 13:51:04 Alf*/

#include "ApvlvUtil.h"
#include "ApvlvView.h"
#include "ApvlvHtm.h"

#include <gdk/gdkkeysyms.h>
#include <webkit/webkit.h>

namespace apvlv
{
  ApvlvHTML::ApvlvHTML (int w, int h)
  {
    mAdjInchg = false;

    mZoomrate = 1.0;

    mLines = 50;
    mChars = 80;

    mProCmd = 0;

    mRotatevalue = 0;

    mFile = NULL;

    mSearchResults = NULL;
    mSearchStr = "";

    mReady = false;

    mProCmd = 0;

    mRotatevalue = 0;

    mHtmlView = GTK_WIDGET (apvlv_html_new_webview ());
    gtk_scrolled_window_add_with_viewport (mScrollwin, mHtmlView);

    mStatus = new ApvlvHTMLStatus (this);

    gtk_box_pack_start (GTK_BOX (mVbox), mScrollwin, FALSE, FALSE, 0);
    gtk_box_pack_end (GTK_BOX (mVbox), mStatus->widget (), FALSE, FALSE, 0);

    gtk_widget_show_all (mVbox);

    setsize (w, h);
  }

  bool ApvlvHTML::reload ()
  {
    return loadfile (filename (), false);
  }

  bool ApvlvHTML::loadfile (const char *path, bool check)
  {
    gchar * uri;
    if (g_ascii_strncasecmp (path, "http://", 7) == 0)
      {
	uri = g_strdup (path);
      }
    else
      {
	uri = g_filename_to_uri (path, NULL, NULL);
      }

    if (uri)
      {
	webkit_web_view_load_uri (WEBKIT_WEB_VIEW (mHtmlView), uri);
	g_free (uri);
      }
    else
      {
	debug ("convert '%s' uri failed\n", path);
      }

    mFilestr = path;

    mStatus->show ();

    return true;
  }

  ApvlvHTML::~ApvlvHTML ()
  {
    delete mStatus;
  }

  returnType ApvlvHTML::subprocess (int ct, guint key)
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

  returnType ApvlvHTML::process (int has, int ct, guint key)
  {
    if (mProCmd != 0)
      {
	return subprocess (ct, key);
      }

    if (!has)
      {
	ct = 1;
      }

    switch (key)
      {
      case GDK_KEY_Page_Down:
      case CTRL ('f'):
	webkit_web_view_go_forward (WEBKIT_WEB_VIEW (mHtmlView));
      break;
      case GDK_KEY_Page_Up:
      case CTRL ('b'):
	webkit_web_view_go_back (WEBKIT_WEB_VIEW (mHtmlView));
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
      case '0':
	scrollleft (mChars);
	break;
      case '$':
	scrollright (mChars);
	break;
      case CTRL ('p'):
      case GDK_KEY_Up:
      case 'k':
	scrollup (ct);
      break;
      case CTRL ('n'):
      case CTRL ('j'):
      case GDK_KEY_Down:
      case 'j':
	scrolldown (ct);
      break;
      case GDK_KEY_BackSpace:
      case GDK_KEY_Left:
      case CTRL ('h'):
      case 'h':
	scrollleft (ct);
	break;
      case GDK_KEY_space:
      case GDK_KEY_Right:
      case CTRL ('l'):
      case 'l':
	scrollright (ct);
	break;
      case 'R':
	webkit_web_view_reload (WEBKIT_WEB_VIEW (mHtmlView));
	break;
      case 't':
	gView->newtab (helppdf.c_str ());
	gView->open ();
	break;
      case 'T':
	gView->newtab (helppdf.c_str ());
	gView->opendir ();
	break;
      case 'o':
	gView->open ();
	break;
      case 'O':
	gView->opendir ();
	break;
      case 'G':
	markposition ('\'');
	if (has)
	  {
	    ct += mSkip;
	    showpage (ct - 1);
	  }
	else
	  {
	    showpage (-1);
	  }
	break;
      case 'm':
      case '\'':
      case 'z':
	mProCmd = key;
	return NEED_MORE;
	break;
      case 'n':
	if (mSearchCmd == SEARCH)
	  {
	    markposition ('\'');
	    search ("");
	  }
	else if (mSearchCmd == BACKSEARCH)
	  {
	    markposition ('\'');
	    search ("", true);
	  }
	else
	  {
	  }
	break;
      case 'N':
	if (mSearchCmd == SEARCH)
	  {
	    markposition ('\'');
	    search ("", true);
	  }
	else if (mSearchCmd == BACKSEARCH)
	  {
	    markposition ('\'');
	    search ("");
	  }
	else
	  {
	  }
	break;
      default:
	return NO_MATCH;
	break;
      }

    return MATCH;
  }

  bool ApvlvHTML::search (const char *str, bool reverse)
  {
    if (!mReady)
      return false;

    if (*str == '\0' && mSearchStr == "")
      {
	return false;
      }

    if (*str != '\0')
      {
	mSearchStr = str;
      }

    mStatus->show ();
    return true;
  }

  void ApvlvHTML::setactive (bool act)
  {
    mStatus->active (act);
    mActive = act;
  }

  ApvlvHTMLStatus::ApvlvHTMLStatus (ApvlvHTML * doc)
  {
    mDoc = doc;
    for (int i = 0; i < AD_STATUS_SIZE; ++i)
      {
	mStlab[i] = gtk_label_new ("");
	gtk_box_pack_start (GTK_BOX (mHbox), mStlab[i], FALSE, FALSE, 0);
      }
  }

  ApvlvHTMLStatus::~ApvlvHTMLStatus ()
  {
  }

  void ApvlvHTMLStatus::active (bool act)
  {
    for (unsigned int i = 0; i < AD_STATUS_SIZE; ++i)
      {
#if GTK_CHECK_VERSION(3, 0, 0)
        gtk_widget_override_color (mStlab[i],
                                   (act)? GTK_STATE_FLAG_ACTIVE:
                                   GTK_STATE_FLAG_INSENSITIVE, NULL);
#else
	gtk_widget_modify_fg (mStlab[i],
			      (act) ? GTK_STATE_ACTIVE :
			      GTK_STATE_INSENSITIVE, NULL);
#endif
      }
  }

  void ApvlvHTMLStatus::setsize (int w, int h)
  {
    int sw[AD_STATUS_SIZE];
    sw[0] = w >> 1;
    sw[1] = sw[0] >> 1;
    sw[2] = sw[1] >> 1;
    sw[3] = sw[1] >> 1;
    for (unsigned int i = 0; i < AD_STATUS_SIZE; ++i)
      {
	gtk_widget_set_size_request (mStlab[i], sw[i], h);
      }
  }

  void ApvlvHTMLStatus::show ()
  {
    if (mDoc->filename ())
      {
	char temp[AD_STATUS_SIZE][256];
	gchar *bn;
	bn = g_path_get_basename (mDoc->filename ());
	g_snprintf (temp[0], sizeof temp[0], "%s", bn);
	g_snprintf (temp[1], sizeof temp[1], "apvlv");
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

  void ApvlvHTML::apvlv_dir_monitor_callback (GFileMonitor *gfm, GFile *gf1, GFile *gf2, GFileMonitorEvent ev, ApvlvHTML *dir)
  {
    if (dir->mType == CORE_CONTENT
	&& ev == G_FILE_MONITOR_EVENT_CHANGED)
      {
	gView->errormessage ("Contents is modified, apvlv reload it automatically");
	dir->reload ();
      }
  }

  void ApvlvHTML::setzoom (const char *z)
  {
    if (z != NULL)
      {
	if (strcasecmp (z, "normal") == 0)
	  {
	    mZoommode = NORMAL;
	    mZoomrate = 1.2;
	  }
	/*
	  else if (strcasecmp (z, "fitwidth") == 0)
	  {
	  mZoommode = FITWIDTH;
	  }
	  else if (strcasecmp (z, "fitheight") == 0)
	  {
	  mZoommode = FITHEIGHT;
	  }*/
	else
	  {
	    double d = atof (z);
	    if (d > 0)
	      {
		mZoommode = CUSTOM;
		mZoomrate = d;
	      }

	    webkit_web_view_set_zoom_level (WEBKIT_WEB_VIEW (mHtmlView), mZoomrate);
	    mStatus->show ();
	  }
      }
  }

  GtkWidget * ApvlvHTML::apvlv_html_new_webview ()
  {
    GtkWidget *view = webkit_web_view_new ();
    webkit_web_view_set_editable (WEBKIT_WEB_VIEW (view), FALSE);
    webkit_web_view_set_zoom_level (WEBKIT_WEB_VIEW (view), mZoomrate);
    webkit_web_view_set_full_content_zoom (WEBKIT_WEB_VIEW (view), TRUE);

    gtk_widget_add_events (GTK_WIDGET (view), GDK_BUTTON_PRESS_MASK);
    g_signal_connect (G_OBJECT (view), "create-web-view", G_CALLBACK (apvlv_html_clicked_cb), this);
    g_signal_connect (G_OBJECT (view), "load-finished", G_CALLBACK (apvlv_html_ready_cb), this);

    return view;
  }

  gboolean
  ApvlvHTML::apvlv_html_replace_webview (WebKitWebView *view, ApvlvHTML *htm)
  {
    gtk_container_remove (GTK_CONTAINER (htm->mScrollwin), htm->mHtmlView);
    gtk_scrolled_window_add_with_viewport (htm->mScrollwin, GTK_WIDGET (view));
    htm->mHtmlView = GTK_WIDGET (view);
    gtk_widget_show_all (htm->mScrollwin);
    return TRUE;
  }

  WebKitWebView *
  ApvlvHTML::apvlv_html_clicked_cb (WebKitWebView *wid, WebKitWebFrame *frame, ApvlvHTML *htm)
  {
    debug ("clicked OK\n");
    GtkWidget *view = htm->apvlv_html_new_webview ();
    g_signal_connect (G_OBJECT (view), "web-view-ready", G_CALLBACK (apvlv_html_replace_webview), htm);
    return WEBKIT_WEB_VIEW (view);
  }

  gboolean
  ApvlvHTML::apvlv_html_ready_cb (WebKitWebView *wid, WebKitWebFrame *frame, ApvlvHTML *htm)
  {
    debug ("load OK\n");
    htm->mReady = TRUE;
    return TRUE;
  }

}

// Local Variables:
// mode: c++
// End:

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
/* @CFILE ApvlvHtm.h
 *
 *  Author: Alf <naihe2010@126.com>
 */
/* @date Created: 2011/09/16 13:50:49 Alf*/

#ifndef _APVLV_HTM_H_
#define _APVLV_HTM_H_

#include "ApvlvCore.h"

#include <gtk/gtk.h>
#include <webkit/webkit.h>

namespace apvlv
{
  class ApvlvHTML;
  class ApvlvHTMLStatus:public ApvlvCoreStatus
  {
  public:
    ApvlvHTMLStatus (ApvlvHTML *);

    ~ApvlvHTMLStatus ();

    void active (bool act);

    void setsize (int, int);

    void show ();

  private:
    ApvlvHTML * mDoc;
#define AD_STATUS_SIZE   4
    GtkWidget *mStlab[AD_STATUS_SIZE];
  };

  class ApvlvHTML:public ApvlvCore
  {
  public:
    ApvlvHTML (int w, int h);

    ~ApvlvHTML ();

    bool loadfile (const char *file, bool check = true);

    void setactive (bool act);

    returnType process (int hastimes, int times, guint keyval);

  private:

    returnType subprocess (int ct, guint key);

    bool reload ();

    void setzoom (const char *z);

    bool search (const char *str, bool reverse = false);

    GtkWidget * apvlv_html_new_webview ();

    static void apvlv_dir_on_changed (GtkTreeSelection *, ApvlvHTML *);

    static void apvlv_dir_monitor_callback (GFileMonitor *, GFile *, GFile *, GFileMonitorEvent, ApvlvHTML *);

    static gboolean apvlv_html_replace_webview (WebKitWebView *wid, ApvlvHTML *htm);
    static WebKitWebView * apvlv_html_clicked_cb (WebKitWebView *wid, WebKitWebFrame *frame, ApvlvHTML *htm);
    static gboolean apvlv_html_ready_cb (WebKitWebView *wid, WebKitWebFrame *frame, ApvlvHTML *htm);

    GtkWidget *mHtmlView;
  };

}

#endif

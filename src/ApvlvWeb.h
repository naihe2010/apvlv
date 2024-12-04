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
/* @CPPFILE ApvlvWeb.h
 *
 *  Author: Alf <naihe2010@126.com>
 */

#ifndef _APVLV_WEB_H_
#define _APVLV_WEB_H_

#include <QUrl>

#include "ApvlvFile.h"
#include "ApvlvWebViewWidget.h"

namespace apvlv
{
class ApvlvWEB : public File
{
public:
  ApvlvWEB () = default;

  [[nodiscard]] DISPLAY_TYPE
  getDisplayType () const override
  {
    return DISPLAY_TYPE::CUSTOM;
  }

  FileWidget *
  getWidget () override
  {
    auto wid = new WebViewWidget ();
    wid->setFile (this);
    wid->setInternalScroll (true);
    return wid;
  }

  bool load (const std::string &filename) override;

  bool pageRenderToWebView (int pn, double zm, int rot,
                            WebView *webview) override;

protected:
  QUrl mUrl;
};

}

#endif

/* Local Variables: */
/* mode: c++ */
/* End: */

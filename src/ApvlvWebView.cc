/*
 * This file is part of the apvlv package
 *
 * Copyright (C) 2008 Alf.
 *
 * Contact: Alf <naihe2010@126.com>
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
/* @CPPFILE ApvlvCore.cc
 *
 *  Author: Alf <naihe2010@126.com>
 */

#include <QBuffer>
#include <QWebEnginePage>
#include <QWebEngineProfile>
#include <filesystem>
#include <iostream>

#include "ApvlvFile.h"
#include "ApvlvWebView.h"

namespace apvlv
{

void
ApvlvSchemeHandler::requestStarted (QWebEngineUrlRequestJob *job)
{
  auto url = job->requestUrl ();
  auto path = url.path ().toStdString ();
  auto key = path.substr (1);
  auto mime = mFile->pathMimeType (key);
  auto roptcont = mFile->pathContent (key);
  if (!roptcont)
    {
      job->fail (QWebEngineUrlRequestJob::UrlNotFound);
      return;
    }

  mArray = std::move (roptcont.value ());
  mBuffer.setData (mArray);
  job->reply (QByteArray (mime.c_str ()), &mBuffer);

  emit webpageUpdated (key);
}

ApvlvWebview::ApvlvWebview ()
{
  auto profile = new QWebEngineProfile ();
  profile->setHttpCacheType (QWebEngineProfile::NoCache);
  mSchemeHandler = make_unique<ApvlvSchemeHandler> ();
  profile->installUrlSchemeHandler ("apvlv", mSchemeHandler.get ());

  mPage = make_unique<QWebEnginePage> (profile);
  setPage (mPage.get ());

  QObject::connect (this, SIGNAL (loadFinished (bool)), this,
                    SLOT (webview_load_finished (bool)));
  QObject::connect (mSchemeHandler.get (),
                    SIGNAL (webpageUpdated (const string &)), this,
                    SLOT (webview_update (const string &)));
}

}

// Local Variables:
// mode: c++
// End:

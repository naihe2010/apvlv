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
/* @CPPFILE ApvlvWebViewWidget.cc
 *
 *  Author: Alf <naihe2010@126.com>
 */

#include <QBuffer>
#include <QWebEnginePage>
#include <QWebEngineProfile>
#include <filesystem>
#include <iostream>
#include <sstream>

#include "ApvlvWebViewWidget.h"

namespace apvlv
{

using namespace std;

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

WebView::WebView ()
{
  mProfile.setHttpCacheType (QWebEngineProfile::NoCache);
  mProfile.installUrlSchemeHandler ("apvlv", &mSchemeHandler);
  mPage = make_unique<QWebEnginePage> (&mProfile);
  setPage (mPage.get ());
}

void
WebViewWidget::showPage (int p, double s)
{
  mFile->pageRenderToWebView (p, mZoomrate, 0, &mWebView);
  mPageNumber = p;
}

void
WebViewWidget::showPage (int p, const string &anchor)
{
  mFile->pageRenderToWebView (p, mZoomrate, 0, &mWebView);
  mPageNumber = p;
  mAnchor = anchor;
}

void
WebViewWidget::scroll (int times, int h, int v)
{
  if (!mFile)
    return;

  stringstream scripts;
  scripts << "window.scrollBy(" << times * h << "," << times * v << ")";
  auto page = mWebView.page ();
  page->runJavaScript (QString::fromLocal8Bit (scripts.str ()));
}

void
WebViewWidget::scrollTo (double xrate, double yrate)
{
  if (!mFile)
    return;

  stringstream scripts;
  scripts << "window.scroll(window.screenX * " << xrate << ",";
  scripts << " (document.body.offsetHeight - window.innerHeight) * " << yrate
          << ");";
  auto page = mWebView.page ();
  page->runJavaScript (QString::fromLocal8Bit (scripts.str ()));
}

void
WebViewWidget::scrollUp (int times)
{
  scroll (times, 0, -50);

  if (mWebView.isScrolledToTop ())
    {
      if (mIsInternalScroll)
        {
          // clang-format off
          auto rs = R"(
            var event = document.createEvent ('Event');
            event.initEvent('keydown', true, true);
            event.keyCode = 37;
            document.dispatchEvent(event);
          )";
          // clang-format on

          auto page = mWebView.page ();
          page->runJavaScript (
              QString::fromLocal8Bit (rs),
              [] (const QVariant &v) { qDebug () << v.toString (); });
        }
      else
        {
          auto p = mFile->pageNumberWrap (mPageNumber - 1);
          if (p >= 0)
            {
              mIsScrollUp = true;
              showPage (p, 0.0);
            }
        }
    }
}

void
WebViewWidget::scrollDown (int times)
{
  scroll (times, 0, 50);

  if (mWebView.isScrolledToBottom ())
    {
      if (mIsInternalScroll)
        {
          // clang-format off
          auto rs = R"(
              var event = document.createEvent ('Event');
              event.initEvent('keydown', true, true);
              event.keyCode = 39;
              document.dispatchEvent(event);
          )";
          // clang-format on
          auto page = mWebView.page ();
          page->runJavaScript (
              QString::fromLocal8Bit (rs),
              [] (const QVariant &v) { qDebug () << v.toString (); });
        }
      else
        {
          auto p = mFile->pageNumberWrap (mPageNumber + 1);
          if (p >= 0)
            showPage (p, 0.0);
        }
    }
}

void
WebViewWidget::scrollLeft (int times)
{
  scroll (times, -50, 0);
}

void
WebViewWidget::scrollRight (int times)
{
  scroll (times, 50, 0);
}

void
WebViewWidget::setSearchStr (const string &str)
{
  auto qstr = QString::fromLocal8Bit (str);
  QWebEnginePage::FindFlags flags{};
  mWebView.findText (qstr, flags);
}

void
WebViewWidget::setSearchSelect (int select)
{
  auto text = mWebView.selectedText ();
  QWebEnginePage::FindFlags flags{};
  mWebView.findText (text, flags);
  mSearchSelect = select;
}

bool
WebView::isScrolledToTop ()
{
  auto page = this->page ();
  auto p = page->scrollPosition ();
  return p.y () < 0.5;
}

bool
WebView::isScrolledToBottom ()
{
  auto page = this->page ();
  auto p = page->scrollPosition ();
  auto cs = page->contentsSize ();
  return p.y () + QWebEngineView::height () + 0.5 > cs.height ();
}

void
WebViewWidget::webviewLoadFinished (bool suc)
{
  if (suc)
    {
      if (!mAnchor.empty ())
        {
          auto page = mWebView.page ();
          stringstream javasrc;
          javasrc << "document.getElementById('";
          javasrc << mAnchor.substr (1);
          javasrc << "').scrollIntoView();";
          page->runJavaScript (QString::fromLocal8Bit (javasrc.str ()));
        }
      else if (mIsScrollUp)
        {
          scrollTo (0.0, 1.0);
          mIsScrollUp = false;
        }
    }
}

void
WebViewWidget::webviewFindTextFinished (const QWebEngineFindTextResult &result)
{
  mSearchResult = result;
}

}

// Local Variables:
// mode: c++
// End:

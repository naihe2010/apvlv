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

#include <filesystem>
#include <iostream>
#include <sstream>

#include <QClipboard>
#include <QFile>
#include <QInputDialog>
#include <QWebEngineScriptCollection>
#include <qevent.h>

#include "ApvlvUtil.h"
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

  mCopyAction.setText (tr ("Copy"));
  QObject::connect (&mCopyAction, SIGNAL (triggered (bool)), this,
                    SLOT (copy ()));
  mMenu.addAction (&mCopyAction);

  mUnderlineAction.setText (tr ("Underline"));
  QObject::connect (&mUnderlineAction, SIGNAL (triggered (bool)), this,
                    SLOT (underline ()));
  mMenu.addAction (&mUnderlineAction);

  mCommentAction.setText (tr ("Comment"));
  QObject::connect (&mCommentAction, SIGNAL (triggered (bool)), this,
                    SLOT (comment ()));
  mMenu.addAction (&mCommentAction);
}

WebViewWidget::WebViewWidget ()
{
  QObject::connect (&mWebView, SIGNAL (loadFinished (bool)), this,
                    SLOT (webviewLoadFinished (bool)));
  QObject::connect (&mWebView.mSchemeHandler,
                    SIGNAL (webpageUpdated (const string &)), this,
                    SLOT (webviewUpdate (const string &)));
  loadJavaScriptFromDir (ScriptDir);
}

bool
WebViewWidget::loadJavaScriptFromDir (const std::string &dir)
{
  std::filesystem::path script_dir{ dir };
  if (!is_directory (script_dir))
    return false;

  QList<QWebEngineScript> script_list;
  for (auto &entry : std::filesystem::directory_iterator (
           script_dir,
           std::filesystem::directory_options::follow_directory_symlink))
    {
      auto const &path = entry.path ();
      if (entry.is_regular_file () && path.extension () == ".js")
        {
          QFile file (path);
          if (!file.open (QIODevice::ReadOnly | QIODevice::Text))
            {
              qWarning () << "open " << path.c_str () << "to read error";
              continue;
            }

          auto contents = file.readAll ();

          auto script = QWebEngineScript{};
          script.setName (QString::fromStdString (path.filename ()));
          script.setSourceCode (contents);
          script.setInjectionPoint (QWebEngineScript::DocumentCreation);
          script.setWorldId (QWebEngineScript::MainWorld);
          script.setRunsOnSubFrames (true);
          script_list.push_back (script);
        }
    }

  auto page = mWebView.page ();
  page->scripts ().insert (script_list);
  return true;
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

  auto scripts
      = QString ("scrollByTimes(%1, %2, %3);").arg (times).arg (h).arg (v);
  auto page = mWebView.page ();
  page->runJavaScript (scripts);
}

void
WebViewWidget::scrollTo (double xrate, double yrate)
{
  if (!mFile)
    return;

  auto scripts
      = QString ("scrollToPosition(%1, %2);").arg (xrate).arg (yrate);
  auto page = mWebView.page ();
  page->runJavaScript (scripts);
}

void
WebViewWidget::scrollUp (int times)
{
  scroll (times, 0, -50);

  if (mWebView.isScrolledToTop ())
    {
      if (mIsInternalScroll)
        {
          auto scripts = QString ("dispatchKeydownEvent(%1);").arg (37);
          auto page = mWebView.page ();
          page->runJavaScript (scripts);
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
          auto scripts = QString ("dispatchKeydownEvent(%1);").arg (39);
          auto page = mWebView.page ();
          page->runJavaScript (scripts);
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

void
WebViewWidget::setZoomrate (double zm)
{
  mWebView.setZoomFactor (zm);
  mZoomrate = zm;
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
WebView::contextMenuEvent (QContextMenuEvent *event)
{
  qDebug () << "WebView::customeMenuRequest";
  mMenu.popup (mapToGlobal (event->pos ()));
}

std::pair<int, int>
WebView::getSelectionPosition () const
{
  int begin = -1;
  int end = -1;
  QEventLoop loop;
  mPage->runJavaScript (
      "getSelectionOffset(0);",
      [&loop, &begin, &end] (const QVariant &result)
        {
          if (result.isValid ()
              && result.typeId () == QMetaType::QVariantList)
            {
              auto offsets = result.toList ();
              begin = offsets[0].toInt ();
              end = offsets[1].toInt ();
              qDebug () << "Begin offset:" << offsets[0].toInt ();
              qDebug () << "End offset:" << offsets[1].toInt ();
              loop.quit ();
            }
        });
  loop.exec ();
  return std::make_pair (begin, end);
}

void
WebView::underLinePosition (int begin, int end, const std::string &tooltip)
{
  qDebug () << "underLinePosition" << begin << " -> " << end;
  QString src = QString ("underlineByOffset(%1, %2, '%3');")
                    .arg (begin)
                    .arg (end)
                    .arg (tooltip);
  mPage->runJavaScript (src);
}

void
WebView::copy () const
{
  qDebug () << "copy text";
  auto text = mPage->selectedText ();
  auto clipboard = QGuiApplication::clipboard ();
  clipboard->setText (text);
}

void
WebView::underline ()
{
  qDebug () << "underline text";
  auto text = mPage->selectedText ();
  if (text.isEmpty ())
    {
      return;
    }

  auto offset = getSelectionPosition ();
  if (offset.first < 0 || offset.second < 0)
    {
      return;
    }

  auto path = mPage->url ().path ().toStdString ();
  auto file = mSchemeHandler.file ();
  auto note = file->getNote ();
  Comment comment;
  comment.quoteText = text.toStdString ();
  comment.begin.set (0, nullptr, offset.first, path);
  comment.end.set (0, nullptr, offset.second, path);
  note->addComment (comment);
}

void
WebView::comment ()
{
  qDebug () << "comment text";
  do
    {
      auto text = mPage->selectedText ();
      if (text.isEmpty ())
        break;

      auto input_text
          = QInputDialog::getText (this, tr ("Input"), tr ("Comment"));
      auto commentText = input_text.trimmed ();
      if (commentText.isEmpty ())
        break;

      commentText = commentText.toHtmlEscaped ();

      auto offset = getSelectionPosition ();
      if (offset.first < 0 || offset.second < 0)
        break;

      auto path = mPage->url ().path ().toStdString ();
      auto file = mSchemeHandler.file ();
      auto note = file->getNote ();
      Comment comment;
      comment.quoteText = text.toStdString ();
      comment.commentText = commentText.toStdString ();
      comment.begin.set (0, nullptr, offset.first, path);
      comment.end.set (0, nullptr, offset.second, path);
      note->addComment (comment);
    }
  while (false);
}

void
WebViewWidget::setComment ()
{
  auto note = mFile->getNote ();
  if (!note)
    return;
  auto path = mWebView.page ()->url ().path ().toStdString ();
  auto comments = note->getCommentsInPath (path);
  for (auto &comment : comments)
    {
      auto begin_offset = comment.begin.offset;
      auto end_offset = comment.end.offset;
      mWebView.underLinePosition (begin_offset, end_offset,
                                  comment.commentText);
    }
}

void
WebViewWidget::webviewLoadFinished (bool suc)
{
  if (suc)
    {
      if (!mAnchor.empty ())
        {
          auto page = mWebView.page ();
          auto scripts
              = QString ("scrollToAnchor('%1');").arg (mAnchor.c_str ());
          page->runJavaScript (scripts);
        }
      else if (mIsScrollUp)
        {
          scrollTo (0.0, 1.0);
          mIsScrollUp = false;
        }

      if (!mIsScrollUp)
        {
          setComment ();
        }
    }
}

void
WebViewWidget::webviewFindTextFinished (
    const QWebEngineFindTextResult &result)
{
  mSearchResult = result;
}
}

// Local Variables:
// mode: c++
// End:

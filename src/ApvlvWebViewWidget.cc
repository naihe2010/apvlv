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
#include <filesystem>
#include <iostream>
#include <sstream>

#include "ApvlvWebViewWidget.h"

#include <QClipboard>
#include <QInputDialog>
#include <qevent.h>

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
      R"(
    var selection = window.getSelection();
    if (selection.rangeCount > 0) {
      let range = selection.getRangeAt(0);

      let start = document.createRange();
      start.setStart(document.documentElement, 0);
      start.setEnd(range.startContainer, range.startOffset);
      let startOffset = start.toString().length;

      let end = document.createRange();
      end.setStart(document.documentElement, 0);
      end.setEnd(range.endContainer, range.endOffset);
      let endOffset = end.toString().length;

      [startOffset, endOffset];
    } else {
      [null, null];
    }
  )",
      [&loop, &begin, &end] (const QVariant &result) {
        if (result.isValid () && result.typeId () == QMetaType::QVariantList)
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
WebView::underLinePosition (int begin, int end)
{
  qDebug () << "underLinePosition";
  QString src = QString(
      R"(
function underlineTextByOffset(startOffset, endOffset) {
    // 获取文档的根元素
    const root = document.documentElement;
    let currentOffset = 0;
    let startNode = null;
    let endNode = null;
    let startNodeOffset = 0;
    let endNodeOffset = 0;

    // 递归遍历文档树，找到起始和结束节点
    function traverse(node) {
        if (node.nodeType === Node.TEXT_NODE) {
            const nodeLength = node.nodeValue.length;
            if (currentOffset <= startOffset && currentOffset + nodeLength > startOffset) {
                startNode = node;
                startNodeOffset = startOffset - currentOffset;
            }
            if (currentOffset <= endOffset && currentOffset + nodeLength > endOffset) {
                endNode = node;
                endNodeOffset = endOffset - currentOffset;
            }
            currentOffset += nodeLength;
        }
        for (let i = 0; i < node.childNodes.length; i++) {
            traverse(node.childNodes[i]);
        }
    }

    traverse(root);

    if (startNode && endNode) {
        if (startNode === endNode) {
            // 起始和结束节点相同
            const text = startNode.nodeValue;
            const beforeText = text.slice(0, startNodeOffset);
            const underlinedText = text.slice(startNodeOffset, endNodeOffset);
            const afterText = text.slice(endNodeOffset);

            const u = document.createElement('u');
            u.textContent = underlinedText;

            const parent = startNode.parentNode;
            const newBeforeText = document.createTextNode(beforeText);
            const newAfterText = document.createTextNode(afterText);

            parent.insertBefore(newBeforeText, startNode);
            parent.insertBefore(u, startNode);
            parent.insertBefore(newAfterText, startNode);
            parent.removeChild(startNode);
        } else {
            // 起始和结束节点不同
            // 处理起始节点
            const startText = startNode.nodeValue;
            const startBeforeText = startText.slice(0, startNodeOffset);
            const startUnderlinedText = startText.slice(startNodeOffset);

            const startU = document.createElement('u');
            startU.textContent = startUnderlinedText;

            const startParent = startNode.parentNode;
            const startNewBeforeText = document.createTextNode(startBeforeText);

            startParent.insertBefore(startNewBeforeText, startNode);
            startParent.insertBefore(startU, startNode);
            startParent.removeChild(startNode);

            // 处理中间节点
            let currentNode = startU.nextSibling;
            while (currentNode && currentNode!== endNode) {
                const nextNode = currentNode.nextSibling;
                const u = document.createElement('u');
                u.textContent = currentNode.nodeValue;
                currentNode.parentNode.replaceChild(u, currentNode);
                currentNode = nextNode;
            }

            // 处理结束节点
            const endText = endNode.nodeValue;
            const endUnderlinedText = endText.slice(0, endNodeOffset);
            const endAfterText = endText.slice(endNodeOffset);

            const endU = document.createElement('u');
            endU.textContent = endUnderlinedText;

            const endParent = endNode.parentNode;
            const endNewAfterText = document.createTextNode(endAfterText);

            endParent.insertBefore(endU, endNode);
            endParent.insertBefore(endNewAfterText, endNode);
            endParent.removeChild(endNode);
        }
    }
}

underlineTextByOffset(%1, %2);
      )").arg(begin).arg(end);

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

      auto input_text = QInputDialog::getMultiLineText (this, tr ("Input"),
                                                        tr ("Comment"));
      auto commentText = input_text.trimmed ();
      if (commentText.isEmpty ())
        break;

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
      mWebView.underLinePosition (begin_offset, end_offset);
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

      if (!mIsScrollUp)
        {
          setComment ();
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

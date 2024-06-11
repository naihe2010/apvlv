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
/* @date Created: 2009/01/04 09:34:51 Alf*/

#include <QLineEdit>
#include <QScrollArea>
#include <QSplitter>
#include <QWebEngineProfile>
#include <QWebEngineView>
#include <filesystem>
#include <iostream>
#include <sstream>

#include "ApvlvCore.h"
#include "ApvlvParams.h"
#include "ApvlvView.h"

namespace apvlv
{
ApvlvCore::ApvlvCore (ApvlvView *view)
{
  mInuse = true;

  mView = view;

  mReady = false;

  mProCmd = 0;

  mZoomrate = 1.0;

  mZoommode = NORMAL;

  mRotatevalue = 0;

  mSearchResults = nullptr;
  mSearchStr = "";

  mVbox = new QVBoxLayout ();
  setLayout (mVbox);

  mPaned = new QSplitter ();
  mPaned->setHandleWidth (4);

  auto f_width = gParams->valuei ("fix_width");
  auto f_height = gParams->valuei ("fix_height");

  if (f_width > 0 && f_height > 0)
    {
      mPaned->setFixedSize (f_width, f_height);

      auto hbox = new QHBoxLayout ();
      hbox->addWidget (mPaned, 0);
      mVbox->addLayout (hbox, 1);
    }
  else
    {
      mVbox->addWidget (mPaned, 1);
    }

  mContent = new ApvlvContent ();
  QObject::connect (this, SIGNAL (indexGenerited (const ApvlvFileIndex &)),
                    mContent, SLOT (setIndex (const ApvlvFileIndex &)));

  mPaned->addWidget (mContent);

  initImageUi ();

  initWebUi ();

  showImage ();

  mStatus = new ApvlvStatus ();
  mVbox->addWidget (mStatus, 0);
  debug ("ApvlvCore: %p be created", this);
}

ApvlvCore::~ApvlvCore ()
{
  debug ("ApvlvCore: %p be freed", this);
  if (mMainImageScrolView->parent () == nullptr)
    mMainImageScrolView->deleteLater ();
  if (mMainWebView->parent () == nullptr)
    mMainWebView->deleteLater ();
}

bool
ApvlvCore::reload ()
{
  return true;
}

void
ApvlvCore::inuse (bool use)
{
  mInuse = use;
}

bool
ApvlvCore::inuse ()
{
  return mInuse;
}

ApvlvCore *
ApvlvCore::copy ()
{
  return nullptr;
}

const char *
ApvlvCore::filename ()
{
  return mFilestr.empty () ? nullptr : mFilestr.c_str ();
}

int
ApvlvCore::pagenumber ()
{
  return mPagenum + 1;
}

double
ApvlvCore::zoomvalue ()
{
  return mZoomrate;
}

ApvlvFile *
ApvlvCore::file ()
{
  return mFile;
}

bool
ApvlvCore::writefile (const char *name)
{
  if (mFile != nullptr)
    {
      debug ("write %p to %s", mFile, name);
      return mFile->writefile (name ? name : filename ());
    }
  return false;
}

bool
ApvlvCore::loadfile (const string &file, bool check, bool show_content)
{
  return false;
}

void
ApvlvCore::showpage (int p, double s)
{
}

void
ApvlvCore::showpage (int, const string &anchor)
{
}

void
ApvlvCore::refresh ()
{
}

double
ApvlvCore::scrollrate ()
{
  if (mMainVaj == nullptr)
    return 0.0;

  double maxv
      = mMainVaj->maximum () - mMainVaj->minimum () - mMainVaj->height ();
  double val = mMainVaj->value () / maxv;
  if (val > 1.0)
    {
      return 1.00;
    }
  else if (val > 0.0)
    {
      return val;
    }
  else
    {
      return 0.00;
    }
}

bool
ApvlvCore::scrollto (double s)
{
  if (!mReady)
    return false;

  auto maxv = mMainVaj->maximum () - mMainVaj->minimum ();
  auto val = static_cast<double> (maxv) * s;
  mMainVaj->setValue (val);
  display ();
  return true;
}

void
ApvlvCore::scrollweb (int times, int h, int v)
{
  if (!mReady)
    return;

  stringstream scripts;
  scripts << "window.scrollBy(" << times * h << "," << times * v << ")";
  auto page = mMainWebView->page ();
  page->runJavaScript (QString::fromStdString (scripts.str ()));
}

void
ApvlvCore::scrollwebto (double xrate, double yrate)
{
  if (!mReady)
    return;

  stringstream scripts;
  scripts << "window.scroll(window.screenX * " << xrate << ",";
  scripts << " (document.body.offsetHeight - window.innerHeight) * " << yrate
          << ");";
  auto page = mMainWebView->page ();
  page->runJavaScript (QString::fromStdString (scripts.str ()));
}

void
ApvlvCore::scrollupweb (int times)
{
  scrollweb (times, 0, -50);

  if (mAutoScrollPage && webIsScrolledToTop ())
    {
      mWebScrollUp = true;
      showpage (mPagenum - 1, 0.0);
    }
}

void
ApvlvCore::scrolldownweb (int times)
{
  scrollweb (times, 0, 50);

  if (mAutoScrollPage && webIsScrolledToBottom ())
    {
      showpage (mPagenum + 1, 0.0);
    }
}

void
ApvlvCore::scrollleftweb (int times)
{
  scrollweb (times, -50, 0);
}

void
ApvlvCore::scrollrightweb (int times)
{
  scrollweb (times, 50, 0);
}

bool
ApvlvCore::webIsScrolledToTop ()
{
  auto page = mMainWebView->page ();
  auto p = page->scrollPosition ();
  return p.y () < 0.5;
}

bool
ApvlvCore::webIsScrolledToBottom ()
{
  auto page = mMainWebView->page ();
  auto p = page->scrollPosition ();
  auto cs = page->contentsSize ();
  return p.y () + mMainWebView->height () + 0.5 > cs.height ();
}

void
ApvlvCore::scrollup (int times)
{
  if (!mReady)
    return;

  auto rate = APVLV_LINE_HEIGHT_DEFAULT * times;
  if (mMainVaj->value () - rate >= mMainVaj->minimum ())
    {
      mMainVaj->setValue (mMainVaj->value () - rate);
    }
  else if (mMainVaj->value () > mMainVaj->minimum ())
    {
      mMainVaj->setValue (mMainVaj->minimum ());
    }
  else
    {
      if (mAutoScrollPage)
        {
          if (mContinuous)
            {
              auto doc_size = mMainVaj->maximum () - mMainVaj->minimum ()
                              + mMainVaj->pageStep ();
              if (gParams->valuei ("continuouspad") > 0)
                doc_size -= (2 * 5); // 2 separates
              auto page_size = doc_size / 3;
              double scrollto
                  = static_cast<double> (mMainVaj->minimum () + page_size
                                         - rate)
                    / (mMainVaj->maximum () - mMainVaj->minimum ());
              showpage (mPagenum - 1, scrollto);
            }
          else
            {
              showpage (mPagenum - 1, 0.0);
            }
        }
    }

  display ();
}

void
ApvlvCore::scrolldown (int times)
{
  if (!mReady)
    return;

  auto rate = APVLV_LINE_HEIGHT_DEFAULT * times;
  if (mMainVaj->value () + rate <= mMainVaj->maximum ())
    {
      mMainVaj->setValue (mMainVaj->value () + rate);
    }
  else if (mMainVaj->value () < mMainVaj->maximum ())
    {
      mMainVaj->setValue (mMainVaj->maximum ());
    }
  else
    {
      if (mAutoScrollPage)
        {
          if (mContinuous)
            {
              auto doc_size = mMainVaj->maximum () - mMainVaj->minimum ()
                              + mMainVaj->pageStep ();
              if (gParams->valuei ("continuouspad") > 0)
                doc_size -= (2 * 5); // 2 separates
              auto page_size = doc_size / 3;
              double scrollto
                  = static_cast<double> (mMainVaj->maximum ()
                                         - mMainVaj->minimum () - page_size
                                         + rate)
                    / (mMainVaj->maximum () - mMainVaj->minimum ());
              showpage (mPagenum + 1, scrollto);
            }
          else
            {
              showpage (mPagenum + 1, 0.0);
            }
        }
    }

  display ();
}

void
ApvlvCore::scrollleft (int times)
{
  if (!mReady)
    return;

  auto val = mMainHaj->value () - APVLV_WORD_WIDTH_DEFAULT * times;
  if (val > mMainHaj->minimumWidth ())
    {
      mMainHaj->setValue (val);
    }
  else
    {
      mMainHaj->setValue (mMainHaj->minimumWidth ());
    }
}

void
ApvlvCore::scrollright (int times)
{
  if (!mReady)
    return;

  auto val = mMainHaj->value () + APVLV_WORD_WIDTH_DEFAULT * times;
  if (val + mMainHaj->width () < mMainHaj->maximumWidth ())
    {
      mMainHaj->setValue (val);
    }
  else
    {
      mMainHaj->setValue (mMainHaj->maximumWidth () - mMainHaj->width ());
    }
}

bool
ApvlvCore::usecache ()
{
  return false;
}

void
ApvlvCore::usecache (bool use)
{
}

bool
ApvlvCore::print (int ct)
{
  return false;
}

bool
ApvlvCore::totext (const char *name)
{
  return false;
}

bool
ApvlvCore::rotate (int ct)
{
  return false;
}

void
ApvlvCore::markposition (const char s)
{
}

void
ApvlvCore::setzoom (const char *z)
{
}

void
ApvlvCore::jump (const char s)
{
}

void
ApvlvCore::nextpage (int times)
{
}

void
ApvlvCore::prepage (int times)
{
}

void
ApvlvCore::halfnextpage (int times)
{
}

void
ApvlvCore::halfprepage (int times)
{
}

bool
ApvlvCore::search (const char *str, bool reverse)
{
  return false;
}

void
ApvlvCore::gotolink (int ct)
{
}

int
ApvlvCore::getskip ()
{
  return mSkip;
}

void
ApvlvCore::setskip (int ct)
{
  mSkip = ct;
}

void
ApvlvCore::toggleContent ()
{
  auto show = !isShowContent ();
  toggleContent (show);
}

void
ApvlvCore::toggleContent (bool show)
{
  if (show)
    {
      if (mContent->isReady ())
        {
          mDirIndex = {};
          auto psize = mPaned->size ();
          QList<int> sizes{ APVLV_DEFAULT_CONTENT_WIDTH,
                            psize.width () - mPaned->handleWidth ()
                                - APVLV_DEFAULT_CONTENT_WIDTH };
          mPaned->setSizes (sizes);
          return;
        }
      else
        {
          errp ("no content, disable content !");
        }
    }

  auto psize = mPaned->size ();
  QList<int> sizes{ 0, psize.width () - mPaned->handleWidth () };
  mPaned->setSizes (sizes);
}

void
ApvlvCore::returnlink (int ct)
{
}

void
ApvlvCore::setActive (bool act)
{
  mActive = act;

  if (act)
    {
      if (mDisplayType == DISPLAY_TYPE_IMAGE)
        mMainImageScrolView->setFocus ();
      else
        mMainWebView->setFocus ();
    }
  else
    {
      mMainImageScrolView->clearFocus ();
      mMainWebView->clearFocus ();
      clearFocus ();
    }

  if (mActive && filename ())
    {
      auto path = filesystem::path (filename ());
      auto base = path.filename ();
      mView->settitle (base.string ());
    }

  mStatus->setActive (act);
}
void
ApvlvCore::setDirIndex (const string &path)
{
  mDirIndex = { "", 0, "", FILE_INDEX_DIR };
  mDirIndex.load_dir (path);
  emit indexGenerited (mDirIndex);
}

bool
ApvlvCore::toggledControlContent (bool is_right)
{
  if (!isShowContent ())
    {
      return false;
    }

  auto controlled = isControlledContent ();

  if (!controlled && !is_right)
    {
      mContent->setIsFocused (true);
      return true;
    }
  else if (controlled && is_right)
    {
      mContent->setIsFocused (false);
      return true;
    }

  return false;
}

void
ApvlvCore::display ()
{
}

bool
ApvlvCore::isShowContent ()
{
  auto sizes = mPaned->sizes ();
  return sizes[0] > 1;
}

bool
ApvlvCore::isControlledContent ()
{
  if (!isShowContent ())
    return false;

  return mContent->isFocused ();
}

ApvlvCore *
ApvlvCore::findByWidget (QWidget *widget)
{
  for (auto doc = widget; doc != nullptr; doc = doc->parentWidget ())
    {
      if (doc->inherits ("apvlv::ApvlvDoc"))
        return dynamic_cast<ApvlvCore *> (doc);

      debug ("doc is a %s", doc->metaObject ()->className ());
    }

  return nullptr;
}

bool
ApvlvCore::find (const char *str)
{
  return false;
}

void
ApvlvCore::initImageUi ()
{
  mMainImageScrolView = new QScrollArea ();
  mMainImageScrolView->setAlignment (Qt::AlignCenter);
  mMainImageScrolView->setHorizontalScrollBarPolicy (
      Qt::ScrollBarPolicy::ScrollBarAsNeeded);
  mMainImageScrolView->setVerticalScrollBarPolicy (
      Qt::ScrollBarPolicy::ScrollBarAsNeeded);

  mMainImageFrame = new QFrame (this);
  mMainImageScrolView->setWidget (mMainImageFrame);
}

void
ApvlvCore::initWebUi ()
{
  mWebProfile = make_unique<QWebEngineProfile> ();
  mMainWebView = new QWebEngineView (mWebProfile.get ());
  QObject::connect (mMainWebView, SIGNAL (loadFinished (bool)), this,
                    SLOT (webview_load_finished (bool)));
  // QObject::connect (mWeb[0].get (),
  //                  SIGNAL (customContextMenuRequested (const QPoint &)),
  //                  this, SLOT (webview_context_menu_cb (const QPoint &)));
  // g_signal_connect (G_OBJECT (mMainVaj), "value-changed",
  //                  G_CALLBACK (apvlv_doc_on_mouse), this);
}

void
ApvlvCore::showImage ()
{
  if (mMainWebView->parent () != nullptr)
    {
      mMainWebView->setParent (nullptr);
    }
  if (mMainImageScrolView->parent () == nullptr)
    {
      mPaned->addWidget (mMainImageScrolView);
      mMainVaj = mMainImageScrolView->verticalScrollBar ();
      mMainHaj = mMainImageScrolView->horizontalScrollBar ();
    }
}

void
ApvlvCore::showWeb ()
{
  if (mMainImageScrolView->parent () != nullptr)
    {
      mMainImageScrolView->setParent (nullptr);
    }
  if (mMainWebView->parent () == nullptr)
    {
      mPaned->addWidget (mMainWebView);
      mMainVaj = nullptr;
      mMainHaj = nullptr;
    }
}

void
ApvlvCore::webview_update (const string &key)
{
  auto pn = file ()->get_ocf_page (key);
  if (pn >= 0)
    {
      mPagenum = pn;
      display ();
    }
}

void
ApvlvCore::webview_load_finished (bool suc)
{
  if (suc)
    {
      if (!mAnchor.empty ())
        {
          auto page = mMainWebView->page ();
          stringstream javasrc;
          javasrc << "document.getElementById('";
          javasrc << mAnchor.substr (1);
          javasrc << "').scrollIntoView();";
          page->runJavaScript (QString::fromStdString (javasrc.str ()));
        }
      else if (mWebScrollUp)
        {
          scrollwebto (0.0, 1.0);
          mWebScrollUp = false;
        }
    }
}

void
ApvlvCore::webview_context_menu_cb (const QPoint &)
{
}

ApvlvStatus::ApvlvStatus ()
{
  setFrameShape (QFrame::NoFrame);
  auto layout = new QHBoxLayout ();
  debug ("status layout: %p", layout);
  setLayout (layout);
}

void
ApvlvStatus::setActive (bool act)
{
  auto children = findChildren<QLabel *> ();
  for (auto child : children)
    {
      if (child)
        {
          child->setEnabled (act);
        }
    }
}

void
ApvlvStatus::showMessages (const vector<string> &msgs)
{
  auto children = findChildren<QLabel *> ();
  vector<QWidget *> newlabels;
  for (size_t ind = 0; ind < msgs.size (); ++ind)
    {
      if (children.size () > (qsizetype)ind)
        {
          auto label = children[ind];
          label->setText (QString::fromStdString (msgs[ind]));
        }
      else
        {
          auto label = new QLabel ();
          label->setText (QString::fromStdString (msgs[ind]));
          newlabels.push_back (label);
        }
    }

  auto hbox = layout ();
  for (auto label : newlabels)
    {
      hbox->addWidget (label);
    }
}
}

// Local Variables:
// mode: c++
// End:

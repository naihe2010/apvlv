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
/* @CPPFILE ApvlvDoc.cc
 *
 *  Author: Alf <naihe2010@126.com>
 */

#include <QClipboard>
#include <QCursor>
#include <QGuiApplication>
#include <QMenu>
#include <QMouseEvent>
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <functional>
#include <set>
#include <sstream>

#include "ApvlvDoc.h"
#include "ApvlvInfo.h"
#include "ApvlvParams.h"
#include "ApvlvUtil.h"
#include "ApvlvView.h"

namespace apvlv
{
using namespace Qt;

const int APVLV_CURSOR_WIDTH_DEFAULT = 2;
const int APVLV_ANNOT_UNDERLINE_HEIGHT = 10;

ApvlvDoc::ApvlvDoc (ApvlvView *view, const char *zm, bool cache)
    : ApvlvCore (view)
{
  mReady = false;

  mAdjInchg = false;

  mAutoScrollPage = gParams->valueb ("autoscrollpage");
  mAutoScrollDoc = gParams->valueb ("autoscrolldoc");
  mContinuous = gParams->valueb ("continuous");

  mZoominit = false;

  mProCmd = 0;

  mInVisual = ApvlvVisualMode::VISUAL_NONE;

  mLastPoint = { 0, 0 };
  mCurPoint = { 0, 0 };

  mLastpress = 0;

  mRotatevalue = 0;

  mFile = nullptr;

  mSearchResults = nullptr;
  mSearchStr = "";

  mVbox = new QVBoxLayout ();
  mVbox->setAlignment (Qt::AlignmentFlag::AlignCenter);
  if (mContinuous && gParams->valuei ("continuouspad") > 0)
    {
      mVbox->setSpacing (5);
    }
  else
    {
    }
  mMainImageFrame->setLayout (mVbox);

  mDisplayType = DISPLAY_TYPE_HTML;

  mImg[0] = make_unique<ApvlvImage> (this, 0);
  mVbox->addWidget (mImg[0].get (), 0);
  if (mAutoScrollPage && mContinuous)
    {
      mImg[1] = make_unique<ApvlvImage> (this, 1);
      mVbox->addWidget (mImg[1].get (), 0);
      mImg[2] = make_unique<ApvlvImage> (this, 2);
      mVbox->addWidget (mImg[2].get (), 0);
    }
  else
    {
      mImg[1] = nullptr;
      mImg[2] = nullptr;
    }

  mContent->setDoc (this);
}

ApvlvDoc::~ApvlvDoc ()
{
  savelastposition (mFilestr.c_str ());
  mPositions.clear ();
}

void
ApvlvDoc::blankarea (ApvlvImage *image, CharRectangle pos, uchar *buffer,
                     int width, int height)
{
  int x1 = int (pos.p1x), x2 = int (pos.p2x);
  int y1 = int (pos.p1y), y2 = int (pos.p2y);
  if (x2 > width)
    {
      x2 = width;
    }
  if (y2 > height)
    {
      y2 = height;
    }

  for (int y = y1; y < y2; y++)
    {
      for (int x = x1; x < x2; x++)
        {
          int p = (int)(y * width * 3 + (x * 3));
          buffer[p + 0] = 0xff - buffer[p + 0];
          buffer[p + 1] = 0xff - buffer[p + 1];
          buffer[p + 2] = 0xff - buffer[p + 2];
        }
    }
}

void
ApvlvDoc::doubleClickBlank (ApvlvImage *img, double x, double y)
{
  auto cache = mCurrentCache[img->mId].get ();

  CharRectangle pos = { x, x, y, y };

  if (strcasecmp (gParams->values ("doubleclick"), "word") == 0)
    {
      ApvlvWord *word;
      word = cache->getword (x, y);
      if (word != nullptr)
        {
          qDebug ("find word: %s, [%.0f, %.0f, %.0f, %.0f]\n",
                  word->word.c_str (), word->pos.p1x, word->pos.p1y,
                  word->pos.p2x, word->pos.p2y);
          pos = word->pos;
        }
    }
  else if (strcasecmp (gParams->values ("doubleclick"), "line") == 0)
    {
      ApvlvLine *line;
      line = cache->getline (y);
      if (line != nullptr)
        {
          pos = line->pos;
        }
    }
  else if (strcasecmp (gParams->values ("doubleclick"), "page") == 0)
    {
      pos.p1x = 0;
      pos.p2x = cache->getwidth ();
      pos.p1y = 0;
      pos.p2y = cache->getheight ();
    }
  else
    {
      return;
    }

  mInVisual = ApvlvVisualMode::VISUAL_CTRL_V;
  updateLastPoint (pos.p1x, pos.p1y);
  updateCurPoint (pos.p2x, pos.p2y, false);
  blank (img);
}

void
ApvlvDoc::blank (ApvlvImage *img)
{
  auto cache = mCurrentCache[img->mId].get ();

  auto poses = cache->getSelected (mLastPoint, mCurPoint, mInVisual);
  auto p = cache->getbuf (true);

  // for (auto pos : poses)
  //  {
  //  blankarea (img, pos, buffer, cache->getwidth (), cache->getheight ());
  //}

  img->setImage (p);
}

void
ApvlvDoc::togglevisual (int key)
{
  if (!gParams->valueb ("visualmode"))
    {
      return;
    }

  if (mInVisual == ApvlvVisualMode::VISUAL_NONE)
    {
      updateLastPoint (mCurPoint.x, mCurPoint.y);
    }

  ApvlvVisualMode type = key == 'v' ? ApvlvVisualMode::VISUAL_V
                                    : ApvlvVisualMode::VISUAL_CTRL_V;
  if (mInVisual == type)
    {
      mInVisual = ApvlvVisualMode::VISUAL_NONE;
    }
  else
    {
      mInVisual = type;
    }

  blank (mCurrentImage);
}

void
ApvlvDoc::yank (ApvlvImage *image, int times)
{
  auto cache = mCurrentCache[image->mId].get ();

  auto poses = cache->getSelected (mLastPoint, mCurPoint, mInVisual);
  string content;
  string text;
  for (auto pos : poses)
    {
      if (mFile->pageSelection (cache->getpagenum (), pos.p1x, pos.p1y,
                                pos.p2x, pos.p2y, text))
        {
          content.append (text);
        }
    }

  qDebug ("selected \n[%s]\n", content.c_str ());

  auto cb = QGuiApplication::clipboard ();
  cb->setText (QString::fromLocal8Bit (content));
}

void
ApvlvDoc::annotUnderline (ApvlvImage *image)
{
  auto cache = mCurrentCache[image->mId].get ();

  auto poses = cache->getSelected (mLastPoint, mCurPoint, mInVisual);
  for (auto pos : poses)
    {
      mFile->pageAnnotUnderline (cache->getpagenum (), pos.p1x, pos.p2y,
                                 pos.p2x, pos.p2y);
    }

  refresh ();
}

void
ApvlvDoc::annotText (ApvlvImage *image)
{
  auto cache = mCurrentCache[image->mId].get ();

  mInVisual = ApvlvVisualMode::VISUAL_CTRL_V;
  auto poses = cache->getSelected (mLastPoint, mCurPoint, mInVisual);
  if (poses.empty ())
    return;

  auto pos = poses[poses.size () - 1];
  auto text
      = ApvlvView::input ("Comment: ", int (mZoomrate * (pos.p2x - pos.p1x)),
                          int (mZoomrate * (pos.p2y - pos.p1y)));
  if (text)
    {
      mFile->pageAnnotText (cache->getpagenum (), pos.p1x,
                            cache->getheight () - pos.p1y, pos.p2x,
                            cache->getheight () - pos.p2y, text);
    }

  refresh ();
}

void
ApvlvDoc::commentText (ApvlvImage *image)
{
  auto cache = mCurrentCache[image->mId].get ();

  auto poses = cache->getSelected (mLastPoint, mCurPoint, mInVisual);
  if (poses.empty ())
    return;

  CharRectangle apos
      = { mLastPoint.x, mCurPoint.x, mLastPoint.y, mCurPoint.y };
  CharRectangle cpos{};
  if (cache->getAvailableSpace (apos, &cpos) == false)
    {
      qDebug ("not find space");
      return;
    }

  for (auto &&pos : poses)
    {
      mFile->pageAnnotUnderline (cache->getpagenum (), pos.p1x, pos.p2y,
                                 pos.p2x, pos.p2y);
    }

  auto text
      = ApvlvView::input ("Comment: ", int (mZoomrate * (cpos.p2x - cpos.p1x)),
                          int (mZoomrate * (cpos.p2y - cpos.p1y)));
  if (text)
    {
      mFile->pageAnnotText (cache->getpagenum (), cpos.p1x,
                            cache->getheight () - cpos.p1y, cpos.p2x,
                            cache->getheight () - cpos.p2y, text);
    }

  refresh ();
}

ReturnType
ApvlvDoc::subprocess (int ct, uint key)
{
  uint procmd = mProCmd;
  mProCmd = 0;
  switch (procmd)
    {
    case 'm':
      markposition (char (key));
      break;

    case '\'':
      jump (char (key));
      break;

    case 'z':
      if (key == 'i')
        {
          char temp[0x10];
          snprintf (temp, sizeof temp, "%f", mZoomrate * 1.1);
          setzoom (temp);
        }
      else if (key == 'o')
        {
          char temp[0x10];
          snprintf (temp, sizeof temp, "%f", mZoomrate / 1.1);
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

ReturnType
ApvlvDoc::process (int has, int ct, uint key)
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
    case Key_PageDown:
    case CTRL ('f'):
      nextpage (ct);
      break;
    case Key_PageUp:
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
    case 'F':
      mView->promptcommand (char (key));
      return NEED_MORE;
    case 'H':
      if (mDisplayType == DISPLAY_TYPE_IMAGE)
        {
          scrollto (0.0);
          updateCurPoint (0, 0, true);
          blank (mCurrentImage);
        }
      else
        scrollwebto (0.0, 0.0);
      break;
    case 'M':
      if (mDisplayType == DISPLAY_TYPE_IMAGE)
        {
          scrollto (0.5);
          auto x = 0;
          auto y
              = double (mCurrentCache[mCurrentImage->mId]->getheight ()) / 2;
          updateCurPoint (x, y, true);
          blank (mCurrentImage);
        }
      else
        scrollwebto (0.0, 0.5);
      break;
    case 'L':
      if (mDisplayType == DISPLAY_TYPE_IMAGE)
        {
          scrollto (1.0);
          auto x = 0;
          auto y = double (mCurrentCache[mCurrentImage->mId]->getheight ());
          updateCurPoint (x, y, true);
          blank (mCurrentImage);
        }
      else
        scrollwebto (0.0, 1.0);
      break;
    case '0':
      if (mDisplayType == DISPLAY_TYPE_IMAGE)
        scrollleft (INT_MAX);
      else
        scrollwebto (0.0, 0.0);
      break;
    case '$':
      if (mDisplayType == DISPLAY_TYPE_IMAGE)
        scrollright (INT_MAX);
      else
        scrollwebto (1.0, 0.0);
      break;
    case CTRL ('p'):
    case Key_Up:
    case 'k':
      if (isControlledContent ())
        {
          mContent->scrollup (ct);
          contentShowPage (mContent->currentIndex (), false);
        }
      else
        {
          if (mDisplayType == DISPLAY_TYPE_IMAGE)
            scrollup (ct);
          else
            scrollupweb (ct);
        }
      break;
    case CTRL ('n'):
    case CTRL ('j'):
    case Key_Down:
    case 'j':
      if (isControlledContent ())
        {
          mContent->scrolldown (ct);
          contentShowPage (mContent->currentIndex (), false);
        }
      else
        {
          if (mDisplayType == DISPLAY_TYPE_IMAGE)
            scrolldown (ct);
          else
            scrolldownweb (ct);
        }
      break;
    case Key_Backspace:
    case Key_Left:
    case CTRL ('h'):
    case 'h':
      if (isControlledContent ())
        {
          mContent->scrollleft (ct);
          contentShowPage (mContent->currentIndex (), false);
        }
      else
        {
          if (mDisplayType == DISPLAY_TYPE_IMAGE)
            scrollleft (ct);
          else
            scrollleftweb (ct);
        }
      break;
    case Key_Space:
    case Key_Right:
    case CTRL ('l'):
    case 'l':
      if (isControlledContent ())
        {
          mContent->scrollright (ct);
          contentShowPage (mContent->currentIndex (), false);
        }
      else
        {
          if (mDisplayType == DISPLAY_TYPE_IMAGE)
            scrollright (ct);
          else
            scrollrightweb (ct);
        }
      break;
    case Key_Return:
      contentShowPage (mContent->currentIndex (), true);
      break;
    case 'R':
      reload ();
      break;
    case CTRL (']'):
      gotolink (ct);
      break;
    case CTRL ('t'):
      returnlink (ct);
      break;
    case 't':
      mView->newtab (helppdf);
      mView->open ();
      break;
    case 'T':
      mView->newtab (helppdf);
      mView->opendir ();
      break;
    case 'o':
      mView->open ();
      break;
    case 'O':
      mView->opendir ();
      break;
    case 'r':
      rotate (ct);
      break;
    case 'G':
      markposition ('\'');
      if (!has)
        {
          showpage (mFile->sum () - 1, 0.0);
        }
      else
        {
          showpage (ct - 1, 0.0);
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
          search ("", false);
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
          search ("", false);
        }
      else
        {
        }
      break;
    case CTRL ('v'):
    case 'v':
      togglevisual (char (key));
      break;
    case ('y'):
      yank (mCurrentImage, ct);
      mInVisual = ApvlvVisualMode::VISUAL_NONE;
      blank (mCurrentImage);
      break;
    case ('s'):
      setskip (ct);
      break;
    case ('c'):
      toggleContent ();
      break;
    case ('A'):
      mCurrentImage->annotate_cb ();
      break;
    case ('U'):
      mCurrentImage->underline_cb ();
      break;
    case ('C'):
      mCurrentImage->comment_cb ();
      break;
    default:
      return NO_MATCH;
      break;
    }

  return MATCH;
}

ApvlvDoc *
ApvlvDoc::copy ()
{
  char rate[16];
  snprintf (rate, sizeof rate, "%f", mZoomrate);
  auto *ndoc = new ApvlvDoc (mView, rate, usecache ());
  ndoc->loadfile (mFilestr, false, false);
  ndoc->showpage (mPagenum, scrollrate ());
  return ndoc;
}

void
ApvlvDoc::setzoom (const char *z)
{
  if (z != nullptr)
    {
      if (strcasecmp (z, "normal") == 0)
        {
          mZoommode = NORMAL;
          mZoomrate = 1.2;
        }
      else if (strcasecmp (z, "fitwidth") == 0)
        {
          mZoommode = FITWIDTH;
        }
      else if (strcasecmp (z, "fitheight") == 0)
        {
          mZoommode = FITHEIGHT;
        }
      else
        {
          double d = strtod (z, nullptr);
          if (d > 0)
            {
              mZoommode = CUSTOM;
              mZoomrate = d;
            }
        }
    }

  if (mFile != nullptr)
    {
      int pn = max (0, pagenumber () - 1);
      mFile->pageSize (pn, mRotatevalue, &mPagex, &mPagey);

      if (mZoommode == FITWIDTH)
        {
          auto x_root = mMainImageFrame->width ();
          mZoomrate = x_root / mPagex;
        }
      else if (mZoommode == FITHEIGHT)
        {
          auto y_root = mMainImageFrame->height ();
          mZoomrate = y_root / mPagey;
        }

      refresh ();
    }
}

bool
ApvlvDoc::savelastposition (const char *filename)
{
  if (filename == nullptr || helppdf == filename || gParams->valueb ("noinfo"))
    {
      return false;
    }

  bool ret = gInfo->updateFile (mPagenum, mSkip, scrollrate (), filename);
  return ret;
}

bool
ApvlvDoc::loadlastposition (const string &filename)
{
  if (filename.empty () || helppdf == filename || gParams->valueb ("noinfo"))
    {
      showpage (0, 0.0);
      return false;
    }

  bool ret = false;

  auto optfp = gInfo->file (filename);
  if (optfp)
    {
      // correctly check
      showpage (optfp.value ()->page, 0.0);
      setskip (optfp.value ()->skip);
    }
  else
    {
      showpage (0, 0.0);
      gInfo->updateFile (0, 0.0, mZoomrate, filename);
    }

  return ret;
}

bool
ApvlvDoc::reload ()
{
  savelastposition (filename ());
  return loadfile (mFilestr, false, isShowContent ());
}

int
ApvlvDoc::pagenumber ()
{
  if (mDisplayType == DISPLAY_TYPE_IMAGE && mContinuous
      && mCurrentCache[1] != nullptr)
    {
      if (scrollrate () > 0.5)
        {
          return int (mCurrentCache[1]->getpagenum () + 1);
        }
      else
        {
          return int (mCurrentCache[1]->getpagenum ());
        }
    }
  else
    {
      return mPagenum + 1;
    }
}

bool
ApvlvDoc::usecache ()
{
  return false;
}

void
ApvlvDoc::usecache (bool use)
{
  if (use)
    {
      mView->errormessage ("No pthread, can't support cache!!!");
      mView->infomessage ("If you really have pthread, please recomplie the "
                          "apvlv and try again.");
    }
}

bool
ApvlvDoc::loadfile (const string &filename, bool check, bool show_content)
{
  if (check)
    {
      if (filename == mFilestr)
        {
          return false;
        }
    }

  if (mFile)
    {
      delete mFile;
      mFile = nullptr;
    }
  mReady = false;

  mFile = File::newFile (filename, false);

  // qDebug ("mFile = %p", mFile);
  if (mFile != nullptr)
    {
      emit indexGenerited (mFile->getIndex ());

      mFilestr = filename;

      if (mFile->sum () <= 1)
        {
          qDebug ("sum () = %d", mFile->sum ());
          mContinuous = false;
          mAutoScrollDoc = false;
          mAutoScrollPage = false;
        }

      // qDebug ("pagesum () = %d", mFile->sum ());

      mCurrentCache[0] = make_unique<ApvlvDocCache> (mFile);
      if (mContinuous)
        {
          mCurrentCache[1] = make_unique<ApvlvDocCache> (mFile);
          mCurrentCache[2] = make_unique<ApvlvDocCache> (mFile);
        }

      setDisplayType (mFile->getDisplayType ());

      loadlastposition (filename);

      display ();

      setActive (true);

      mReady = true;

      mCurrentImage = mImg[0].get ();

      mSearchStr = "";
      mSearchResults = nullptr;

      mInVisual = ApvlvVisualMode::VISUAL_NONE;

      if (gParams->valuei ("autoreload") > 0)
        {
          mWatcher = make_unique<QFileSystemWatcher> ();
          // QObject::connect(mWatcher, SIGNAL(fileChanged()), this,
          // SLOT(changed_cb()));

          auto systempath = filesystem::path (filename);
          if (filesystem::is_symlink (systempath))
            {
              auto realname = filesystem::read_symlink (systempath).string ();
              if (filesystem::is_regular_file (realname))
                {
                  mWatcher->addPath (QString::fromLocal8Bit (realname));
                }
            }
          else
            {
              mWatcher->addPath (QString::fromLocal8Bit (filename));
            }
        }
    }

  if (show_content && mFile != nullptr)
    {
      toggleContent (true);
    }
  else
    {
      toggleContent (false);
    }

  return mFile != nullptr;
}

int
ApvlvDoc::convertindex (int p)
{
  if (mFile != nullptr)
    {
      int c = mFile->sum ();

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

void
ApvlvDoc::markposition (const char s)
{
  ApvlvDocPosition adp = { mPagenum, scrollrate () };
  mPositions[s] = adp;
}

void
ApvlvDoc::jump (const char s)
{
  auto it = mPositions.find (s);
  if (it != mPositions.end ())
    {
      ApvlvDocPosition adp = it->second;
      markposition ('\'');
      showpage (adp.pagenum, adp.scrollrate);
    }
}

void
ApvlvDoc::showpage (int p, double s)
{
  int rp = convertindex (p);
  if (rp < 0)
    return;

  // qDebug ("display page: %d | %lf", rp,s);
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

  mAnchor = "";
  mPagenum = rp;

  if (!mZoominit)
    {
      mZoominit = true;
      setzoom (nullptr);
      qDebug ("zoom rate: %f", mZoomrate);
    }

  refresh ();

  if (mDisplayType == DISPLAY_TYPE_IMAGE)
    scrollto (s);
  else
    scrollwebto (0, s);
}

void
ApvlvDoc::showpage (int p, const string &anchor)
{
  showpage (p, 0.0);
  mAnchor = anchor;
}

void
ApvlvDoc::nextpage (int times)
{
  showpage (mPagenum + times, 0.0);
}

void
ApvlvDoc::prepage (int times)
{
  showpage (mPagenum - times, 0.0);
}

void
ApvlvDoc::refresh ()
{
  if (mFile == nullptr)
    return;

  mFile->pageSize (mPagenum, mRotatevalue, &mPagex, &mPagey);
  int px = static_cast<int> (mPagex * mZoomrate);
  int py = static_cast<int> (mPagey * mZoomrate);

  mMainImageFrame->resize (px, py);

  if (mDisplayType == DISPLAY_TYPE_IMAGE)
    {
      mCurrentCache[0]->set (mPagenum, mZoomrate, mRotatevalue, false);
      auto p = mCurrentCache[0]->getbuf (true);
      mImg[0]->resize (px, py);
      mImg[0]->setImage (p);

      if (mAutoScrollPage && mContinuous)
        {
          mMainImageFrame->resize (px, 3 * py + 10);

          mCurrentCache[1]->set (convertindex (mPagenum + 1), mZoomrate,
                                 mRotatevalue, false);
          p = mCurrentCache[1]->getbuf (true);
          mImg[1]->resize (px, py);
          mImg[1]->setImage (p);

          mCurrentCache[2]->set (convertindex (mPagenum + 2), mZoomrate,
                                 mRotatevalue, false);
          p = mCurrentCache[2]->getbuf (true);
          mImg[2]->resize (px, py);
          mImg[2]->setImage (p);
        }
    }
  else if (mDisplayType == DISPLAY_TYPE_CUSTOM)
    {
      mFile->widgetGoto (mCustomWidget, mPagenum);
      mFile->widgetZoom (mCustomWidget, mZoomrate);
    }
  else if (mDisplayType == DISPLAY_TYPE_HTML)
    {
      mFile->pageRender (mPagenum, px, py, mZoomrate, mRotatevalue,
                         mMainWebView);
    }

  display ();
}

void
ApvlvDoc::srtranslate (int &rtimes, double &sr, bool single2continuous)
{
  if (mMainVaj == nullptr)
    {
      sr = 0.0;
      return;
    }

  double winv = mMainVaj->maximumHeight () - mMainVaj->minimumHeight (),
         pagewidth = mMainVaj->height (), maxv = winv - pagewidth,
         maxv2 = (winv - 2 * pagewidth) / 2, value;

  if (single2continuous)
    {
      value = 0.5 * pagewidth + sr * maxv2;
      sr = (value - 0.5 * pagewidth) / maxv;
    }
  else
    {
      value = 0.5 * pagewidth + sr * maxv;
      if (value > 0.5 * winv)
        {
          rtimes += 1;
          value -= 0.5 * winv;
        }

      if (value > 0.5 * (winv - pagewidth))
        sr = 1;
      else if (value > 0.5 * pagewidth)
        sr = (value - 0.5 * pagewidth) / maxv2;
      else
        sr = 0;
    }
}

void
ApvlvDoc::halfnextpage (int times)
{
  double sr = scrollrate ();
  int rtimes = times / 2;

  if (mAutoScrollPage && mContinuous)
    srtranslate (rtimes, sr, false);

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

  if (mAutoScrollPage && mContinuous)
    srtranslate (rtimes, sr, true);

  showpage (mPagenum + rtimes, sr);
}

void
ApvlvDoc::halfprepage (int times)
{
  double sr = scrollrate ();
  int rtimes = times / 2;

  if (mAutoScrollPage && mContinuous)
    srtranslate (rtimes, sr, false);

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

  if (mAutoScrollPage && mContinuous)
    srtranslate (rtimes, sr, true);

  showpage (mPagenum - rtimes, sr);
}

void
ApvlvDoc::markselection ()
{
  qDebug ("mSelect: %d.", mSearchSelect);
  qDebug ("zoomrate: %f", mZoomrate);

  if (mSearchResults->size () <= mSearchSelect)
    return;

  auto rect = (*mSearchResults)[mSearchSelect][0];

  // Caculate the correct position
  // qDebug ("pagex: %f, pagey: %f, p1x: %f, p1y: %f, p2x: %f, p2y: %f",
  // mPagex, mPagey, rect->p1x, rect->p1y, rect->p2x, rect->p2y);
  int x1 = (int)(rect.p1x * mZoomrate);
  int x2 = (int)(rect.p2x * mZoomrate);
  int y1 = (int)(rect.p2y * mZoomrate);
  int y2 = (int)(rect.p1y * mZoomrate);
  qDebug ("p1x: %d, p1y: %d, p2x: %d, p2y: %d", x1, y1, x2, y2);

  // make the selection at the page center
  double val = ((y1 + y2) - mMainVaj->minimum ()) / 2;
  if (val + mMainVaj->pageStep ()
      > mMainVaj->maximum () - mMainVaj->minimum () - 5)
    {
      qDebug ("set value: %d", mMainVaj->maximum () - mMainVaj->minimum ()
                                   - mMainVaj->pageStep () - 5);
      mMainVaj->setValue (mMainVaj->maximum () - mMainVaj->minimum ()
                          - mMainVaj->pageStep ()
                          - 5); /* just for avoid the auto scroll page */
    }
  else if (val > 5)
    {
      qDebug ("set value: %f", val);
      mMainVaj->setValue (val);
    }
  else
    {
      // qDebug ("set value: %f", gtk_adjustment_get_lower (mMainVaj) + 5);
      mMainVaj->setValue (mMainVaj->minimum ()
                          + 5); /* avoid auto scroll page */
    }

  val = ((x1 + x2) - mMainHaj->minimum ()) / 2;
  if (val + mMainHaj->pageStep () > mMainHaj->maximum ())
    {
      mMainHaj->setValue (mMainHaj->maximum ());
    }
  else if (val > 0)
    {
      mMainHaj->setValue (val);
    }
  else
    {
      mMainHaj->setValue (mMainHaj->minimumWidth ());
    }

  mCurrentCache[0]->set (mPagenum, mZoomrate, mRotatevalue);
  auto p = mCurrentCache[0]->getbuf (true);

  mFile->pageSelectSearch (
      mPagenum, mCurrentCache[0]->getwidth (), mCurrentCache[0]->getheight (),
      mZoomrate, mRotatevalue, &p, mSearchSelect, mSearchResults.get ());
  mImg[0]->setImage (p);
  qDebug ("helight num: %d", mPagenum);
}

void
ApvlvDoc::markselectionweb ()
{
  Q_ASSERT (mSearchResults->size () > mSearchSelect);
  auto rect = (*mSearchResults)[mSearchSelect][0];

  double width, height, xrate = 0.0, yrate = 0.0;
  if (mFile->pageSize (mPagenum, mRotatevalue, &width, &height))
    {
      xrate = (rect.p1x + rect.p2x) / 2 / width;
      yrate = (rect.p1y + rect.p2y) / 2 / height;
    }

  qDebug ("helight num: %d:%f", mPagenum, yrate);
  mFile->pageSelectSearch (mPagenum, int (width), int (height), mZoomrate,
                           mRotatevalue, mMainWebView, int (mSearchSelect),
                           mSearchResults.get ());
  mFile->pageRender (mPagenum, int (width), int (height), mZoomrate,
                     mRotatevalue, mMainWebView);
  scrollwebto (xrate, yrate);
}

void
ApvlvDoc::scrollup (int times)
{
  if (!mReady)
    {
      return;
    }

  if (!gParams->valueb ("visualmode"))
    {
      ApvlvCore::scrollup (times);
      return;
    }

  auto cache = mCurrentCache[mCurrentImage->mId].get ();

  auto rate = cache->getHeightOfLine (mCurPoint.y);

  int ny1 = int (mCurPoint.y - rate * times);
  int height = ny1;

  if (mCurrentImage->mId > 0)
    height += mImg[0]->height ();
  if (mCurrentImage->mId > 1)
    height += mImg[1]->height ();

  /*
  qDebug ("mCurrentImage->mId: %d, mCurpoint.y: %f, cursor height: %d, image "
         "height: %d, page "
         "size: %f, adj: %f, max: %f",
         mCurrentImage->mId, mCurPoint.y, height,
         gtk_widget_get_allocated_height (mCurrentImage->widget ()),
         gtk_adjustment_get_page_size (mMainVaj),
         gtk_adjustment_get_value (mMainVaj),
         gtk_adjustment_get_upper (mMainVaj));
  */
  if (height < mMainVaj->value ())
    {
      ApvlvCore::scrollup (times);
    }

  if (ny1 <= 0)
    {
      if (mCurrentImage->mId > 0)
        {
          mCurrentImage = mImg[mCurrentImage->mId - 1].get ();
        }
      ny1 = mCurrentImage->height () - rate;
    }

  updateCurPoint (mCurPoint.x, ny1, mInVisual == ApvlvVisualMode::VISUAL_NONE);
  blank (mCurrentImage);
}

void
ApvlvDoc::scrolldown (int times)
{
  if (!mReady)
    {
      return;
    }

  if (!gParams->valueb ("visualmode"))
    {
      ApvlvCore::scrolldown (times);
      return;
    }

  auto cache = mCurrentCache[mCurrentImage->mId].get ();

  /*
  qDebug ("mCurrentImage->mId: %d, mCurpoint.y: %f, image height: %d, page "
         "size: %f, adj: %f, max: %f",
         mCurrentImage->mId, mCurPoint.y,
         gtk_widget_get_allocated_height (mCurrentImage->widget ()),
         gtk_adjustment_get_page_size (mMainVaj),
         gtk_adjustment_get_value (mMainVaj),
         gtk_adjustment_get_upper (mMainVaj));
  */
  auto rate = cache->getHeightOfLine (mCurPoint.y);

  int ny1 = int (mCurPoint.y + rate * times);
  int height = ny1;

  if (mCurrentImage->mId > 0)
    height += mImg[0]->height ();
  if (mCurrentImage->mId > 1)
    height += mImg[1]->height ();

  if (height >= mMainVaj->pageStep () + mMainVaj->value ())
    {
      ApvlvCore::scrolldown (times);
    }

  if (ny1 >= mCurrentImage->height ())
    {
      ny1 = 0;
      if (mCurrentImage->mId < 2)
        {
          mCurrentImage = mImg[mCurrentImage->mId + 1].get ();
        }
    }

  updateCurPoint (mCurPoint.x, ny1, mInVisual == ApvlvVisualMode::VISUAL_NONE);
  blank (mCurrentImage);
}

void
ApvlvDoc::scrollleft (int times)
{
  if (!mReady)
    return;

  if (!gParams->valueb ("visualmode"))
    {
      ApvlvCore::scrollleft (times);
      return;
    }

  auto cache = mCurrentCache[mCurrentImage->mId].get ();
  auto rate = cache->getWidthOfWord (mCurPoint.x, mCurPoint.y);

  int nx1 = int (mCurPoint.x - rate * times);
  if (nx1 < 0)
    nx1 = 0;

  if (nx1 < mMainHaj->maximumWidth () - mMainHaj->pageStep ())
    {
      ApvlvCore::scrollleft (times);
    }

  updateCurPoint (nx1, mCurPoint.y, mInVisual == ApvlvVisualMode::VISUAL_NONE);
  blank (mCurrentImage);
}

void
ApvlvDoc::scrollright (int times)
{
  if (!mReady)
    return;

  if (!gParams->valueb ("visualmode"))
    {
      ApvlvCore::scrollright (times);
      return;
    }

  auto cache = mCurrentCache[mCurrentImage->mId].get ();

  auto rate = cache->getWidthOfWord (mCurPoint.x, mCurPoint.y);

  int nx1 = int (mCurPoint.x + rate * times);
  if (nx1 > mCurrentCache[0]->getwidth ())
    nx1 = mCurrentCache[0]->getwidth () - 1;

  if (nx1 > mMainHaj->pageStep ())
    {
      ApvlvCore::scrollright (times);
    }

  updateCurPoint (nx1, mCurPoint.y, mInVisual == ApvlvVisualMode::VISUAL_NONE);
  blank (mCurrentImage);
}

bool
ApvlvDoc::needsearch (const char *str, bool reverse)
{
  if (mFile == nullptr)
    return false;

  // search a different string
  if (strlen (str) > 0 && strcmp (str, mSearchStr.c_str ()) != 0)
    {
      qDebug ("different string.");
      mSearchSelect = 0;
      mSearchStr = str;
      return true;
    }

  else if (mSearchResults == nullptr)
    {
      qDebug ("no result.");
      mSearchSelect = 0;
      return true;
    }

  // same string, but need to search next page
  else if ((mSearchPagenum != mPagenum)
           || ((mSearchReverse == reverse)
               && mSearchSelect == mSearchResults->size () - 1)
           || ((mSearchReverse != reverse) && mSearchSelect == 0))
    {
      qDebug ("same, but need next string: S: %d, s: %d, sel: %d, max: %lu.",
              mSearchReverse, reverse, mSearchSelect, mSearchResults->size ());
      mSearchSelect = 0;
      return true;
    }

  // same string, not need search, but has zoomed
  else
    {
      qDebug ("same, not need next string. sel: %d, max: %ld", mSearchSelect,
              mSearchResults->size ());
      if (mSearchReverse == reverse)
        {
          mSearchSelect++;
        }
      else
        {
          mSearchSelect--;
        }

      if (mDisplayType == DISPLAY_TYPE_IMAGE)
        markselection ();
      else
        markselectionweb ();
      return false;
    }
}

bool
ApvlvDoc::search (const char *str, bool reverse)
{
  if (*str == '\0' && mSearchStr.empty ())
    {
      return false;
    }

  if (mFile->getDisplayType () == DISPLAY_TYPE_CUSTOM)
    {
      return mFile->widgetSearch (mCustomWidget, str);
    }

  if (*str != '\0')
    {
      mSearchCmd = reverse ? BACKSEARCH : SEARCH;
    }

  if (!needsearch (str, reverse))
    {
      return true;
    }

  mSearchResults = nullptr;

  bool wrap = gParams->valueb ("wrapscan");

  int i = mPagenum;
  int sum = mFile->sum (), from = i;
  bool search = false;
  while (true)
    {
      if (*str != 0 || search)
        {
          mSearchResults = mFile->pageSearch ((i + sum) % sum,
                                              mSearchStr.c_str (), reverse);
          mSearchReverse = reverse;
          if (mSearchResults != nullptr)
            {
              showpage ((i + sum) % sum, 0.5);
              mSearchPagenum = mPagenum;
              if (mDisplayType == DISPLAY_TYPE_IMAGE)
                markselection ();
              else
                markselectionweb ();
              return true;
            }
        }

      search = true;

      if (!reverse && i < (wrap ? (from + sum) : (sum - 1)))
        {
          //          qDebug ("wrap: %d, i++:", wrap, i, i + 1);
          i++;
        }
      else if (reverse && i > (wrap ? (from - sum) : 0))
        {
          qDebug ("wrap: %d, i--: %d", wrap, i - 1);
          i--;
        }
      else
        {
          mView->errormessage ("can't find word: '%s'", mSearchStr.c_str ());
          return false;
        }
    }
}

bool
ApvlvDoc::totext (const char *file)
{
  if (mFile == nullptr)
    return false;

  string txt;
  bool ret
      = mFile->pageSelection (mPagenum, 0, 0, mCurrentCache[0]->getwidth (),
                              mCurrentCache[0]->getheight (), txt);
  if (ret)
    {
      // need impl g_file_set_contents (file, txt, -1, nullptr);
      return true;
    }
  return false;
}

bool
ApvlvDoc::rotate (int ct)
{
  // just hack
  if (ct == 1)
    ct = 90;

  if (ct % 90 != 0)
    {
      mView->errormessage ("Not a 90 times value: %d", ct);
      return false;
    }

  mRotatevalue += ct;
  while (mRotatevalue < 0)
    {
      mRotatevalue += 360;
    }
  while (mRotatevalue > 360)
    {
      mRotatevalue -= 360;
    }
  refresh ();
  return true;
}

void
ApvlvDoc::gotolink (int ct)
{
  auto links1 = mCurrentCache[0]->getlinks ();
  auto links2 = mCurrentCache[1] ? mCurrentCache[1]->getlinks () : nullptr;

  int siz = links1 ? int (links1->size ()) : 0;
  siz += links2 ? int (links2->size ()) : 0;

  ct--;

  if (ct >= 0 && ct < siz)
    {
      markposition ('\'');

      ApvlvDocPosition p = { mPagenum, scrollrate () };
      mLinkPositions.push_back (p);

      if (links1 == nullptr)
        {
          showpage ((*links2)[ct].mPage, 0.0);
        }
      else
        {
          if (ct < (int)links1->size ())
            {
              showpage ((*links1)[ct].mPage, 0.0);
            }
          else
            {
              showpage ((*links2)[ct - links1->size ()].mPage, 0.0);
            }
        }
    }
}

void
ApvlvDoc::returnlink (int ct)
{
  qDebug ("Ctrl-t %d", ct);
  if (ct <= (int)mLinkPositions.size () && ct > 0)
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

bool
ApvlvDoc::print (int ct)
{
  // need impl
  return false;
}

void
ApvlvDoc::on_mouse ()
{
  if (mAdjInchg)
    {
      mAdjInchg = false;
      return;
    }

  auto adj = mMainVaj;

  if (adj->maximumHeight () - adj->minimumHeight ()
      == adj->height () + adj->value ())
    {
      scrolldown (1);
    }
  else if (adj->value () == 0)
    {
      scrollup (1);
    }
}

void
ApvlvDoc::monitor_callback ()
{
  if (!mInuse)
    {
      return;
    }

  qDebug ("Contents is modified, apvlv reload it automatically");
  reload ();
}

void
ApvlvDoc::edit_annotation_cb ()
{
  auto cache = mCurrentCache[mCurrentImage->mId].get ();
  auto pos = mCurrentAnnotText->pos;
  auto text = ApvlvView::input (
      "Comment: ", int (mZoomrate * (pos.p2x - pos.p1x)),
      int (mZoomrate * (pos.p2y - pos.p1y)), mCurrentAnnotText->text);
  if (text)
    {
      mCurrentAnnotText->text = text;
      mFile->pageAnnotUpdate (cache->getpagenum (), mCurrentAnnotText);
    }

  refresh ();
}

void
ApvlvDoc::delete_annotation_cb ()
{
  auto cache = mCurrentCache[mCurrentImage->mId].get ();
  mCurrentAnnotText->text = "";
  mFile->pageAnnotUpdate (cache->getpagenum (), mCurrentAnnotText);
  refresh ();
}

void
ApvlvDoc::contentShowPage (const FileIndex *index, bool force)
{
  if (index == nullptr)
    return;

  if (index->type == FileIndexType::FILE_INDEX_FILE)
    {
      loadfile (index->path, true, true);
      return;
    }

  auto file = mContent->currentFileIndex ();
  if (file && file->path != mFilestr)
    loadfile (file->path, true, true);

  auto follow_mode = std::string ("always");
  if (!force)
    {
      follow_mode = gParams->values ("content_follow_mode");
    }

  if (follow_mode == "none")
    {
      return;
    }

  else if (follow_mode == "page")
    {
      if (index->type == FileIndexType::FILE_INDEX_PAGE)
        {
          if (index->page != mPagenum || index->anchor != mAnchor)
            showpage (index->page, index->anchor);
        }
      return;
    }

  else if (follow_mode == "always")
    {
      if (index->type == FileIndexType::FILE_INDEX_PAGE)
        {
          if (index->page != mPagenum || index->anchor != mAnchor)
            showpage (index->page, index->anchor);
        }
      else
        {
          if (index->path != filename ())
            {
              loadfile (index->path, true, false);
            }
        }
    }
}

void
ApvlvDoc::setDisplayType (DISPLAY_TYPE type)
{
  if (type == DISPLAY_TYPE_IMAGE)
    {
      showImage ();
    }
  else if (type == DISPLAY_TYPE_CUSTOM)
    {
      if (mCustomWidget != nullptr)
        {
          mCustomWidget->deleteLater ();
        }
      mCustomWidget = mFile->getWidget ();
      showWidget ();
    }
  else
    {
      mMainWebView->setFile (mFile);
      showWeb ();
    }

  mDisplayType = type;
}

void
ApvlvDoc::updateLastPoint (double x, double y)
{
  mLastPoint = { x, y };
}

void
ApvlvDoc::updateCurPoint (double x, double y, bool updateLast)
{
  if (updateLast)
    {
      updateLastPoint (mCurPoint.x, mCurPoint.y);
    }
  mCurPoint = { x, y };
}

ApvlvDocCache::ApvlvDocCache (File *file)
{
  mFile = file;
  mPagenum = -1;
  mInverted = false;
  mZoom = 1.0;
  mRotate = 0;
}

void
ApvlvDocCache::set (uint p, double zm, uint rot, bool delay)
{
  mPagenum = int (p);
  mZoom = zm;
  mRotate = rot;

  mLines.clear ();

  if (mLinks != nullptr)
    {
      mLinks.release ();
    }

  mInverted = gParams->valueb ("inverted");
  load ();
}

void
ApvlvDocCache::load ()
{
  int c = mFile->sum ();

  if (mPagenum < 0 || mPagenum >= c)
    {
      qDebug ("no this page: %d", mPagenum);
      return;
    }

  double tpagex, tpagey;
  if (!mFile->pageSize (mPagenum, int (mRotate), &tpagex, &tpagey))
    {
      qCritical ("error getting pageSize for pagenum: %d", mPagenum);
      return;
    };

  mWidth = static_cast<int> (tpagex);
  mHeight = static_cast<int> (tpagey);
  // qDebug ("ac->mFile: %p", ac->mFile);
  mFile->pageRender (mPagenum, mWidth, mHeight, mZoom, int (mRotate), &mBuf);
  if (mInverted)
    {
      // need impl invert_pixbuf (bu);
    }

  // set annot text
  mAnnotTexts = mFile->pageAnnotTexts (mPagenum);
  for (const auto &annot : mAnnotTexts)
    {
      if (annot.type == APVLV_ANNOT_TEXT && annot.text.length () > 0)
        setAnnot (annot);
    }

  mLinks = mFile->pageLinks (mPagenum);
  // qDebug ("has mLinkMappings: %p", ac->mLinks);

  preGetLines (0, 0, int (tpagex), int (tpagey));
  sortLines ();
}

void
ApvlvDocCache::setAnnot (const ApvlvAnnotText &annot) const
{
  // need impl
}

bool
ApvlvDocCache::getAvailableSpace (CharRectangle pos, CharRectangle *outpos)
{
  auto lines = getlines (pos.p1y, pos.p2y);
  if (lines.empty ())
    {
      *outpos = pos;
      return true;
    }

  auto linelid = lines.size () - 1;
  auto maxx = lines[0]->pos.p2x;
  for (auto &l : lines)
    {
      if (l->pos.p2x > maxx)
        maxx = l->pos.p2x;
    }
  CharRectangle apos = { maxx + 20, double (mWidth - 1), lines[0]->pos.p1y,
                         lines[linelid]->pos.p2y };
  if (annotAtPos (apos) == nullptr)
    {
      *outpos = apos;
      return true;
    }

  auto minx = lines[0]->pos.p1x;
  for (auto &l : lines)
    {
      if (l->pos.p1x < minx)
        minx = l->pos.p1x;
    }
  CharRectangle bpos
      = { 1, minx - 20, lines[0]->pos.p1y, lines[linelid]->pos.p2y };
  if (annotAtPos (bpos) == nullptr)
    {
      *outpos = bpos;
      return true;
    }

  CharRectangle cpos = { 1, double (mWidth - 1), 1, mLines[0].pos.p1y - 20 };
  if (annotAtPos (cpos) == nullptr)
    {
      *outpos = cpos;
      return true;
    }

  auto glid = mLines.size () - 1;
  CharRectangle dpos = { 1, double (mWidth - 1), mLines[glid].pos.p2y + 20,
                         double (mHeight - 1) };
  if (annotAtPos (dpos) == nullptr)
    {
      *outpos = dpos;
      return true;
    }

  return false;
}

ApvlvDocCache::~ApvlvDocCache () {}

int
ApvlvDocCache::getpagenum () const
{
  return mPagenum;
}

/*
 * get the cache QImage
 * @param: wait, if not wait, not wait the pixbuf be prepared
 * @return: the buffer
 * */
const QImage &
ApvlvDocCache::getbuf (bool wait)
{
  return mBuf;
}

int
ApvlvDocCache::getwidth () const
{
  return mWidth;
}

int
ApvlvDocCache::getheight () const
{
  return mHeight;
}

ApvlvLinks *
ApvlvDocCache::getlinks ()
{
  return mLinks.get ();
}

ApvlvWord *
ApvlvDocCache::getword (double x, double y)
{
  auto line = getline (y);
  if (line == nullptr)
    return nullptr;

  for (auto &mWord : line->mWords)
    {
      if (x >= mWord.pos.p1x && x <= mWord.pos.p2x)
        {
          return &mWord;
        }
    }

  return nullptr;
}

ApvlvLine *
ApvlvDocCache::getline (double y)
{
  if (mLines.empty ())
    return nullptr;

  for (auto &mLine : mLines)
    {
      if (y >= mLine.pos.p1y && y <= mLine.pos.p2y)
        {
          return &mLine;
        }
    }

  return nullptr;
}

ApvlvAnnotText *
ApvlvDocCache::annotAtPos (CharRectangle vpos)
{
  auto x1 = vpos.p1x, y1 = mHeight - vpos.p2y;
  auto x2 = vpos.p2x, y2 = mHeight - vpos.p1y;
  for (auto &annot : mAnnotTexts)
    {
      if (annot.type == APVLV_ANNOT_TEXT)
        {
          if (x1 >= annot.pos.p1x && x2 <= annot.pos.p2x && y1 >= annot.pos.p1y
              && y2 <= annot.pos.p2y)
            return &annot;
        }
      else
        {
          if (x1 >= annot.pos.p1x && x2 <= annot.pos.p2x
              && abs (y1 - annot.pos.p1y) < APVLV_ANNOT_UNDERLINE_HEIGHT
              && abs (y2 - annot.pos.p2y) < APVLV_ANNOT_UNDERLINE_HEIGHT)
            return &annot;
        }
    }

  return nullptr;
}

void
ApvlvDocCache::preGetLines (int x1, int y1, int x2, int y2)
{
  if (strcmp (gParams->values ("doubleclick"), "page") == 0
      || strcmp (gParams->values ("doubleclick"), "none") == 0)
    {
      return;
    }

  string content;
  mFile->pageSelection (mPagenum, x1, y1, x2, y2, content);
  if (!content.empty ())
    {
      string word;

      mLines.clear ();

      std::set<string> processed;

      if (strcmp (gParams->values ("doubleclick"), "word") == 0)
        {
          stringstream ss (content);
          while (!ss.eof ())
            {
              ss >> word;

              if (processed.count (word) > 0)
                continue;

              processed.insert (word);

              auto results
                  = mFile->pageSearch (mPagenum, word.c_str (), false);
              if (results != nullptr)
                {
                  prepare_add (word.c_str (), results.get ());
                }
            }
        }
      else if (strcmp (gParams->values ("doubleclick"), "line") == 0)
        {
          char **v, *p;
          int i;

          // v = g_strsplit (content, "\n", -1);
          if (v != nullptr)
            {
              for (i = 0; v[i] != nullptr; ++i)
                {
                  p = v[i];
                  while (*p != '\0' && isspace (*p))
                    {
                      p++;
                    }
                  if (*p == '\0')
                    {
                      continue;
                    }

                  if (processed.count (word) > 0)
                    continue;

                  processed.insert (word);

                  qDebug ("search [%s]", p);
                  auto results = mFile->pageSearch (mPagenum, p, false);
                  if (results != nullptr)
                    {
                      prepare_add (p, results.get ());
                    }
                }
              // g_strfreev (v);
            }
        }
    }
}

void
ApvlvDocCache::sortLines ()
{
  for (auto line : mLines)
    {
      sort (line.mWords.begin (), line.mWords.end (),
            [] (const ApvlvWord &w1, const ApvlvWord &w2) {
              return w1.pos.p1x < w2.pos.p1x;
            });
    }

  sort (mLines.begin (), mLines.end (),
        [] (const ApvlvLine &line1, const ApvlvLine &line2) {
          return line1.pos.p1y < line2.pos.p1y;
        });
}

void
ApvlvDocCache::prepare_add (const char *word, WordListRectangle *results)
{
  for (auto itr : (*results)[0]) // need impl loop
    {
      itr.p1x = itr.p1x * mZoom;
      itr.p2x = itr.p2x * mZoom;
      itr.p1y = mHeight - itr.p1y * mZoom;
      itr.p2y = mHeight - itr.p2y * mZoom;

      vector<ApvlvLine>::iterator litr;
      for (litr = mLines.begin (); litr != mLines.end (); ++litr)
        {
          if (fabs (itr.p1y - litr->pos.p1y) < 0.0001
              && fabs (itr.p2y - litr->pos.p2y) < 0.0001)
            {
              break;
            }
        }

      if (litr != mLines.end ())
        {
          bool need = true;
          for (auto &mWord : litr->mWords)
            {
              auto w = &mWord;
              if (itr.p1x >= w->pos.p1x && itr.p2x <= w->pos.p2x)
                {
                  need = false;
                  break;
                }
              else if (itr.p1x <= w->pos.p1x && itr.p2x >= w->pos.p2x)
                {
                  w->pos.p1x = itr.p1x;
                  w->pos.p2x = itr.p2x;
                  w->word = word;
                  need = false;
                  break;
                }
            }

          if (need)
            {
              ApvlvWord w = { itr, word };
              litr->mWords.push_back (w);
              if (itr.p1x < litr->pos.p1x)
                {
                  litr->pos.p1x = itr.p1x;
                }
              if (itr.p2x > litr->pos.p2x)
                {
                  litr->pos.p2x = itr.p2x;
                }
            }
        }
      else
        {
          ApvlvLine line;
          line.pos = itr;
          ApvlvWord w = { itr, word };
          line.mWords.push_back (w);
          mLines.push_back (line);
        }
    }
}

double
ApvlvDocCache::getHeightOfLine (double y)
{
  for (const auto &line : mLines)
    {
      if (y > line.pos.p2y && y < line.pos.p1y)
        {
          return line.pos.p1y - line.pos.p2y;
        }
    }

  return APVLV_LINE_HEIGHT_DEFAULT;
}

double
ApvlvDocCache::getWidthOfWord (double x, double y)
{
  for (const auto &line : mLines)
    {
      if (y > line.pos.p2y && y < line.pos.p1y)
        {
          for (const auto &word : line.mWords)
            {
              if (x > word.pos.p1x && x < word.pos.p2x)
                {
                  return word.pos.p2x - word.pos.p1x;
                }
            }
        }
    }

  return APVLV_WORD_WIDTH_DEFAULT;
}

vector<CharRectangle>
ApvlvDocCache::getSelected (ApvlvPoint last, ApvlvPoint cur,
                            ApvlvVisualMode visual)
{
  vector<CharRectangle> poses;
  auto y1 = last.y, y2 = cur.y;
  auto x1 = last.x, x2 = cur.x;

  // g_return_val_if_fail (p1y <= p2y, poses);
  if (y1 > y2)
    {
      y1 = cur.y, y2 = last.y;
      x1 = cur.x, x2 = last.x;
    }

  auto lines = getlines (y1, y2);

  if (visual == ApvlvVisualMode::VISUAL_V)
    {
      if (lines.empty ())
        {
          CharRectangle pos = { x1, x2, y1, y2 };
          poses.push_back (pos);
        }
      else if (lines.size () == 1)
        {
          CharRectangle pos = { x1, x2, lines[0]->pos.p1y, lines[0]->pos.p2y };
          poses.push_back (pos);
        }
      else if (lines.size () == 2)
        {
          CharRectangle pos1 = { x1, lines[0]->pos.p2x, lines[0]->pos.p1y,
                                 lines[0]->pos.p2y };
          poses.push_back (pos1);
          CharRectangle pos2 = { lines[1]->pos.p1x, x2, lines[1]->pos.p1y,
                                 lines[1]->pos.p2y };
          poses.push_back (pos2);
        }
      else
        {
          CharRectangle pos1 = { x1, lines[0]->pos.p2x, lines[0]->pos.p1y,
                                 lines[0]->pos.p2y };
          poses.push_back (pos1);
          for (size_t lid = 1; lid < lines.size () - 1; ++lid)
            {
              poses.push_back (lines[lid]->pos);
            }
          auto lastid = lines.size () - 1;
          CharRectangle pos2
              = { lines[lastid]->pos.p1x, x2, lines[lastid]->pos.p1y,
                  lines[lastid]->pos.p2y };
          poses.push_back (pos2);
        }
    }
  else if (visual == ApvlvVisualMode::VISUAL_CTRL_V)
    {
      CharRectangle pos = { x1, x2, y1, y2 };
      poses.push_back (pos);
    }
  else
    {
      CharRectangle pos = { cur.x, cur.x + int (APVLV_CURSOR_WIDTH_DEFAULT),
                            cur.y, cur.y + APVLV_LINE_HEIGHT_DEFAULT };
      if (!lines.empty ())
        {
          pos.p1y = lines[lines.size () - 1]->pos.p1y;
          pos.p2y = lines[lines.size () - 1]->pos.p2y;
        }
      poses.push_back (pos);
    }

  return poses;
}

vector<ApvlvLine *>
ApvlvDocCache::getlines (double y1, double y2)
{
  vector<ApvlvLine *> lines;

  for (auto &mLine : mLines)
    {
      auto line = &mLine;
      if (line->pos.p2y >= y1 && line->pos.p1y <= y2)
        lines.push_back (line);
    }

  return lines;
}

void
ApvlvDoc::display ()
{
  if (filename ())
    {
      vector<string> labels;

      int pn = pagenumber ();
      int totpn = file ()->sum ();
      double sr = scrollrate ();
      int tmprtimes = 0;
      srtranslate (tmprtimes, sr, false);

      char temp[256];
      auto systempath = filesystem::path (filename ());
      auto bn = systempath.filename ();
      snprintf (temp, sizeof temp, "%s", bn.string ().c_str ());
      labels.emplace_back (temp);
      snprintf (temp, sizeof temp, "%d/%d", pn, totpn);
      labels.emplace_back (temp);
      snprintf (temp, sizeof temp, "%d%%", (int)(zoomvalue () * 100));
      labels.emplace_back (temp);
      snprintf (temp, sizeof temp, "%d%%",
                (int)((sr + pn - 1.0) / totpn * 100));
      labels.emplace_back (temp);

      mStatus->showMessages (labels);

      mContent->setCurrentIndex (mFilestr, mPagenum, mAnchor.c_str ());
    }
}

bool
ApvlvDoc::find (const char *str)
{
  auto cache = mCurrentCache[mCurrentImage->mId].get ();
  auto results = mFile->pageSearch (cache->getpagenum (), str, false);

  for (auto const &wordpos : *results)
    {
      auto pos = wordpos[0];
      if (pos.p1y > mCurPoint.y
          || (pos.p1y == mCurPoint.y && pos.p1x > mCurPoint.x))
        {
          Rectangle pos1 = { pos.p1x, pos.p2x, cache->getheight () - pos.p1y,
                             cache->getheight () - pos.p2y };
          blankarea (mCurrentImage, pos1, nullptr, cache->getwidth (),
                     cache->getheight ());
          auto p = cache->getbuf (true);
          mCurrentImage->setImage (p);
          mLastPoint = { pos1.p1x, pos1.p1y };
          mCurPoint = { pos1.p2x, pos1.p2y };
          break;
        }
    }

  return true;
}

ApvlvImage::ApvlvImage (ApvlvDoc *doc, int id)
{
  mDoc = doc;
  mId = id;
  setFocusPolicy (Qt::NoFocus);
}

void
ApvlvImage::setImage (const QImage &buf)
{
  setPixmap (QPixmap::fromImage (buf));
}

void
ApvlvImage::toCacheSize (double x, double y, ApvlvDocCache *cache, double *rx,
                         double *ry)
{
  // need impl
  auto x_root = width ();
  auto y_root = height ();
  if (rx)
    *rx = x - (x_root - double (cache->getwidth ())) / 2;
  if (ry)
    *ry = y - (y_root - double (cache->getheight ())) / 2;
}

void
ApvlvImage::copytoclipboard_cb ()
{
  mDoc->yank (this, 1);
  mDoc->mInVisual = ApvlvVisualMode::VISUAL_NONE;
  mDoc->updateCurPoint (mDoc->mCurPoint.x, mDoc->mCurPoint.y, true);
}

void
ApvlvImage::underline_cb ()
{
  mDoc->annotUnderline (this);
  mDoc->mInVisual = ApvlvVisualMode::VISUAL_NONE;
  mDoc->updateCurPoint (mDoc->mCurPoint.x, mDoc->mCurPoint.y, true);
}

void
ApvlvImage::annotate_cb ()
{
  mDoc->annotText (this);
  mDoc->mInVisual = ApvlvVisualMode::VISUAL_NONE;
  mDoc->updateCurPoint (mDoc->mCurPoint.x, mDoc->mCurPoint.y, true);
}

void
ApvlvImage::comment_cb ()
{
  mDoc->commentText (this);
  mDoc->mInVisual = ApvlvVisualMode::VISUAL_NONE;
  mDoc->updateCurPoint (mDoc->mCurPoint.x, mDoc->mCurPoint.y, true);
}

void
ApvlvImage::contextMenuEvent (QContextMenuEvent *ev)
{
  auto menu = new QMenu (this);

  auto cache = mDoc->mCurrentCache[mId].get ();
  auto cursor_pos = QCursor::pos ();
  CharRectangle pos = { (double)cursor_pos.x (), (double)cursor_pos.x (),
                        (double)cursor_pos.y (), (double)cursor_pos.y () };
  mDoc->mCurrentAnnotText = cache->annotAtPos (pos);

  if (mDoc->mCurrentAnnotText == nullptr
      && mDoc->mInVisual == ApvlvVisualMode::VISUAL_NONE)
    {
      delete menu;
      return;
    }

  else if (mDoc->mCurrentAnnotText != nullptr)
    {
      QWidget *item;

      if (mDoc->mCurrentAnnotText->type == APVLV_ANNOT_TEXT)
        {
          auto edit_action = new QAction ("Edit Annotation text", this);
          QObject::connect (edit_action, SIGNAL (triggered ()), this,
                            SLOT (edit_annotation_cb ()));
          menu->addAction (edit_action);
        }

      auto del_action = new QAction ("Delete Annotation", this);
      QObject::connect (del_action, SIGNAL (triggered ()), this,
                        SLOT (delete_annotation_cb ()));
      menu->addAction (del_action);
    }

  else if (mDoc->mInVisual != ApvlvVisualMode::VISUAL_NONE)
    {
      auto copy_action = new QAction ("Copy to clipboard", this);
      QObject::connect (copy_action, SIGNAL (triggered ()), this,
                        SLOT (copytoclipboard_cb ()));
      menu->addAction (copy_action);

      auto underline_action = new QAction ("Under line", this);
      QObject::connect (underline_action, SIGNAL (triggered ()), this,
                        SLOT (underline_cb ()));
      menu->addAction (underline_action);

      auto annt_action = new QAction ("Annotate", this);
      QObject::connect (annt_action, SIGNAL (triggered ()), this,
                        SLOT (annotate_cb ()));
      menu->addAction (annt_action);

      auto comment_action = new QAction ("Comment", this);
      QObject::connect (comment_action, SIGNAL (triggered ()), this,
                        SLOT (comment_cb ()));
      menu->addAction (comment_action);
    }

  menu->exec (QCursor::pos ());
}

void
ApvlvImage::mouseMoveEvent (QMouseEvent *evt)
{
  double x, y;
  auto cache = mDoc->mCurrentCache[mId].get ();

  auto position = evt->position ();
  toCacheSize (position.x (), position.y (), cache, &x, &y);
  mDoc->mInVisual = ApvlvVisualMode::VISUAL_V;
  if (evt->modifiers () & Qt::ControlModifier)
    mDoc->mInVisual = ApvlvVisualMode::VISUAL_CTRL_V;
  mDoc->updateCurPoint (x, y, false);
  mDoc->blank (this);
}

void
ApvlvImage::mousePressEvent (QMouseEvent *evt)
{
  auto cache = mDoc->mCurrentCache[mId].get ();

  double x, y;
  auto position = evt->position ();
  mDoc->mCurrentImage->toCacheSize (position.x (), position.y (), cache, &x,
                                    &y);
  // this is a manual method to test double click
  // I think, a normal user double click will in 500 millseconds
  if (evt->timestamp () - mDoc->mLastpress < 500)
    {
      mDoc->mInVisual = ApvlvVisualMode::VISUAL_NONE;
      mDoc->doubleClickBlank (this, x, y);
    }
  else
    {
      mDoc->mInVisual = ApvlvVisualMode::VISUAL_NONE;
      mDoc->updateLastPoint (x, y);
      mDoc->updateCurPoint (x, y, false);
      mDoc->blank (this);
    }

  mDoc->mLastpress = evt->timestamp ();
}

void
ApvlvImage::mouseReleaseEvent (QMouseEvent *evt)
{
}

}

// Local Variables:
// mode: c++
// End:

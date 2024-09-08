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
/* @CPPFILE ApvlvFrame.cc
 *
 *  Author: Alf <naihe2010@126.com>
 */

#include <QBuffer>
#include <QMessageBox>
#include <QSplitter>
#include <filesystem>
#include <memory>

#include "ApvlvFileWidget.h"
#include "ApvlvFrame.h"
#include "ApvlvImageWidget.h"
#include "ApvlvInfo.h"
#include "ApvlvParams.h"
#include "ApvlvView.h"
#include "ApvlvWebViewWidget.h"

namespace apvlv
{

using namespace Qt;

ApvlvFrame::ApvlvFrame (ApvlvView *view)
{
  mInuse = true;

  mView = view;

  mProCmd = 0;

  mZoommode = NORMAL;

  mSearchResults = nullptr;
  mSearchStr = "";

  mVbox = new QVBoxLayout ();
  setLayout (mVbox);

  mPaned = new QSplitter ();
  mPaned->setHandleWidth (4);
  mContentWidth = DEFAULT_CONTENT_WIDTH;

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
  mContent->setFrame (this);
  QObject::connect (this, SIGNAL (indexGenerited (const FileIndex &)),
                    mContent, SLOT (set_index (const FileIndex &)));

  mPaned->addWidget (mContent);

  mStatus = new ApvlvStatus ();
  mVbox->addWidget (mStatus, 0);
  qDebug ("ApvlvFrame: %p be created", this);
}

ApvlvFrame::~ApvlvFrame () { qDebug ("ApvlvFrame: %p be freed", this); }

void
ApvlvFrame::inuse (bool use)
{
  mInuse = use;
}

bool
ApvlvFrame::inuse ()
{
  return mInuse;
}

const char *
ApvlvFrame::filename ()
{
  return mFilestr.empty () ? nullptr : mFilestr.c_str ();
}

bool
ApvlvFrame::print (int ct)
{
  return false;
}

int
ApvlvFrame::getskip ()
{
  return mSkip;
}

void
ApvlvFrame::setskip (int ct)
{
  mSkip = ct;
}

void
ApvlvFrame::toggleContent ()
{
  auto show = !isShowContent ();
  toggleContent (show);
}

void
ApvlvFrame::toggleContent (bool show)
{
  if (show)
    {
      if (!mContent->isReady ())
        {
          qWarning ("file %s has no content", mFilestr.c_str ());
          show = false;
        }
    }

  auto sizes = mPaned->sizes ();
  if (show)
    {
      mDirIndex = {};
      if (sizes[0] == 0)
        {
          auto psize = mPaned->size ();
          sizes = { mContentWidth, psize.width () - mPaned->handleWidth ()
                                       - DEFAULT_CONTENT_WIDTH };
          mPaned->setSizes (sizes);
        }
    }
  else
    {
      if (sizes[0] != 0)
        mContentWidth = sizes[0];

      auto psize = mPaned->size ();
      sizes = { 0, psize.width () - mPaned->handleWidth () };
      mPaned->setSizes (sizes);
    }
}

void
ApvlvFrame::setActive (bool act)
{
  mActive = act;

  auto wid = mWidget->widget ();
  if (act)
    {
      wid->setFocus ();
    }
  else
    {
      wid->clearFocus ();
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
ApvlvFrame::setDirIndex (const string &path)
{
  mDirIndex = { "", 0, path, FILE_INDEX_DIR };
  mDirIndex.loadDirectory (path);
  emit indexGenerited (mDirIndex);
  toggleContent (true);
}

bool
ApvlvFrame::toggledControlContent (bool is_right)
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

bool
ApvlvFrame::isShowContent ()
{
  auto sizes = mPaned->sizes ();
  return sizes[0] > 1;
}

bool
ApvlvFrame::isControlledContent ()
{
  if (!isShowContent ())
    return false;

  return mContent->isFocused ();
}

ApvlvFrame *
ApvlvFrame::findByWidget (QWidget *widget)
{
  for (auto doc = widget; doc != nullptr; doc = doc->parentWidget ())
    {
      if (doc->inherits ("apvlv::ApvlvFrame"))
        return dynamic_cast<ApvlvFrame *> (doc);
    }

  return nullptr;
}

ApvlvStatus::ApvlvStatus ()
{
  setFrameShape (QFrame::NoFrame);
  auto layout = new QHBoxLayout ();
  qDebug ("status layout: %p", layout);
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
          label->setText (QString::fromLocal8Bit (msgs[ind]));
        }
      else
        {
          auto label = new QLabel ();
          label->setText (QString::fromLocal8Bit (msgs[ind]));
          newlabels.push_back (label);
        }
    }

  auto hbox = layout ();
  for (auto label : newlabels)
    {
      hbox->addWidget (label);
    }
}

ReturnType
ApvlvFrame::subprocess (int ct, uint key)
{
  auto zoomrate = mWidget->zoomrate ();
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
          snprintf (temp, sizeof temp, "%f", zoomrate * 1.1);
          setzoom (temp);
          updateStatus ();
        }
      else if (key == 'o')
        {
          char temp[0x10];
          snprintf (temp, sizeof temp, "%f", zoomrate / 1.1);
          setzoom (temp);
          updateStatus ();
        }
      else if (key == 'h')
        {
          setzoom ("fitheight");
          updateStatus ();
        }
      else if (key == 'w')
        {
          setzoom ("fitwidth");
          updateStatus ();
        }
      break;

    default:
      return NO_MATCH;
      break;
    }

  return MATCH;
}

ReturnType
ApvlvFrame::process (int has, int ct, uint key)
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
      mWidget->scrollTo (0.0, 0.0);
      break;
    case 'M':
      mWidget->scrollTo (0.0, 0.5);
      break;
    case 'L':
      mWidget->scrollTo (0.0, 1.0);
      break;
    case '0':
      mWidget->scrollLeft (INT_MAX);
      break;
    case '$':
      mWidget->scrollRight (INT_MAX);
      break;
    case CTRL ('p'):
    case Key_Up:
    case 'k':
      if (isControlledContent ())
        {
          mContent->scrollUp (ct);
          contentShowPage (mContent->currentIndex (), false);
        }
      else
        {
          mWidget->scrollUp (ct);
          updateStatus ();
        }
      break;
    case CTRL ('n'):
    case CTRL ('j'):
    case Key_Down:
    case 'j':
      if (isControlledContent ())
        {
          mContent->scrollDown (ct);
          contentShowPage (mContent->currentIndex (), false);
        }
      else
        {
          mWidget->scrollDown (ct);
          updateStatus ();
        }
      break;
    case Key_Backspace:
    case Key_Left:
    case CTRL ('h'):
    case 'h':
      if (isControlledContent ())
        {
          mContent->scrollLeft (ct);
          contentShowPage (mContent->currentIndex (), false);
        }
      else
        {
          mWidget->scrollLeft (ct);
          updateStatus ();
        }
      break;
    case Key_Space:
    case Key_Right:
    case CTRL ('l'):
    case 'l':
      if (isControlledContent ())
        {
          mContent->scrollRight (ct);
          contentShowPage (mContent->currentIndex (), false);
        }
      else
        {
          mWidget->scrollRight (ct);
          updateStatus ();
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
      // togglevisual (char (key));
      break;
    case ('y'):
      // yank (mCurrentImage, ct);
      // mInVisual = ApvlvVisualMode::VISUAL_NONE;
      // blank (mCurrentImage);
      break;
    case ('s'):
      setskip (ct);
      break;
    case ('c'):
      toggleContent ();
      break;
    case ('A'):
      // mCurrentImage->annotate_cb ();
      break;
    case ('U'):
      // mCurrentImage->underline_cb ();
      break;
    case ('C'):
      // mCurrentImage->comment_cb ();
      break;
    default:
      return NO_MATCH;
      break;
    }

  return MATCH;
}

ApvlvFrame *
ApvlvFrame::copy ()
{
  char rate[16];
  snprintf (rate, sizeof rate, "%f", mWidget->zoomrate ());
  auto *ndoc = new ApvlvFrame (mView);
  ndoc->loadfile (mFilestr, false, false);
  ndoc->showpage (mWidget->pageNumber (), mWidget->scrollRate ());
  return ndoc;
}

void
ApvlvFrame::setzoom (const char *z)
{
  auto zoomrate = mWidget->zoomrate ();
  if (z != nullptr)
    {
      if (strcasecmp (z, "normal") == 0)
        {
          mZoommode = NORMAL;
          zoomrate = 1.2;
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
              zoomrate = d;
            }
        }
    }

  if (mFile != nullptr)
    {
      int pn = max (0, pageNumber () - 1);
      auto size = mFile->pageSizeF (pn, 0);

      if (size.width > 0 && size.height > 0)
        {
          auto wid = mWidget->widget ();
          if (mZoommode == FITWIDTH)
            {
              auto x_root = wid->width ();
              zoomrate = x_root / size.width;
            }
          else if (mZoommode == FITHEIGHT)
            {
              auto y_root = wid->height ();
              zoomrate = y_root / size.height;
            }
        }
      mWidget->setZoomrate (zoomrate);
    }
}

bool
ApvlvFrame::saveLastPosition (const string &filename)
{
  if (filename.empty () || helppdf == filename || gParams->valueb ("noinfo"))
    {
      return false;
    }

  bool ret = gInfo->updateFile (mWidget->pageNumber (), mSkip,
                                mWidget->scrollRate (), filename);
  return ret;
}

bool
ApvlvFrame::loadLastPosition (const string &filename)
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
      gInfo->updateFile (0, 0.0, mWidget->zoomrate (), filename);
    }

  return ret;
}

bool
ApvlvFrame::reload ()
{
  saveLastPosition (filename ());
  return loadfile (mFilestr, false, isShowContent ());
}

int
ApvlvFrame::pageNumber ()
{
  return mWidget ? mWidget->pageNumber () : 0;
}

bool
ApvlvFrame::loadfile (const string &filename, bool check, bool show_content)
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

  mFile = File::newFile (filename, false);

  // qDebug ("mFile = %p", mFile);
  if (mFile != nullptr)
    {
      emit indexGenerited (mFile->getIndex ());

      mFilestr = filename;

      if (mFile->sum () <= 1)
        {
          qDebug ("sum () = %d", mFile->sum ());
        }

      // qDebug ("pagesum () = %d", mFile->sum ());

      setWidget (mFile->getDisplayType ());

      loadLastPosition (filename);

      setActive (true);

      mSearchStr = "";
      mSearchResults = nullptr;

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

void
ApvlvFrame::markposition (const char s)
{
  ApvlvDocPosition adp = { mWidget->pageNumber (), mWidget->scrollRate () };
  mPositions[s] = adp;
}

void
ApvlvFrame::jump (const char s)
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
ApvlvFrame::showpage (int p, double s)
{
  auto rp = mFile->pageNumberWrap (p);
  if (rp < 0)
    return;

  mWidget->setAnchor ("");

  if (!mZoominit)
    {
      mZoominit = true;
      setzoom (nullptr);
    }

  refresh (rp, s);
}

void
ApvlvFrame::showpage (int p, const string &anchor)
{
  auto pn = mFile->pageNumberWrap (p);
  if (pn < 0)
    return;
  if (!mZoominit)
    {
      mZoominit = true;
      setzoom (nullptr);
    }

  mWidget->showPage (p, anchor);
  updateStatus ();
}

void
ApvlvFrame::nextpage (int times)
{
  showpage (mWidget->pageNumber () + times, 0.0f);
}

void
ApvlvFrame::prepage (int times)
{
  showpage (mWidget->pageNumber () - times, 0.0f);
}

void
ApvlvFrame::refresh (int pn, double s)
{
  if (mFile == nullptr)
    return;

  mWidget->showPage (pn, s);
  updateStatus ();
}

void
ApvlvFrame::halfnextpage (int times)
{
  double sr = mWidget->scrollRate ();
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

  showpage (mWidget->pageNumber () + rtimes, sr);
}

void
ApvlvFrame::halfprepage (int times)
{
  double sr = mWidget->scrollRate ();
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

  showpage (mWidget->pageNumber () - rtimes, sr);
}

bool
ApvlvFrame::needsearch (const string &str, bool reverse)
{
  if (mFile == nullptr)
    return false;

  // search a different string
  if (!str.empty () && str != mSearchStr)
    {
      qDebug ("different string.");
      mSearchStr = str;
      return true;
    }

  else if (mSearchResults == nullptr)
    {
      qDebug ("no result.");
      return true;
    }

  // same string, but need to search next page
  else if ((!reverse
            && mWidget->searchSelect () == (int)mSearchResults->size () - 1)
           || (reverse && mWidget->searchSelect () == 0))
    {
      qDebug ("same, but need next string: s: %d, sel: %d, max: %lu.", reverse,
              mWidget->searchSelect (), mSearchResults->size ());
      return true;
    }

  // same string, not need search, but has zoomed
  else
    {
      qDebug ("same, not need next string. sel: %d, max: %ld",
              mWidget->searchSelect (), mSearchResults->size ());
      if (!reverse)
        {
          setHighlightAndIndex (*mSearchResults, mWidget->searchSelect () + 1);
        }
      else
        {
          setHighlightAndIndex (*mSearchResults, mWidget->searchSelect () - 1);
        }

      return false;
    }

  return false;
}

bool
ApvlvFrame::search (const char *str, bool reverse)
{
  if (*str == '\0' && mSearchStr.empty ())
    {
      return false;
    }

  if (*str)
    {
      mSearchCmd = (reverse ? BACKSEARCH : SEARCH);
    }

  if (!needsearch (str, reverse))
    {
      return true;
    }

  mSearchResults = nullptr;
  unsetHighlight ();

  bool wrap = gParams->valueb ("wrapscan");

  int i = mWidget->pageNumber ();
  int sum = mFile->sum (), from = i;
  bool search = false;
  while (true)
    {
      if (*str != 0 || search)
        {
          mSearchResults
              = mFile->pageSearch ((i + sum) % sum, mSearchStr.c_str ());
          if (mSearchResults != nullptr && !mSearchResults->empty ())
            {
              if (i != mWidget->pageNumber ())
                showpage (i, 0.0);
              auto results = *mSearchResults;
              auto sel = 0;
              if (reverse)
                sel = static_cast<int> (results.size () - 1);
              setHighlightAndIndex (results, sel);
              return true;
            }
        }

      search = true;

      if (!reverse && i < (wrap ? (from + sum) : (sum - 1)))
        {
          // qDebug ("wrap: %d, i++:", wrap, i, i + 1);
          i++;
        }
      else if (reverse && i > (wrap ? (from - sum) : 0))
        {
          // qDebug ("wrap: %d, i--: %d", wrap, i - 1);
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
ApvlvFrame::totext (const char *file)
{
  if (mFile == nullptr)
    return false;

  auto pn = mWidget->pageNumber ();
  string txt;
  auto size = mFile->pageSizeF (pn, 0);
  bool ret = mFile->pageText (pn, { 0, 0, size.width, size.height }, txt);
  if (ret)
    {
      // need impl g_file_set_contents (file, txt, -1, nullptr);
      return true;
    }
  return false;
}

bool
ApvlvFrame::rotate (int ct)
{
  // just hack
  if (ct == 1)
    ct = 90;

  if (ct % 90 != 0)
    {
      mView->errormessage ("Not a 90 times value: %d", ct);
      return false;
    }

  auto rotate = mWidget->rotate ();
  rotate += ct;
  while (rotate < 0)
    {
      rotate += 360;
    }
  while (rotate > 360)
    {
      rotate -= 360;
    }
  mWidget->setRotate (rotate);
  refresh (mWidget->pageNumber (), 0.0);
  return true;
}

void
ApvlvFrame::gotolink (int ct)
{
}

void
ApvlvFrame::returnlink (int ct)
{
}

void
ApvlvFrame::contentShowPage (const FileIndex *index, bool force)
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

  if (index->type == FileIndexType::FILE_INDEX_PAGE)
    {
      if (index->page != mWidget->pageNumber ()
          || index->anchor != mWidget->anchor ())
        showpage (index->page, index->anchor);
    }
}

void
ApvlvFrame::setWidget (DISPLAY_TYPE type)
{
  if (type == DISPLAY_TYPE_IMAGE)
    {
      mWidget = make_unique<ImageWidget> ();
      mWidget->setFile (mFile);
    }
  else if (type == DISPLAY_TYPE_HTML)
    {
      mWidget = make_unique<WebViewWidget> ();
      mWidget->setFile (mFile);
    }
  else
    {
      mWidget.reset (mFile->getWidget ());
    }
  if (mPaned->count () == 1)
    mPaned->addWidget (mWidget->widget ());
  else
    mPaned->replaceWidget (1, mWidget->widget ());
}

void
ApvlvFrame::unsetHighlight ()
{
  mWidget->setSearchStr ("");
  mWidget->setSearchSelect (0);
  mWidget->setSearchResults ({});
}

void
ApvlvFrame::setHighlightAndIndex (const WordListRectangle &poses, int sel)
{
  if (!poses[sel].word.empty ())
    {
      mWidget->setSearchStr (poses[sel].word);
    }
  mWidget->setSearchSelect (sel);
  mWidget->setSearchResults (poses);

  auto sr = mWidget->scrollRate ();
  if (!poses[sel].rect_list.empty ())
    {
      auto rect = poses[sel].rect_list[0];
      auto size
          = mFile->pageSizeF (mWidget->pageNumber (), mWidget->rotate ());
      if (size.height > 0)
        {
          sr = rect.p2y / size.height;
        }
    }

  refresh (mWidget->pageNumber (), sr);
}

void
ApvlvFrame::updateStatus ()
{
  if (filename ())
    {
      vector<string> labels;

      int pn = pageNumber () + 1;
      int totpn = mFile->sum ();
      double sr = mWidget->scrollRate ();
      string anchor = mWidget->anchor ();
      // int tmprtimes = 0;

      char temp[256];
      auto systempath = filesystem::path (filename ());
      auto bn = systempath.filename ();
      snprintf (temp, sizeof temp, "%s", bn.string ().c_str ());
      labels.emplace_back (temp);
      snprintf (temp, sizeof temp, "%d/%d", pn, totpn);
      labels.emplace_back (temp);
      snprintf (temp, sizeof temp, "%d%%", (int)(mWidget->zoomrate () * 100));
      labels.emplace_back (temp);
      snprintf (temp, sizeof temp, "%d%%",
                (int)((sr + pn - 1.0) / totpn * 100));
      labels.emplace_back (temp);

      mStatus->showMessages (labels);

      mContent->setCurrentIndex (mFilestr, mWidget->pageNumber (),
                                 mWidget->anchor ());
    }
}

}

// Local Variables:
// mode: c++
// End:

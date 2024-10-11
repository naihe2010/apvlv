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
#include <fstream>
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

using namespace std;
using namespace Qt;
using namespace CommandModeType;

ApvlvFrame::ApvlvFrame (ApvlvView *view)
{
  mInuse = true;

  mView = view;

  mProCmd = 0;

  mZoommode = ZoomMode::NORMAL;

  mSearchResults = nullptr;
  mSearchStr = "";

  mVbox = new QVBoxLayout ();
  setLayout (mVbox);

  mPaned = new QSplitter ();
  mPaned->setHandleWidth (4);
  mContentWidth = DEFAULT_CONTENT_WIDTH;

  auto f_width = gParams->getIntOrDefault ("fix_width");
  auto f_height = gParams->getIntOrDefault ("fix_height");

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
  auto guiopt = gParams->getStringOrDefault ("guioptions");
  if (guiopt.find ("S") == string::npos)
    {
      mStatus->hide ();
    }
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
  mDirIndex = { "", 0, path, FileIndexType::DIR };
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

  if (auto controlled = isControlledContent (); !controlled && !is_right)
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
  for (std::size_t ind = 0; ind < msgs.size (); ++ind)
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

CmdReturn
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
          setzoom (zoomrate * 1.1);
          updateStatus ();
        }
      else if (key == 'o')
        {
          setzoom (zoomrate / 1.1);
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
      return CmdReturn::NO_MATCH;
      break;
    }

  return CmdReturn::MATCH;
}

CmdReturn
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
      return CmdReturn::NEED_MORE;
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
          contentShowPage (mContent->currentItemFileIndex (), false);
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
          contentShowPage (mContent->currentItemFileIndex (), false);
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
          contentShowPage (mContent->currentItemFileIndex (), false);
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
          contentShowPage (mContent->currentItemFileIndex (), false);
        }
      else
        {
          mWidget->scrollRight (ct);
          updateStatus ();
        }
      break;
    case Key_Return:
      contentShowPage (mContent->currentItemFileIndex (), true);
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
      return CmdReturn::NEED_MORE;
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
      break;
    case 's':
      setskip (ct);
      break;
    case 'c':
      toggleContent ();
      break;
    default:
      return CmdReturn::NO_MATCH;
      break;
    }

  return CmdReturn::MATCH;
}

ApvlvFrame *
ApvlvFrame::copy ()
{
  auto *ndoc = new ApvlvFrame (mView);
  ndoc->loadfile (mFilestr, false, false);
  ndoc->showpage (mWidget->pageNumber (), mWidget->scrollRate ());
  return ndoc;
}

void
ApvlvFrame::setzoom (double zm)
{
  mZoommode = ZoomMode::CUSTOM;
  mWidget->setZoomrate (zm);
}

void
ApvlvFrame::setzoom (const char *z)
{
  auto zoomrate = mWidget->zoomrate ();
  if (z != nullptr)
    {
      string_view sv (z);
      if (sv == "normal")
        {
          mZoommode = ZoomMode::NORMAL;
          zoomrate = 1.2;
        }
      else if (sv == "fitwidth")
        {
          mZoommode = ZoomMode::FITWIDTH;
        }
      else if (sv == "fitheight")
        {
          mZoommode = ZoomMode::FITHEIGHT;
        }
      else
        {
          double d = strtod (z, nullptr);
          if (d > 0)
            {
              mZoommode = ZoomMode::CUSTOM;
              zoomrate = d;
            }
        }
    }

  if (mFile)
    {
      int pn = std::max (0, pageNumber () - 1);
      auto size = mFile->pageSizeF (pn, 0);

      if (size.width > 0 && size.height > 0)
        {
          auto wid = mWidget->widget ();
          if (mZoommode == ZoomMode::FITWIDTH)
            {
              auto x_root = wid->width ();
              zoomrate = x_root / size.width;
            }
          else if (mZoommode == ZoomMode::FITHEIGHT)
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
  if (filename.empty () || helppdf == filename
      || gParams->getBoolOrDefault ("noinfo", false))
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
  if (filename.empty () || helppdf == filename
      || gParams->getBoolOrDefault ("noinfo"))
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
  if (check && filename == mFilestr)
    {
      return false;
    }

  mFile = File::loadFile (filename);

  if (mFile)
    {
      emit indexGenerited (mFile->getIndex ());

      mFilestr = filename;

      if (mFile->sum () <= 1)
        {
          qDebug ("sum () = %d", mFile->sum ());
        }

      setWidget (mFile->getDisplayType ());

      loadLastPosition (filename);

      setActive (true);

      mSearchStr = "";
      mSearchResults = nullptr;

      if (gParams->getIntOrDefault ("autoreload") > 0)
        {
          mWatcher = make_unique<QFileSystemWatcher> ();
          QObject::connect (mWatcher.get (), SIGNAL (fileChanged ()), this,
                            SLOT (changed_cb ()));

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
      mSearchCmd
          = (reverse ? CommandModeType::BACKSEARCH : CommandModeType::SEARCH);
    }

  if (!needsearch (str, reverse))
    {
      return true;
    }

  mSearchResults = nullptr;
  unsetHighlight ();

  auto wrap = gParams->getBoolOrDefault ("wrapscan");

  auto i = mWidget->pageNumber ();
  auto sum = mFile->sum ();
  auto from = i;
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
          i++;
        }
      else if (reverse && i > (wrap ? (from - sum) : 0))
        {
          i--;
        }
      else
        {
          mView->errorMessage (string ("can't find word: "), mSearchStr);
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
      fstream fs{ filename (), ios::out };
      if (fs.is_open ())
        {
          fs.write (txt.c_str (), txt.length ());
          fs.close ();
          return true;
        }
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
      mView->errorMessage ("Not a 90 times value: ", ct);
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
  // need impl
}

void
ApvlvFrame::returnlink (int ct)
{
  // need impl
}

void
ApvlvFrame::contentShowPage (const FileIndex *index, bool force)
{
  if (index == nullptr)
    return;

  if (index->type == FileIndexType::FILE)
    {
      loadfile (index->path, true, true);
      return;
    }

  auto file = mContent->currentFileFileIndex ();
  if (file && file->path != mFilestr)
    loadfile (file->path, true, true);

  if (index->type == FileIndexType::PAGE)
    {
      if (index->page != mWidget->pageNumber ()
          || index->anchor != mWidget->anchor ())
        showpage (index->page, index->anchor);
    }
}

void
ApvlvFrame::setWidget (DISPLAY_TYPE type)
{
  if (type == DISPLAY_TYPE::IMAGE)
    {
      mWidget = make_unique<ImageWidget> ();
      mWidget->setFile (mFile.get ());
    }
  else if (type == DISPLAY_TYPE::HTML)
    {
      mWidget = make_unique<WebViewWidget> ();
      mWidget->setFile (mFile.get ());
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
      auto zm = mWidget->zoomrate ();
      auto sr = mWidget->scrollRate ();
      string anchor = mWidget->anchor ();

      auto systempath = filesystem::path (filename ());
      auto bn = systempath.filename ();
      labels.emplace_back (bn.string ());

      auto ss = QString ("%1/%2").arg (pn).arg (totpn);
      labels.emplace_back (ss.toStdString ());

      ss = QString ("%1%").arg (static_cast<int> (zm * 100));
      labels.emplace_back (ss.toStdString ());

      ss = QString ("%1%").arg (static_cast<int> (sr * 100));
      labels.emplace_back (ss.toStdString ());

      mStatus->showMessages (labels);

      mContent->setCurrentIndex (mFilestr, mWidget->pageNumber (),
                                 mWidget->anchor ());
    }
}

bool
ApvlvFrame::isStatusHidden ()
{
  return mStatus->isHidden ();
}

void
ApvlvFrame::statusShow ()
{
  mStatus->show ();
}

void
ApvlvFrame::statusHide ()
{
  mStatus->hide ();
}

}

// Local Variables:
// mode: c++
// End:
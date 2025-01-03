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
#include <QClipboard>
#include <QMessageBox>
#include <filesystem>
#include <fstream>
#include <memory>

#include "ApvlvFileWidget.h"
#include "ApvlvFrame.h"
#include "ApvlvImageWidget.h"
#include "ApvlvInfo.h"
#include "ApvlvParams.h"
#include "ApvlvView.h"
#include "ApvlvWeb.h"
#include "ApvlvWebViewWidget.h"

namespace apvlv
{

using namespace std;
using namespace Qt;
using namespace CommandModeType;

std::vector<const char *> ApvlvFrame::ZoomLabel = {
  QT_TR_NOOP ("Default"),
  QT_TR_NOOP ("Fit Width"),
  QT_TR_NOOP ("Fit Height"),
  QT_TR_NOOP ("Custom"),
};

ApvlvFrame::ApvlvFrame (ApvlvView *view) : mToolStatus (this)
{
  mInuse = true;

  mView = view;

  mProCmd = 0;

  mZoomMode = ZoomMode::NORMAL;

  mSearchResults = nullptr;
  mSearchStr = "";

  setLayout (&mVbox);

  mPaned.setHandleWidth (4);
  mDirectoryWidth = DEFAULT_CONTENT_WIDTH;

  auto f_width = ApvlvParams::instance ()->getIntOrDefault ("fix_width");
  auto f_height = ApvlvParams::instance ()->getIntOrDefault ("fix_height");

  if (f_width > 0 && f_height > 0)
    {
      mPaned.setFixedSize (f_width, f_height);

      mVbox.addLayout (&mHBoxLayout, 1);
      mHBoxLayout.addWidget (&mPaned, 0);
    }
  else
    {
      mVbox.addWidget (&mPaned, 1);
    }

  mDirectory.setFrame (this);
  QObject::connect (this, SIGNAL (indexGenerited (const FileIndex &)),
                    &mDirectory, SLOT (setIndex (const FileIndex &)));

  // left is Directory
  mPaned.addWidget (&mDirectory);

  // Right is Text
  mPaned.addWidget (&mTextFrame);
  mTextFrame.setLayout (&mTextLayout);
  mTextLayout.addWidget (&mToolStatus, 0);
  auto guiopt = ApvlvParams::instance ()->getStringOrDefault ("guioptions");
  if (guiopt.find ('S') == string::npos)
    {
      mToolStatus.hide ();
    }

  mVbox.addWidget (&mStatus, 0);
  if (guiopt.find ('s') == string::npos)
    {
      mStatus.hide ();
    }
  qDebug () << "ApvlvFrame: " << this << " be created";
}

ApvlvFrame::~ApvlvFrame ()
{
  qDebug () << "ApvlvFrame: " << this << " be freed";
  saveLastPosition (mFilestr);
}

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

bool
ApvlvFrame::loadUri (const string &uri)
{
  mFile = make_unique<ApvlvWEB> ();
  mFile->load (uri);
  setWidget (mFile->getDisplayType ());
  refresh (0, 0.0);
  return true;
}

const char *
ApvlvFrame::filename ()
{
  return mFilestr.empty () ? nullptr : mFilestr.c_str ();
}

bool
ApvlvFrame::print ([[maybe_unused]] int ct)
{
  return false;
}

int
ApvlvFrame::getSkip ()
{
  return mSkip;
}

void
ApvlvFrame::setSkip (int ct)
{
  mSkip = ct;
}

void
ApvlvFrame::toggleDirectory ()
{
  auto show = !isShowDirectory ();
  toggleDirectory (show);
}

void
ApvlvFrame::toggleDirectory (bool show)
{
  if (show)
    {
      if (!mDirectory.isReady ())
        {
          qWarning () << "file " << mFilestr << " has no directory";
          show = false;
        }
    }

  auto sizes = mPaned.sizes ();
  if (show)
    {
      mDirIndex = {};
      if (sizes[0] == 0)
        {
          auto psize = mPaned.size ();
          sizes = { mDirectoryWidth, psize.width () - mPaned.handleWidth ()
                                         - DEFAULT_CONTENT_WIDTH };
          mPaned.setSizes (sizes);
        }
    }
  else
    {
      if (sizes[0] != 0)
        mDirectoryWidth = sizes[0];

      auto psize = mPaned.size ();
      sizes = { 0, psize.width () - mPaned.handleWidth () };
      mPaned.setSizes (sizes);
    }
}

void
ApvlvFrame::setActive (bool act)
{
  mActive = act;

  auto wid = mWidget->widget ();
  if (act)
    {
      QTimer::singleShot (50, wid, SLOT (setFocus ()));
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
      mView->setTitle (base.string ());
    }

  mStatus.setActive (act);
}

void
ApvlvFrame::setDirIndex (const string &path)
{
  mDirIndex = { "", 0, path, FileIndexType::DIR };
  mDirIndex.loadDirectory (path);
  emit indexGenerited (mDirIndex);
  toggleDirectory (true);
}

bool
ApvlvFrame::toggledControlDirectory (bool is_right)
{
  if (!isShowDirectory ())
    {
      return false;
    }

  if (auto controlled = isControlledDirectory (); !controlled && !is_right)
    {
      mDirectory.setActive (true);
      return true;
    }
  else if (controlled && is_right)
    {
      mDirectory.setActive (false);
      return true;
    }

  return false;
}

bool
ApvlvFrame::isShowDirectory ()
{
  auto sizes = mPaned.sizes ();
  return sizes[0] > 1;
}

bool
ApvlvFrame::isControlledDirectory ()
{
  if (!isShowDirectory ())
    return false;

  return mDirectory.isActive ();
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
  setLayout (&mLayout);
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

ApvlvToolStatus::ApvlvToolStatus (ApvlvFrame *frame) : mFrame (frame)
{
  auto paction = addAction (QIcon::fromTheme (QIcon::ThemeIcon::GoPrevious),
                            tr ("Previous Page"));
  QObject::connect (paction, SIGNAL (triggered (bool)), mFrame,
                    SLOT (previousPage ()));
  auto naction = addAction (QIcon::fromTheme (QIcon::ThemeIcon::GoNext),
                            tr ("Next Page"));
  QObject::connect (naction, SIGNAL (triggered (bool)), mFrame,
                    SLOT (nextPage ()));

  mPageValue.setSizePolicy (QSizePolicy::Fixed, QSizePolicy::Fixed);
  QObject::connect (&mPageValue, SIGNAL (editingFinished ()), this,
                    SLOT (gotoPage ()));
  addWidget (&mPageValue);
  addWidget (&mPageSum);
  addSeparator ();

  addWidget (&mScrollRate);
  addSeparator ();

  auto iaction = addAction (QIcon::fromTheme (QIcon::ThemeIcon::ZoomIn),
                            tr ("Zoom In"));
  QObject::connect (iaction, SIGNAL (triggered (bool)), mFrame,
                    SLOT (zoomIn ()));
  auto oaction = addAction (QIcon::fromTheme (QIcon::ThemeIcon::ZoomOut),
                            tr ("Zoom Out"));
  QObject::connect (oaction, SIGNAL (triggered (bool)), mFrame,
                    SLOT (zoomOut ()));

  addWidget (&mZoomType);
  for (auto const &str : ApvlvFrame::ZoomLabel)
    {
      mZoomType.addItem (ApvlvFrame::tr (str));
    }
  QObject::connect (&mZoomType, SIGNAL (currentIndexChanged (int)), mFrame,
                    SLOT (setZoomMode (int)));
  mZoomType.setLineEdit (&mZoomValue);
  addSeparator ();

#ifdef APVLV_WITH_OCR
  addWidget (&mOcrParse);
  mOcrParse.setText (tr ("OCR Parse"));
  QObject::connect (&mOcrParse, SIGNAL (checkStateChanged (Qt::CheckState)),
                    mFrame, SLOT (ocrParse ()));

  addAction (&mOcrCopy);
  mOcrCopy.setIcon (QIcon::fromTheme (QIcon::ThemeIcon::Scanner));
  mOcrCopy.setText (tr ("OCR Copy"));
  QObject::connect (&mOcrCopy, SIGNAL (triggered (bool)), mFrame,
                    SLOT (ocrCopy ()));
#endif
}

void
ApvlvToolStatus::updateValue (int pn, int totpn, double zm, double sr)
{
  mPageValue.setText (QString::number (pn));
  mPageSum.setText (QString::fromLocal8Bit ("/%1").arg (totpn));
  if (mZoomType.currentIndex () == 3)
    {
      mZoomValue.setText (
          QString::fromLocal8Bit ("%1%").arg (static_cast<int> (zm * 100)));
    }
  mScrollRate.setText (
      QString::fromLocal8Bit ("%1%").arg (static_cast<int> (sr * 100)));

#ifdef APVLV_WITH_OCR
  auto need_ocr = mFrame->mFile->pageIsOnlyImage (mFrame->pageNumber ());
  mOcrParse.setEnabled (need_ocr);
  mOcrCopy.setEnabled (need_ocr);
#endif
}

void
ApvlvToolStatus::gotoPage ()
{
  auto text = mPageValue.text ().trimmed ();
  auto pn = text.toInt ();
  if (pn != mFrame->pageNumber ())
    {
      mFrame->showPage (pn, 0.f);
    }
}

CmdReturn
ApvlvFrame::subProcess ([[maybe_unused]] int ct, uint key)
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
          zoomIn ();
        }
      else if (key == 'o')
        {
          zoomOut ();
        }
      else if (key == 'h')
        {
          setZoomMode (static_cast<int> (ZoomMode::FITHEIGHT));
        }
      else if (key == 'w')
        {
          setZoomMode (static_cast<int> (ZoomMode::FITWIDTH));
        }
      break;

    default:
      return CmdReturn::NO_MATCH;
      break;
    }

  return CmdReturn::MATCH;
}

void
ApvlvFrame::previousPage ()
{
  previousPage (1);
}

void
ApvlvFrame::nextPage ()
{
  nextPage (1);
}

void
ApvlvFrame::setZoomMode (int mode)
{
  if (mode < static_cast<int> (ZoomMode::CUSTOM))
    {
      switch (static_cast<ZoomMode> (mode))
        {
          using enum ZoomMode;
        case NORMAL:
          setZoomString ("normal");
          break;
        case FITWIDTH:
          setZoomString ("fitwidth");
          break;
        case FITHEIGHT:
          setZoomString ("fitheight");
          break;
        case CUSTOM:
          break;
        }
    }

  updateStatus ();
}

void
ApvlvFrame::zoomIn ()
{
  auto zoomrate = mWidget->zoomrate ();
  setZoomrate (zoomrate * 1.1);
  updateStatus ();
}

void
ApvlvFrame::zoomOut ()
{
  auto zoomrate = mWidget->zoomrate ();
  setZoomrate (zoomrate / 1.1);
  updateStatus ();
}

#ifdef APVLV_WITH_OCR
void
ApvlvFrame::ocrParse ()
{
  auto meta = mWidget->widget ()->metaObject ();
  qDebug () << "widget is " << meta->className ();
  if (!mWidget->widget ()->inherits ("apvlv::ApvlvImage"))
    return;

  auto image = dynamic_cast<ApvlvImage *> (mWidget->widget ());
  auto state = mToolStatus.mOcrParse.checkState ();
  image->ocrDisplay (state == Qt::Checked);
}

void
ApvlvFrame::ocrCopy ()
{
  auto meta = mWidget->widget ()->metaObject ();
  qDebug () << "widget is " << meta->className ();
  if (!mWidget->widget ()->inherits ("apvlv::ApvlvImage"))
    return;

  auto image = dynamic_cast<ApvlvImage *> (mWidget->widget ());
  auto text = image->ocrGetText ();
#ifdef QT_DEBUG
  QMessageBox::information (this, tr ("text in clipboard"),
                            QString::fromLocal8Bit (text.get ()));
#endif
  auto clipboard = QGuiApplication::clipboard ();
  clipboard->setText (text.get ());
}
#endif

void
ApvlvFrame::wheelEvent (QWheelEvent *event)
{
  auto angel = event->angleDelta ();
  if (angel.y () > 0)
    {
      mWidget->scrollUp (1);
      updateStatus ();
    }
  else
    {
      mWidget->scrollDown (1);
      updateStatus ();
    }
}

CmdReturn
ApvlvFrame::process (int has, int ct, uint key)
{
  emit focusIn ();

  if (mProCmd != 0)
    {
      return subProcess (ct, key);
    }

  if (!has)
    {
      ct = 1;
    }

  switch (key)
    {
    case Key_PageDown:
    case ctrlValue ('f'):
      nextPage (ct);
      break;
    case Key_PageUp:
    case ctrlValue ('b'):
      previousPage (ct);
      break;
    case ctrlValue ('d'):
      halfNextPage (ct);
      break;
    case ctrlValue ('u'):
      halfPreviousPage (ct);
      break;
    case ':':
    case '/':
    case '?':
    case 'F':
      if (isControlledDirectory ())
        {
          mDirectory.focusFilter ();
        }
      else
        {
          mView->promptCommand (char (key));
          return CmdReturn::NEED_MORE;
        }
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
    case ctrlValue ('p'):
    case Key_Up:
    case 'k':
      if (isControlledDirectory ())
        {
          mDirectory.scrollUp (ct);
        }
      else
        {
          mWidget->scrollUp (ct);
          updateStatus ();
        }
      break;
    case ctrlValue ('n'):
    case ctrlValue ('j'):
    case Key_Down:
    case 'j':
      if (isControlledDirectory ())
        {
          mDirectory.scrollDown (ct);
        }
      else
        {
          mWidget->scrollDown (ct);
          updateStatus ();
        }
      break;
    case Key_Backspace:
    case Key_Left:
    case ctrlValue ('h'):
    case 'h':
      if (isControlledDirectory ())
        {
          mDirectory.scrollLeft (ct);
        }
      else
        {
          mWidget->scrollLeft (ct);
          updateStatus ();
        }
      break;
    case Key_Space:
    case Key_Right:
    case ctrlValue ('l'):
    case 'l':
      if (isControlledDirectory ())
        {
          mDirectory.scrollRight (ct);
        }
      else
        {
          mWidget->scrollRight (ct);
          updateStatus ();
        }
      break;
    case Key_Return:
      directoryShowPage (mDirectory.currentItemFileIndex (), false);
      break;
    case 'R':
      reload ();
      break;
    case ctrlValue (']'):
      gotoLink (ct);
      break;
    case ctrlValue ('t'):
      returnLink (ct);
      break;
    case 't':
      mView->newTab (HelpPdf);
      mView->open ();
      break;
    case 'T':
      mView->newTab (HelpPdf);
      mView->openDir ();
      break;
    case 'o':
      mView->open ();
      break;
    case 'O':
      mView->openDir ();
      break;
    case 'r':
      rotate (ct);
      break;
    case 'G':
      markposition ('\'');
      if (!has)
        {
          showPage (mFile->sum () - 1, 0.0);
        }
      else
        {
          showPage (ct - 1, 0.0);
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
      setSkip (ct);
      break;
    case 'c':
      toggleDirectory ();
      break;
    default:
      return CmdReturn::NO_MATCH;
      break;
    }

  return CmdReturn::MATCH;
}

ApvlvFrame *
ApvlvFrame::clone ()
{
  auto *ndoc = new ApvlvFrame (mView);
  ndoc->loadFile (mFilestr, false, false);
  ndoc->showPage (mWidget->pageNumber (), mWidget->scrollRate ());
  return ndoc;
}

void
ApvlvFrame::setZoomrate (double zm)
{
  mZoomMode = ZoomMode::CUSTOM;
  mWidget->setZoomrate (zm);
}

void
ApvlvFrame::setZoomString (const char *z)
{
  auto zoomrate = mWidget->zoomrate ();
  if (z != nullptr)
    {
      string_view sv (z);
      if (sv == "normal")
        {
          mZoomMode = ZoomMode::NORMAL;
          zoomrate = 1.2;
        }
      else if (sv == "fitwidth")
        {
          mZoomMode = ZoomMode::FITWIDTH;
        }
      else if (sv == "fitheight")
        {
          mZoomMode = ZoomMode::FITHEIGHT;
        }
      else
        {
          double d = strtod (z, nullptr);
          if (d > 0)
            {
              mZoomMode = ZoomMode::CUSTOM;
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
          if (mZoomMode == ZoomMode::FITWIDTH)
            {
              auto x_root = wid->width ();
              zoomrate = x_root / size.width;
            }
          else if (mZoomMode == ZoomMode::FITHEIGHT)
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
  if (filename.empty () || HelpPdf == filename
      || ApvlvParams::instance ()->getBoolOrDefault ("noinfo", false))
    {
      return false;
    }

  bool ret = ApvlvInfo::instance ()->updateFile (
      mWidget->pageNumber (), mSkip, mWidget->scrollRate (), filename);
  return ret;
}

bool
ApvlvFrame::loadLastPosition (const string &filename)
{
  if (filename.empty () || HelpPdf == filename
      || ApvlvParams::instance ()->getBoolOrDefault ("noinfo"))
    {
      showPage (0, 0.0);
      return false;
    }

  bool ret = false;

  auto optfp = ApvlvInfo::instance ()->file (filename);
  if (optfp)
    {
      // correctly check
      showPage (optfp.value ()->page, 0.0);
      setSkip (optfp.value ()->skip);
    }
  else
    {
      showPage (0, 0.0);
      ApvlvInfo::instance ()->updateFile (0, 0.0, mWidget->zoomrate (),
                                          filename);
    }

  return ret;
}

bool
ApvlvFrame::reload ()
{
  return loadFile (mFilestr, false, isShowDirectory ());
}

int
ApvlvFrame::pageNumber ()
{
  return mWidget ? mWidget->pageNumber () : 0;
}

bool
ApvlvFrame::loadFile (const std::string &file, bool check, bool show_directory)
{
  if (check && file == mFilestr)
    {
      return false;
    }

  saveLastPosition (mFilestr);

  mFile = FileFactory::loadFile (file);

  if (mFile)
    {
      emit indexGenerited (mFile->getIndex ());

      mFilestr = file;

      if (mFile->sum () <= 1)
        {
          qDebug () << "sum () = " << mFile->sum ();
        }

      setWidget (mFile->getDisplayType ());

      loadLastPosition (file);

      setActive (true);

      mSearchStr = "";
      mSearchResults = nullptr;

      if (ApvlvParams::instance ()->getIntOrDefault ("autoreload") > 0)
        {
          mWatcher = make_unique<QFileSystemWatcher> ();
          QObject::connect (mWatcher.get (), SIGNAL (fileChanged ()), this,
                            SLOT (changed_cb ()));

          auto systempath = filesystem::path (file);
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
              mWatcher->addPath (QString::fromLocal8Bit (file));
            }
        }
    }

  if (show_directory && mFile != nullptr)
    {
      toggleDirectory (true);
    }
  else
    {
      toggleDirectory (false);
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
      showPage (adp.pagenum, adp.scrollrate);
    }
}

void
ApvlvFrame::showPage (int pn, double s)
{
  auto rp = mFile->pageNumberWrap (pn);
  if (rp < 0)
    return;

  mWidget->setAnchor ("");

  if (!mZoominit)
    {
      mZoominit = true;
      setZoomString (nullptr);
    }

  refresh (rp, s);
}

void
ApvlvFrame::showPage (int pn, const std::string &anchor)
{
  auto rp = mFile->pageNumberWrap (pn);
  if (rp < 0)
    return;
  if (!mZoominit)
    {
      mZoominit = true;
      setZoomString (nullptr);
    }

  mWidget->showPage (rp, anchor);
  updateStatus ();
}

void
ApvlvFrame::nextPage (int times)
{
  showPage (mWidget->pageNumber () + times, 0.0f);
}

void
ApvlvFrame::previousPage (int times)
{
  showPage (mWidget->pageNumber () - times, 0.0f);
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
ApvlvFrame::halfNextPage (int times)
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

  showPage (mWidget->pageNumber () + rtimes, sr);
}

void
ApvlvFrame::halfPreviousPage (int times)
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

  showPage (mWidget->pageNumber () - rtimes, sr);
}

bool
ApvlvFrame::needSearch (const std::string &str, bool reverse)
{
  if (mFile == nullptr)
    return false;

  // search a different string
  if (!str.empty () && str != mSearchStr)
    {
      qDebug () << "different string.";
      mSearchStr = str;
      return true;
    }

  else if (mSearchResults == nullptr)
    {
      qDebug () << "no result.";
      return true;
    }

  // same string, but need to search next page
  else if ((!reverse
            && mWidget->searchSelect () == (int)mSearchResults->size () - 1)
           || (reverse && mWidget->searchSelect () == 0))
    {
      qDebug () << "same, but need next string: s: " << reverse
                << ", sel: " << mWidget->searchSelect ()
                << ", max: " << mSearchResults->size ();
      return true;
    }

  // same string, not need search, but has zoomed
  else
    {
      qDebug () << "same, not need next string. sel: "
                << mWidget->searchSelect ()
                << ", max: " << mSearchResults->size ();
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

  if (!needSearch (str, reverse))
    {
      return true;
    }

  mSearchResults = nullptr;
  unsetHighlight ();

  auto wrap = ApvlvParams::instance ()->getBoolOrDefault ("wrapscan");

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
                showPage (i, 0.0);
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
ApvlvFrame::gotoLink ([[maybe_unused]] int ct)
{
  // need impl
}

void
ApvlvFrame::returnLink ([[maybe_unused]] int ct)
{
  // need impl
}

void
ApvlvFrame::directoryShowPage (const FileIndex *index, bool force)
{
  if (index == nullptr)
    return;

  if (index->type == FileIndexType::FILE)
    {
      loadFile (index->path, true, true);
      return;
    }

  auto file = mDirectory.currentFileFileIndex ();
  if (file && file->path != mFilestr)
    loadFile (file->path, true, true);

  if (index->type == FileIndexType::PAGE)
    {
      if (index->page != mWidget->pageNumber ()
          || index->anchor != mWidget->anchor ())
        showPage (index->page, index->anchor);
    }
}

void
ApvlvFrame::setWidget (DISPLAY_TYPE type)
{
  auto sizes = mPaned.sizes ();

  if (type == DISPLAY_TYPE::IMAGE)
    {
      mWidget = make_unique<ImageWidget> ();
      mWidget->setFile (mFile.get ());
#ifdef APVLV_WITH_OCR
      ocrParse ();
#endif
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
  mTextLayout.addWidget (mWidget->widget (), 1);

  mPaned.setSizes (sizes);
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

      mStatus.showMessages (labels);

      mToolStatus.updateValue (pn, totpn, zm, sr);

      mDirectory.setCurrentIndex (mFilestr, mWidget->pageNumber (),
                                  mWidget->anchor ());
    }
}

bool
ApvlvFrame::isStatusHidden ()
{
  return mStatus.isHidden ();
}

void
ApvlvFrame::statusShow ()
{
  mStatus.show ();
}

void
ApvlvFrame::statusHide ()
{
  mStatus.hide ();
}

}

// Local Variables:
// mode: c++
// End:

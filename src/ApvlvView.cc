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
/* @CPPFILE ApvlvView.cc
 *
 *  Author: Alf <naihe2010@126.com>
 */

#include <QApplication>
#include <QFileDialog>
#include <QInputDialog>
#include <QKeyEvent>
#include <QLineEdit>
#include <QMenu>
#include <QMessageBox>
#include <QToolButton>
#include <QVBoxLayout>
#include <filesystem>
#include <sstream>

#include "ApvlvDired.h"
#include "ApvlvInfo.h"
#include "ApvlvParams.h"
#include "ApvlvSearchDialog.h"
#include "ApvlvView.h"

namespace apvlv
{
using namespace std;
using namespace CommandModeType;

static bool
isAltEscape (QKeyEvent *event)
{
  if (event->key () == Qt::Key_BracketLeft &&
      /* ...so we can ignore mod keys like numlock and capslock. */
      (event->modifiers () & Qt::ControlModifier))
    {
      return true;
    }

  return false;
}

void
ApvlvCommandBar::keyPressEvent (QKeyEvent *evt)
{
  if (evt->key () == Qt::Key_Escape || isAltEscape (evt)
      || evt->key () == Qt::Key_Tab || evt->key () == Qt::Key_Up
      || evt->key () == Qt::Key_Down)
    {
      emit keyPressed (evt);
      return;
    }

  QLineEdit::keyPressEvent (evt);
}

bool
ApvlvCommandBar::eventFilter (QObject *obj, QEvent *event)
{
  if (event->type () == QEvent::KeyPress
      && dynamic_cast<QKeyEvent *> (event)->key () == Qt::Key_Tab)
    {
      qDebug () << "Ate key press tab";
      emit keyPressed (dynamic_cast<QKeyEvent *> (event));
      return true;
    }
  else
    {
      return QObject::eventFilter (obj, event);
    }
}

ApvlvView::ApvlvView (ApvlvView *parent) : mCmds (this)
{
  mParent = parent;
  if (mParent)
    {
      mParent->appendChild (this);
    }

  auto guiopt
      = ApvlvParams::instance ()->getStringOrDefault ("guioptions", "mTS");

  mCmdType = CmdStatusType::CMD_NONE;

  mProCmd = 0;
  mProCmdCnt = 0;

  mCurrHistroy = -1;

  mHasFull = false;

  keyLastEnd = true;

  processInLast = false;

  int w = ApvlvParams::instance ()->getIntOrDefault ("width");
  int h = ApvlvParams::instance ()->getIntOrDefault ("height");

  if (ApvlvParams::instance ()->getBoolOrDefault ("fullscreen"))
    {
      fullScreen ();
    }
  else
    {
      resize (w > 1 ? w : 800, h > 1 ? h : 600);
    }
  show ();

  setCentralWidget (&mCentral);

  setupMenuBar (guiopt);
  setMenuBar (&mMenuBar);
  setupToolBar ();
  addToolBar (Qt::TopToolBarArea, &mToolBar);

  bool isMenuBarShow = guiopt.find ('m') != string::npos;
  if (!isMenuBarShow)
    {
      mMenuBar.hide ();
    }
  bool isToolBarShow = guiopt.find ('T') != string::npos;
  if (!isToolBarShow)
    {
      mToolBar.hide ();
    }

  mCentral.setLayout (&mVBoxLayout);

  mTabContainer.setTabBarAutoHide (true);
  mVBoxLayout.addWidget (&mTabContainer, 1);

  QObject::connect (&mTabContainer, SIGNAL (currentChanged (int)), this,
                    SLOT (tabSwitched (int)));

  mVBoxLayout.addWidget (&mCommandBar, 0);
  QObject::connect (&mCommandBar, SIGNAL (textEdited (const QString &)), this,
                    SLOT (commandbarEdited (const QString &)));
  QObject::connect (&mCommandBar, SIGNAL (returnPressed ()), this,
                    SLOT (commandbarReturn ()));
  QObject::connect (&mCommandBar, SIGNAL (keyPressed (QKeyEvent *)), this,
                    SLOT (commandbarKeyPressed (QKeyEvent *)));

  cmdHide ();
}

ApvlvView::~ApvlvView ()
{
  if (mParent)
    {
      mParent->eraseChild (this);
    }

  auto itr = mChildren.begin ();
  while (itr != mChildren.end ())
    {
      delete *itr;
      itr = mChildren.begin ();
    }

  mCmdHistroy.clear ();
}

ApvlvWindow *
ApvlvView::currentWindow ()
{
  auto index = mTabContainer.currentIndex ();
  if (index < 0)
    return nullptr;

  auto root_win = dynamic_cast<ApvlvWindow *> (mTabContainer.widget (index));
  Q_ASSERT (root_win);

  auto widget = QApplication::focusWidget ();
  if (widget)
    {
      auto win = root_win->findWindowByWidget (widget);
      if (win)
        {
          root_win->setActiveWindow (win);
          return win;
        }
    }

  auto win = root_win->getActiveWindow ();
  if (win)
    return win;

  return root_win->firstFrameWindow ();
}

void
ApvlvView::delCurrentWindow ()
{
  if (auto win = currentWindow (); win)
    win->perish ();
  updateTabName ();
}

void
ApvlvView::open ()
{
  QString dirname;
  auto fp = ApvlvInfo::instance ()->file (0);
  if (fp)
    {
      dirname = QString::fromLocal8Bit (
          filesystem::path (fp.value ()->file).parent_path ().string ());
    }
  else
    {
      dirname = QDir::homePath ();
    }

  // qDebug ("lastfile: [%s], dirname: [%s]", fp ? fp->file.c_str () : "",
  // dirname);
  auto const mimes = FileFactory::supportMimeTypes ();
  QString filters;
  for (const auto &m : mimes)
    {
      filters += m.first.c_str ();
      filters += "(";
      filters += ('*' + m.second[0]);
      for (decltype (m.second.size ()) index = 1; index < m.second.size ();
           ++index)
        filters += (" *" + m.second[index]);
      filters += ");;";
    }

  QString selected;
  auto filename = QFileDialog::getOpenFileName (this, "open file", dirname,
                                                filters, &selected);
  if (!filename.isEmpty ())
    {
      loadFile (string (filename.toLocal8Bit ().constData ()));
    }
}

void
ApvlvView::openDir ()
{
  QString dirname;
  auto fp = ApvlvInfo::instance ()->file (0);
  if (fp)
    {
      dirname = QString::fromLocal8Bit (
          filesystem::path (fp.value ()->file).parent_path ().string ());
    }
  else
    {
      dirname = QDir::homePath ();
    }

  auto filename = QFileDialog::getExistingDirectory (
      this, "open dir", dirname, QFileDialog::ShowDirsOnly);
  if (!filename.isEmpty ())
    {
      loadDir (filename.toStdString ());
    }
}

void
ApvlvView::openRrl ()
{
  auto res = QInputDialog::getText (this, "url", tr ("input url: "));
  if (!res.isEmpty ())
    {
      currentFrame ()->loadUri (res.toStdString ());
    }
}

bool
ApvlvView::loadDir (const std::string &path)
{
  currentFrame ()->setDirIndex (path);
  return true;
}

void
ApvlvView::quit (bool only_tab)
{
  if (mTabContainer.count () <= 1 || only_tab == false)
    {
      closeEvent (nullptr);
      return;
    }

  auto index = mTabContainer.currentIndex ();
  Q_ASSERT (index != -1);
  mTabContainer.removeTab (index);
}

void
ApvlvView::search ()
{
  promptCommand ('/');
}

void
ApvlvView::backSearch ()
{
  promptCommand ('?');
}

void
ApvlvView::advancedSearch ()
{
  auto diag = SearchDialog (this);
  QObject::connect (&diag, SIGNAL (loadFile (const string &, int)), this,
                    SLOT (loadFileOnPage (const string &, int)));
  diag.exec ();
}

void
ApvlvView::dired ()
{
  auto diag = DiredDialog (this);
  QObject::connect (&diag, SIGNAL (loadFile (const string &, int)), this,
                    SLOT (loadFileOnPage (const string &, int)));
  diag.exec ();
}

bool
ApvlvView::newTab (const std::string &filename)
{
  auto docname = filename;
  if (filesystem::is_directory (filename))
    {
      docname = helppdf;
    }

  auto optndoc = hasLoaded (docname);
  if (!optndoc)
    {
      auto ndoc = new ApvlvFrame (this);
      if (!ndoc->loadfile (docname, true, true))
        {
          delete ndoc;
          ndoc = nullptr;
        }

      if (ndoc)
        {
          regLoaded (ndoc);
          optndoc = make_optional<ApvlvFrame *> (ndoc);
        }
    }

  if (optndoc)
    {
      newTab (optndoc.value ());
      if (filesystem::is_directory (filename))
        {
          loadDir (filename);
        }
      return true;
    }
  else
    {
      return false;
    }
}

bool
ApvlvView::newTab (ApvlvFrame *core)
{
  auto win = new ApvlvWindow ();
  win->setFrame (core);

  auto basename
      = core->filename ()
            ? filesystem::path (core->filename ()).filename ().string ()
            : "NONE";
  auto pos = mTabContainer.currentIndex () + 1;
  mTabContainer.insertTab (pos, win, QString::fromLocal8Bit (basename));
  mTabContainer.setCurrentIndex (pos);

  return true;
}

bool
ApvlvView::loadFile (const std::string &filename)
{
  auto abpath = filesystem::absolute (filename).string ();

  ApvlvWindow *win = currentWindow ();
  ApvlvFrame *ndoc = nullptr;

  auto optndoc = hasLoaded (abpath);
  if (!optndoc)
    {
      ndoc = new ApvlvFrame (this);
      if (!ndoc->loadfile (filename, true, true))
        {
          delete ndoc;
          ndoc = nullptr;
        }
      else
        {
          regLoaded (ndoc);
          optndoc = make_optional<ApvlvFrame *> (ndoc);
        }
    }

  if (optndoc)
    {
      win->setFrame (optndoc.value ());
      updateTabName ();
    }

  return ndoc != nullptr;
}

void
ApvlvView::loadFileOnPage (const string &filename, int pn)
{
  auto cdoc = currentFrame ();
  if (cdoc)
    {
      if (loadFile (filename))
        {
          cdoc->showpage (pn, 0.0);
        }
    }
}

optional<ApvlvFrame *>
ApvlvView::hasLoaded (string_view abpath)
{
  for (auto &core : mDocs)
    {
      if (!core->inuse () && abpath == core->filename ())
        {
          return make_optional<ApvlvFrame *> (core.get ());
        }
    }

  return nullopt;
}

void
ApvlvView::regLoaded (ApvlvFrame *doc)
{
  auto cache_count
      = ApvlvParams::instance ()->getIntOrDefault ("cache_count", 10);
  if (mDocs.size () >= static_cast<size_t> (cache_count))
    {
      auto found_itr = mDocs.end ();
      for (auto itr = mDocs.begin (); itr != mDocs.end (); ++itr)
        {
          if ((*itr)->inuse () == false)
            {
              found_itr = itr;
              break;
            }
        }

      if (found_itr != mDocs.end ())
        {
          mDocs.erase (found_itr);
        }
    }

  mDocs.push_back (unique_ptr<ApvlvFrame> (doc));
}

void
ApvlvView::setupMenuBar (const string &guiopt)
{
  // File -> open, openDir
  auto mfile = new QMenu (tr ("File"));
  mMenuBar.addMenu (mfile);

  auto action = mfile->addAction (tr ("Open"));
  QObject::connect (action, SIGNAL (triggered (bool)), this, SLOT (open ()));
  action = mfile->addAction (tr ("OpenDir"));
  QObject::connect (action, SIGNAL (triggered (bool)), this,
                    SLOT (openDir ()));
  action = mfile->addAction (tr ("OpenUrl"));
  QObject::connect (action, SIGNAL (triggered (bool)), this,
                    SLOT (openRrl ()));
  action = mfile->addAction (tr ("New Tab"));
  QObject::connect (action, SIGNAL (triggered (bool)), this, SLOT (newTab ()));
  action = mfile->addAction (tr ("Close Tab"));
  QObject::connect (action, SIGNAL (triggered (bool)), this,
                    SLOT (closeTab ()));
  action = mfile->addAction (tr ("Quit"));
  QObject::connect (action, SIGNAL (triggered (bool)), this,
                    SLOT (quit (bool)));

  // Edit ->
  auto medit = new QMenu (tr ("Edit"));
  mMenuBar.addMenu (medit);
  action = medit->addAction (tr ("Search"));
  QObject::connect (action, SIGNAL (triggered (bool)), this, SLOT (search ()));
  action = medit->addAction (tr ("Back Search"));
  QObject::connect (action, SIGNAL (triggered (bool)), this,
                    SLOT (backSearch ()));

  // View ->
  auto mview = new QMenu (tr ("View"));
  mMenuBar.addMenu (mview);

  action = mview->addAction (tr ("ToolBar"));
  action->setCheckable (true);
  QObject::connect (action, SIGNAL (triggered (bool)), this,
                    SLOT (toggleToolBar ()));
  bool isToolBarShow = guiopt.find ('T') != string::npos;
  action->setChecked (isToolBarShow);

  action = mview->addAction (tr ("StatusBar"));
  action->setCheckable (true);
  QObject::connect (action, SIGNAL (triggered (bool)), this,
                    SLOT (toggleStatus ()));
  bool isStatusShow = guiopt.find ('S') != string::npos;
  action->setChecked (isStatusShow);

  action = mview->addAction (tr ("Horizontal Split"));
  QObject::connect (action, SIGNAL (triggered (bool)), this,
                    SLOT (horizontalSplit ()));
  action = mview->addAction (tr ("Vertical Split"));
  QObject::connect (action, SIGNAL (triggered (bool)), this,
                    SLOT (verticalSplit ()));
  action = mview->addAction (tr ("Close Split"));
  QObject::connect (action, SIGNAL (triggered (bool)), this,
                    SLOT (unBirth ()));

  // Navigate ->
  auto mnavigate = new QMenu (tr ("Navigate"));
  mMenuBar.addMenu (mnavigate);
  action = mnavigate->addAction (tr ("Previous Page"));
  QObject::connect (action, SIGNAL (triggered (bool)), this,
                    SLOT (previousPage ()));
  action = mnavigate->addAction (tr ("Next Page"));
  QObject::connect (action, SIGNAL (triggered (bool)), this,
                    SLOT (nextPage ()));

  // Tools
  auto mtools = new QMenu (tr ("Tools"));
  mMenuBar.addMenu (mtools);
  action = mtools->addAction (tr ("Advanced Search"));
  QObject::connect (action, SIGNAL (triggered (bool)), this,
                    SLOT (advancedSearch ()));
  action = mtools->addAction (tr ("Dired"));
  QObject::connect (action, SIGNAL (trigged (bool)), this, SLOT (dired ()));

  // Help -> about
  auto mhelp = new QMenu (tr ("Help"));
  mMenuBar.addMenu (mhelp);
}

void
ApvlvView::setupToolBar ()
{
  auto action = mToolBar.addAction (tr ("Open"));
  action->setIcon (
      QApplication::style ()->standardIcon (QStyle::SP_DialogOpenButton));
  QObject::connect (action, SIGNAL (triggered (bool)), this, SLOT (open ()));

  action = mToolBar.addAction (tr ("OpenDir"));
  action->setIcon (
      QApplication::style ()->standardIcon (QStyle::SP_DirOpenIcon));
  QObject::connect (action, SIGNAL (triggered (bool)), this,
                    SLOT (openDir ()));

  action = mToolBar.addAction (tr ("Toggle Content"));
  QObject::connect (action, SIGNAL (triggered (bool)), this,
                    SLOT (toggleContent ()));

  action = mToolBar.addAction (tr ("Quit"));
  QObject::connect (action, SIGNAL (triggered (bool)), this,
                    SLOT (quit (bool)));
}

void
ApvlvView::promptCommand (char ch)
{
  QString s{ ch };
  mCommandBar.setText (s);
  cmdShow (CmdStatusType::CMD_CMD);
}

void
ApvlvView::promptCommand (const char *str)
{
  mCommandBar.setText (str);
  cmdShow (CmdStatusType::CMD_CMD);
}

char *
ApvlvView::input (const char *str, int width, int height,
                  const string &content)
{
  // need impl
  return nullptr;
}

void
ApvlvView::cmdShow (CmdStatusType cmdtype)
{
  mCmdType = cmdtype;

  mCommandBar.show ();
  mCommandBar.setCursorPosition (1);
  mCommandBar.setFocus ();
}

void
ApvlvView::cmdHide ()
{
  mCmdType = CmdStatusType::CMD_NONE;
  mCmdTime = chrono::steady_clock::now ();

  mCommandBar.hide ();
  mTabContainer.setFocus ();
}

void
ApvlvView::cmdAuto (const char *ps)
{
  stringstream ss (ps);
  string cmd;
  string np;
  string argu;

  ss >> cmd >> np >> argu;

  if (np.empty ())
    {
      return;
    }

  if (np[np.length () - 1] == '\\')
    {
      np.replace (np.length () - 1, 1, 1, ' ');
      np += argu;
      ss >> argu;
    }

  if (cmd.empty () || np.empty ())
    {
      return;
    }

  auto comp = ApvlvCompletion{};
  if (cmd == "o" || cmd == "open" || cmd == "TOtext")
    {
      comp.addPath (np);
    }
  else if (cmd == "doc")
    {
      vector<string> items;
      for (auto &doc : mDocs)
        {
          items.emplace_back (doc->filename ());
        }
      comp.addItems (items);
    }

  qDebug () << "find match: " << np;
  auto comtext = comp.complete (np);
  if (!comtext.empty ())
    {
      qDebug () << "get a match: " << comtext;
      QString s = QString::fromLocal8Bit (comtext);
      s.replace (" ", "\\ ");
      QString linetext = QString::asprintf (":%s %s", cmd.c_str (),
                                            s.toStdString ().c_str ());
      mCommandBar.setText (linetext);
    }
  else
    {
      qDebug () << "no get match";
    }
}

void
ApvlvView::fullScreen ()
{
  if (mHasFull == false)
    {
      ApvlvFrame *core = currentFrame ();
      if (core)
        core->toggleContent (false);

      showFullScreen ();
      mHasFull = true;
    }
  else
    {
      showNormal ();
      mHasFull = false;
    }
}

void
ApvlvView::nextPage ()
{
  currentFrame ()->nextpage (1);
}

void
ApvlvView::previousPage ()
{
  currentFrame ()->prepage (1);
}

void
ApvlvView::toggleContent ()
{
  currentFrame ()->toggleContent ();
}

void
ApvlvView::toggleToolBar ()
{
  if (mToolBar.isHidden ())
    {
      mToolBar.show ();
    }
  else
    {
      mToolBar.hide ();
    }
}

void
ApvlvView::toggleStatus ()
{
  for (auto &doc : mDocs)
    {
      if (doc->isStatusHidden ())
        {
          doc->statusShow ();
        }
      else
        {
          doc->statusHide ();
        }
    }
}

void
ApvlvView::newTab ()
{
  auto doc = currentFrame ()->clone ();
  newTab (doc);
}

void
ApvlvView::closeTab ()
{
  quit (true);
}

void
ApvlvView::horizontalSplit ()
{
  currentWindow ()->birth (ApvlvWindow::WindowType::SP, nullptr);
}

void
ApvlvView::verticalSplit ()
{
  currentWindow ()->birth (ApvlvWindow::WindowType::VSP, nullptr);
}

void
ApvlvView::unBirth ()
{
  currentWindow ()->perish ();
}

ApvlvFrame *
ApvlvView::currentFrame ()
{
  if (auto widget = QApplication::focusWidget (); widget)
    {
      if (auto doc = ApvlvFrame::findByWidget (widget); doc)
        return doc;
    }

  ApvlvWindow *curwin = currentWindow ();
  Q_ASSERT (curwin);
  return curwin->getFrame ();
}

CmdReturn
ApvlvView::subProcess (int times, uint keyval)
{
  uint procmd = mProCmd;
  mProCmd = 0;
  mProCmdCnt = 0;
  switch (procmd)
    {
    case 'Z':
      if (keyval == 'Z')
        quit (true);

    case ctrlValue ('w'):
      if (keyval == 'q' || keyval == ctrlValue ('Q'))
        {
          if (currentWindow ()->isRoot ())
            {
              quit (true);
            }
          else
            {
              delCurrentWindow ();
            }
        }
      else
        {
          CmdReturn rv = currentWindow ()->process (times, keyval);
          updateTabName ();
          return rv;
        }
      break;

    case 'g':
      if (keyval == 't')
        {
          if (times == 0)
            switchTab (mTabContainer.currentIndex () + 1);
          else
            switchTab (times - 1);
        }
      else if (keyval == 'T')
        {
          if (times == 0)
            switchTab (mTabContainer.currentIndex () - 1);
          else
            switchTab (times - 1);
        }
      else if (keyval == 'g')
        {
          if (times == 0)
            times = 1;
          currentFrame ()->showpage (times - 1, 0.0);
        }
      break;

    default:
      return CmdReturn::NO_MATCH;
    }

  return CmdReturn::MATCH;
}

CmdReturn
ApvlvView::process (int has, int ct, uint key)
{
  if (mProCmd != 0)
    {
      auto ret = subProcess (mProCmdCnt, key);
      if (ret == CmdReturn::MATCH)
        {
          saveKey (has, ct, key, true);
        }
      return ret;
    }

  switch (key)
    {
    case 'Z':
      mProCmd = 'Z';
      return CmdReturn::NEED_MORE;

    case ctrlValue ('w'):
      mProCmd = ctrlValue ('w');
      mProCmdCnt = has ? ct : 1;
      saveKey (has, ct, key, false);
      return CmdReturn::NEED_MORE;
    case 'q':
      quit (true);
      break;
    case 'f':
      fullScreen ();
      break;
    case '.':
      processLastKey ();
      break;
    case 'g':
      mProCmd = 'g';
      mProCmdCnt = has ? ct : 0;
      saveKey (has, ct, key, false);
      return CmdReturn::NEED_MORE;
    default:
      CmdReturn ret = currentFrame ()->process (has, ct, key);
      if (ret == CmdReturn::NEED_MORE)
        {
          saveKey (has, ct, key, false);
        }
      else if (ret == CmdReturn::MATCH)
        {
          saveKey (has, ct, key, true);
        }
    }

  return CmdReturn::MATCH;
}

bool
ApvlvView::run (const char *str)
{
  bool ret;

  switch (*str)
    {
    case SEARCH:
      currentFrame ()->markposition ('\'');
      ret = currentFrame ()->search (str + 1, false);
      break;

    case BACKSEARCH:
      currentFrame ()->markposition ('\'');
      ret = currentFrame ()->search (str + 1, true);
      break;

    case COMMANDMODE:
      ret = runCommand (str + 1);
      break;

    case FIND:
      ret = currentFrame ()->search (str + 1, false);
      break;

    default:
      ret = false;
      break;
    }

  return ret;
}

void
ApvlvView::setTitle (const std::string &title)
{
  setWindowTitle (QString::fromLocal8Bit (title));
}

bool
ApvlvView::runCommand (const char *str)
{
  bool ret = true;

  if (*str == '!')
    {
      system (str + 1);
    }
  else
    {
      stringstream ss (str);
      string cmd;
      string subcmd;
      string argu;
      ss >> cmd >> subcmd >> argu;

      if (!subcmd.empty () && subcmd[subcmd.length () - 1] == '\\')
        {
          subcmd.replace (subcmd.length () - 1, 1, 1, ' ');
          subcmd += argu;
          ss >> argu;
        }

      if (cmd == "set")
        {
          if (subcmd == "skip")
            {
              currentFrame ()->setskip (
                  int (strtol (argu.c_str (), nullptr, 10)));
            }
          else
            {
              ApvlvParams::instance ()->push (subcmd, argu);
            }
        }
      else if (cmd == "map" && !subcmd.empty ())
        {
          ApvlvCmds::buildCommandMap (subcmd, argu);
        }
      else if ((cmd == "o" || cmd == "open" || cmd == "doc")
               && !subcmd.empty ())
        {
          char *home;

          if (subcmd[0] == '~')
            {
              home = getenv ("HOME");

              if (home)
                {
                  subcmd.replace (0, 1, home);
                }
            }

          if (filesystem::is_directory (subcmd))
            {
              ret = loadDir (subcmd);
            }
          else if (filesystem::is_regular_file (subcmd))
            {
              ret = loadFile (subcmd);
            }
          else
            {
              errorMessage (string ("no file: "), subcmd);
              ret = false;
            }
        }
      else if (cmd == "TOtext" && !subcmd.empty ())
        {
          currentFrame ()->totext (subcmd.c_str ());
        }
      else if ((cmd == "pr" || cmd == "print"))
        {
          currentFrame ()->print (
              subcmd.empty () ? -1
                              : int (strtol (subcmd.c_str (), nullptr, 10)));
        }
      else if (cmd == "sp")
        {
          if (currentWindow ()->birth (ApvlvWindow::WindowType::SP, nullptr))
            {
              windowAdded ();
            }
        }
      else if (cmd == "vsp")
        {
          if (currentWindow ()->birth (ApvlvWindow::WindowType::VSP, nullptr))
            {
              windowAdded ();
            }
        }
      else if ((cmd == "zoom" || cmd == "z") && !subcmd.empty ())
        {
          currentFrame ()->setZoomString (subcmd.c_str ());
        }
      else if (cmd == "forwardpage" || cmd == "fp")
        {
          if (subcmd.empty ())
            currentFrame ()->nextpage (1);
          else
            currentFrame ()->nextpage (
                int (strtol (subcmd.c_str (), nullptr, 10)));
        }
      else if (cmd == "prewardpage" || cmd == "bp")
        {
          if (subcmd.empty ())
            currentFrame ()->prepage (1);
          else
            currentFrame ()->prepage (
                int (strtol (subcmd.c_str (), nullptr, 10)));
        }
      else if (cmd == "content")
        {
          currentFrame ()->toggleContent ();
        }
      else if (cmd == "goto" || cmd == "g")
        {
          currentFrame ()->markposition ('\'');
          auto p = strtol (subcmd.c_str (), nullptr, 10);
          p += currentFrame ()->getskip ();
          currentFrame ()->showpage (int (p - 1), 0.0);
        }
      else if (cmd == "help" || cmd == "h")
        {
          loadFile (helppdf);
        }
      else if (cmd == "q" || cmd == "quit")
        {
          if (currentWindow ()->isRoot ())
            {
              quit (true);
            }
          else
            {
              delCurrentWindow ();
            }
        }
      else if (cmd == "qall")
        {
          quit (false);
        }
      else if (cmd == "tabnew")
        {
          newTab (helppdf);
        }
      else if (cmd == "tabn" || cmd == "tabnext")
        {
          switchTab (mTabContainer.currentIndex () + 1);
        }
      else if (cmd == "tabp" || cmd == "tabprevious")
        {
          switchTab (mTabContainer.currentIndex () - 1);
        }
      else if (cmd == "toc")
        {
          loadFile (currentFrame ()->filename ());
        }
      else
        {
          bool isn = true;
          for (char i : cmd)
            {
              if (i < '0' || i > '9')
                {
                  isn = false;
                  break;
                }
            }
          if (isn && currentFrame ())
            {
              auto p = strtol (cmd.c_str (), nullptr, 10);
              p += currentFrame ()->getskip ();
              if (p != currentFrame ()->pageNumber ())
                {
                  currentFrame ()->showpage (int (p - 1), 0.0);
                }
            }
          else
            {
              errorMessage (string ("no command: "), cmd);
              ret = false;
            }
        }
    }

  return ret;
}

void
ApvlvView::keyPressEvent (QKeyEvent *evt)
{
  if (mCmdType != CmdStatusType::CMD_NONE)
    {
      return;
    }

  auto now = chrono::steady_clock::now ();
  auto millis
      = chrono::duration_cast<chrono::milliseconds> (now - mCmdTime).count ();
  if (millis < 1000L)
    return;

  mCmds.append (evt);
}

void
ApvlvView::commandbarEdited ([[maybe_unused]] const QString &str)
{
  // need impl
}

void
ApvlvView::commandbarReturn ()
{
  if (mCmdType == CmdStatusType::CMD_CMD)
    {
      auto str = mCommandBar.text ();
      if (!str.isEmpty ())
        {
          if (run (str.toStdString ().c_str ()))
            {
              mCmdHistroy.emplace_back (str.toStdString ());
              mCurrHistroy = mCmdHistroy.size () - 1;
              cmdHide ();
            }
        }
      else
        {
          cmdHide ();
        }
    }
  else if (mCmdType == CmdStatusType::CMD_MESSAGE)
    {
      cmdHide ();
    }
}

void
ApvlvView::commandbarKeyPressed (QKeyEvent *gek)
{
  if (gek->key () == Qt::Key_Tab)
    {
      auto str = mCommandBar.text ();
      if (!str.isEmpty ())
        {
          cmdAuto (str.toStdString ().c_str () + 1);
        }
    }
  else if (gek->key () == Qt::Key_Backspace)
    {
      auto str = mCommandBar.text ();
      if (str.length () == 1)
        {
          cmdHide ();
          mCurrHistroy = mCmdHistroy.size () - 1;
        }
    }
  else if (gek->key () == Qt::Key_Escape || isAltEscape (gek))
    {
      cmdHide ();
      mCurrHistroy = mCmdHistroy.size () - 1;
    }
  else if (gek->key () == Qt::Key_Up)
    {
      if (!mCmdHistroy.empty ())
        {
          mCommandBar.setText (QString::fromLocal8Bit (
              mCurrHistroy > 0 ? mCmdHistroy[mCurrHistroy--]
                               : mCmdHistroy[0]));
        }
    }
  else if (gek->key () == Qt::Key_Down)
    {
      if (!mCmdHistroy.empty ())
        {
          mCommandBar.setText (QString::fromLocal8Bit (
              (size_t)mCurrHistroy < mCmdHistroy.size () - 1
                  ? mCmdHistroy[++mCurrHistroy]
                  : mCmdHistroy[mCmdHistroy.size () - 1]));
        }
    }
}

void
ApvlvView::closeEvent (QCloseEvent *evt)
{
  if (mParent == nullptr)
    {
      QGuiApplication::exit ();
    }
}

void
ApvlvView::tabSwitched (int pnum)
{
  qDebug () << "tabwidget switch to: " << pnum;
  if (pnum == -1)
    return;

  auto adoc = currentFrame ();
  if (adoc && adoc->filename ())
    {
      auto filename
          = filesystem::path (currentFrame ()->filename ()).filename ();
      setTitle (filename.string ());
    }
}

void
ApvlvView::switchTab (int tabPos)
{
  int ntabs = mTabContainer.count ();
  while (tabPos < 0)
    tabPos += ntabs;

  tabPos = tabPos % ntabs;
  mTabContainer.setCurrentIndex (tabPos);
}

void
ApvlvView::windowAdded ()
{
  auto index = mTabContainer.currentIndex ();
  Q_ASSERT (index != -1);
  updateTabName ();
}

void
ApvlvView::updateTabName ()
{
  auto win = currentWindow ();
  if (win == nullptr)
    return;

  auto frame = win->getFrame ();
  if (frame == nullptr)
    return;

  const char *filename = frame->filename ();
  string gfilename;

  if (filename == nullptr)
    gfilename = "None";
  else
    gfilename = filesystem::path (filename).filename ().string ();

  setTitle (gfilename);

  auto index = mTabContainer.currentIndex ();
  Q_ASSERT (index != -1);
  auto tagname = QString::fromLocal8Bit (gfilename);
  // need impl tagname = QString("[%1] %2").arg (mTabList[index]).arg
  // (gfilename.c_str());
  mTabContainer.setTabText (index, tagname);
}

void
ApvlvView::appendChild (ApvlvView *view)
{
  mChildren.push_back (view);
}

void
ApvlvView::eraseChild (ApvlvView *view)
{
  auto itr = mChildren.begin ();
  while (*itr != view && itr != mChildren.end ())
    itr++;

  if (*itr == view)
    {
      mChildren.erase (itr);
    }
}

void
ApvlvView::saveKey (int has, int ct, uint key, bool end)
{
  if (processInLast)
    return;

  if (keyLastEnd)
    {
      keySquence.clear ();
    }

  struct keyNode key_node = { has, ct, key, end };
  keySquence.push_back (key_node);
  keyLastEnd = end;
}

void
ApvlvView::processLastKey ()
{
  processInLast = true;
  for (auto node : keySquence)
    {
      process (node.Has, node.Ct, node.Key);
    }
  processInLast = false;
}
}

// Local Variables:
// mode: c++
// End:

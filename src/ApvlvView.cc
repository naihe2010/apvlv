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
#include <QKeyEvent>
#include <QLineEdit>
#include <QMainWindow>
#include <QMenu>
#include <QMenuBar>
#include <QToolBar>
#include <QToolButton>
#include <QVBoxLayout>
#include <filesystem>
#include <sstream>

#include "ApvlvInfo.h"
#include "ApvlvParams.h"
#include "ApvlvSearchDialog.h"
#include "ApvlvView.h"

namespace apvlv
{
static bool
isalt_escape (QKeyEvent *event)
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
  // qDebug ("command bar key press: %d, modifiers: %d", evt->key (),
  // evt->modifiers ().toInt ());
  if (evt->key () == Qt::Key_Escape || isalt_escape (evt)
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
      qDebug ("Ate key press tab");
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
      mParent->append_child (this);
    }

  mCmdType = CMD_NONE;

  mProCmd = 0;
  mProCmdCnt = 0;

  mCurrHistroy = -1;

  mHasFull = false;

  keyLastEnd = true;

  processInLast = false;

  int w = gParams->valuei ("width");
  int h = gParams->valuei ("height");

  if (gParams->valueb ("fullscreen"))
    {
      fullscreen ();
    }
  else
    {
      resize (w > 1 ? w : 800, h > 1 ? h : 600);
    }
  show ();

  mCentral = new QFrame ();
  setCentralWidget (mCentral);

  mMenuBar = new QMenuBar ();
  setupMenuBar ();
  setMenuBar (mMenuBar);
  mToolBar = new QToolBar ();
  setupToolBar ();
  addToolBar (Qt::TopToolBarArea, mToolBar);

  string guiopt = gParams->values ("guioptions");
  if (guiopt.find ('m') == string::npos)
    {
      mMenuBar->hide ();
    }
  if (guiopt.find ('T') == string::npos)
    {
      mToolBar->hide ();
    }

  auto box = new QVBoxLayout ();
  mCentral->setLayout (box);

  mTabContainer = new QTabWidget (this);
  mTabContainer->setTabBarAutoHide (true);
  box->addWidget (mTabContainer, 1);

  QObject::connect (mTabContainer, SIGNAL (currentChanged (int)), this,
                    SLOT (notebook_switch_cb (int)));
  QObject::connect (mTabContainer, SIGNAL (tabCloseRequested (int)), this,
                    SLOT (notebook_close_cb (int)));

  mCommandBar = new ApvlvCommandBar ();
  box->addWidget (mCommandBar, 0);

  QObject::connect (mCommandBar, SIGNAL (textEdited (const QString &)), this,
                    SLOT (commandbar_edit_cb (const QString &)));
  QObject::connect (mCommandBar, SIGNAL (returnPressed ()), this,
                    SLOT (commandbar_return_cb ()));
  QObject::connect (mCommandBar, SIGNAL (keyPressed (QKeyEvent *)), this,
                    SLOT (commandbar_keypress_cb (QKeyEvent *)));

  cmd_hide ();
}

ApvlvView::~ApvlvView ()
{
  if (mParent)
    {
      mParent->erase_child (this);
    }

  auto itr = mChildren.begin ();
  while (itr != mChildren.end ())
    {
      delete *itr;
      itr = mChildren.begin ();
    }

  for (auto &t : mTabList)
    {
      delete t->getRootWindow ();
    }
  mTabList.clear ();

  mCmdHistroy.clear ();
}

ApvlvWindow *
ApvlvView::currentWindow ()
{
  auto index = mTabContainer->currentIndex ();
  Q_ASSERT (index != -1);

  auto widget = QApplication::focusWidget ();
  if (widget)
    {
      auto win = mTabList[index]->findWindowByWidget (widget);
      if (win)
        return win;
    }

  return mTabList[index]->getActiveWindow ();
}

void
ApvlvView::delCurrentWindow ()
{
  auto win = currentWindow ();
  if (win)
    win->unbirth ();
  updatetabname ();
}

void
ApvlvView::open ()
{
  QString dirname;
  auto fp = gInfo->file (0);
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
  auto mimes = File::supportMimeTypes ();
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
      loadfile (string (filename.toLocal8Bit ().constData ()));
    }
}

void
ApvlvView::opendir ()
{
  QString dirname;
  auto fp = gInfo->file (0);
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
      loaddir (filename.toStdString ());
    }
}

bool
ApvlvView::loaddir (const string &path)
{
  crtadoc ()->setDirIndex (path);
  return true;
}

void
ApvlvView::quit (bool only_tab)
{
  if (int (mTabList.size ()) <= 1 || only_tab == false)
    {
      mTabList.clear ();
      closeEvent (nullptr);
      return;
    }

  auto index = mTabContainer->currentIndex ();
  Q_ASSERT (index != -1);
  delete_tabcontext (index);
  mTabContainer->removeTab (index);
}

void
ApvlvView::search ()
{
  promptcommand ('/');
}

void
ApvlvView::backSearch ()
{
  promptcommand ('?');
}

void
ApvlvView::advancedSearch ()
{
  auto diag = SearchDialog (this);
  QObject::connect (&diag, SIGNAL (loadFile (const string &, int)), this,
                    SLOT (loadFileOnPage (const string &, int)));
  diag.exec ();
}

bool
ApvlvView::newtab (const string &filename, bool disable_content)
{
  auto optndoc = hasloaded (filename);
  if (!optndoc)
    {
      auto ndoc = new ApvlvFrame (this);
      if (!ndoc->loadfile (filename, true, gParams->valueb ("content")))
        {
          delete ndoc;
          ndoc = nullptr;
        }

      if (ndoc)
        {
          regloaded (ndoc);
          optndoc = make_optional<ApvlvFrame *> (ndoc);
        }
    }

  if (optndoc)
    {
      newtab (optndoc.value ());
      return true;
    }
  else
    {
      return false;
    }
}

bool
ApvlvView::newtab (ApvlvFrame *core)
{
  auto pos = new_tabcontext (core);

  auto basename
      = core->filename ()
            ? filesystem::path (core->filename ()).filename ().string ()
            : "NONE";
  mTabContainer->insertTab (pos, mTabList[pos],
                            QString::fromLocal8Bit (basename));
  mTabContainer->setCurrentIndex (pos);

  return true;
}

int
ApvlvView::new_tabcontext (ApvlvFrame *core)
{
  auto context = new ApvlvWindowContext (this);

  new ApvlvWindow (context, core);

  auto index = mTabContainer->currentIndex () + 1;
  auto insPos = mTabList.begin () + index;
  mTabList.insert (insPos, context);

  return index;
}

void
ApvlvView::delete_tabcontext (int tabPos)
{
  auto iter = mTabList.begin () + tabPos;
  delete (*iter)->getRootWindow ();
  mTabList.erase (iter);
}

bool
ApvlvView::loadfile (const string &filename)
{
  auto abpath = filesystem::absolute (filename).string ();

  ApvlvWindow *win = currentWindow ();
  ApvlvFrame *ndoc = nullptr;

  auto optndoc = hasloaded (abpath);
  if (!optndoc)
    {
      ndoc = new ApvlvFrame (this);
      if (!ndoc->loadfile (filename, true, gParams->valueb ("content")))
        {
          delete ndoc;
          ndoc = nullptr;
        }
      else
        {
          regloaded (ndoc);
          optndoc = make_optional<ApvlvFrame *> (ndoc);
        }
    }

  if (optndoc)
    {
      win->setCore (optndoc.value ());
      updatetabname ();
    }

  return ndoc != nullptr;
}

void
ApvlvView::loadFileOnPage (const string &filename, int pn)
{
  auto cdoc = crtadoc ();
  if (cdoc)
    {
      if (loadfile (filename))
        {
          cdoc->showpage (pn, 0.0);
        }
    }
}

optional<ApvlvFrame *>
ApvlvView::hasloaded (const string &abpath)
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
ApvlvView::regloaded (ApvlvFrame *core)
{
  if (gParams->valuei ("pdfcache") < 2)
    {
      gParams->push ("pdfcache", "2");
    }

  if (mDocs.size () >= (size_t)gParams->valuei ("pdfcache"))
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

  mDocs.push_back (unique_ptr<ApvlvFrame> (core));
}

void
ApvlvView::setupMenuBar ()
{
  // File -> open, opendir
  auto mfile = new QMenu (tr ("File"));
  mMenuBar->addMenu (mfile);

  auto action = mfile->addAction (tr ("Open"));
  QObject::connect (action, SIGNAL (triggered (bool)), this, SLOT (open ()));
  action = mfile->addAction (tr ("OpenDir"));
  QObject::connect (action, SIGNAL (triggered (bool)), this,
                    SLOT (opendir ()));
  action = mfile->addAction (tr ("New Tab"));
  QObject::connect (action, SIGNAL (triggered (bool)), this, SLOT (newtab ()));
  action = mfile->addAction (tr ("Close Tab"));
  QObject::connect (action, SIGNAL (triggered (bool)), this,
                    SLOT (closetab ()));
  action = mfile->addAction (tr ("Quit"));
  QObject::connect (action, SIGNAL (triggered (bool)), this,
                    SLOT (quit (bool)));

  // Edit ->
  auto medit = new QMenu (tr ("Edit"));
  mMenuBar->addMenu (medit);
  action = medit->addAction (tr ("Search"));
  QObject::connect (action, SIGNAL (triggered (bool)), this, SLOT (search ()));
  action = medit->addAction (tr ("Back Search"));
  QObject::connect (action, SIGNAL (triggered (bool)), this,
                    SLOT (backSearch ()));
  action = medit->addAction (tr ("Advanced Search"));
  QObject::connect (action, SIGNAL (triggered (bool)), this,
                    SLOT (advancedSearch ()));

  // View ->
  auto mview = new QMenu (tr ("View"));
  mMenuBar->addMenu (mview);
  action = mview->addAction (tr ("ToolBar"));
  action->setCheckable (true);
  if (strchr (gParams->values ("guioptions"), 'T') == nullptr)
    {
      action->setChecked (false);
    }
  else
    {
      action->setChecked (true);
    }
  QObject::connect (action, SIGNAL (triggered (bool)), this,
                    SLOT (toggleToolBar ()));
  action = mview->addAction (tr ("Horizontal Split"));
  QObject::connect (action, SIGNAL (triggered (bool)), this, SLOT (hsplit ()));
  action = mview->addAction (tr ("Vertical Split"));
  QObject::connect (action, SIGNAL (triggered (bool)), this, SLOT (vsplit ()));
  action = mview->addAction (tr ("Close Split"));
  QObject::connect (action, SIGNAL (triggered (bool)), this,
                    SLOT (unbirth ()));

  // Navigate ->
  auto mnavigate = new QMenu (tr ("Navigate"));
  mMenuBar->addMenu (mnavigate);
  action = mnavigate->addAction (tr ("Previous Page"));
  QObject::connect (action, SIGNAL (triggered (bool)), this,
                    SLOT (previousPage ()));
  action = mnavigate->addAction (tr ("Next Page"));
  QObject::connect (action, SIGNAL (triggered (bool)), this,
                    SLOT (nextPage ()));

  // Tools
  auto mtools = new QMenu (tr ("Tools"));
  mMenuBar->addMenu (mtools);

  // Help -> about
  auto mhelp = new QMenu (tr ("Help"));
  mMenuBar->addMenu (mhelp);
}

void
ApvlvView::setupToolBar ()
{
  auto action = mToolBar->addAction (tr ("Open"));
  action->setIcon (
      QApplication::style ()->standardIcon (QStyle::SP_DialogOpenButton));
  QObject::connect (action, SIGNAL (triggered (bool)), this, SLOT (open ()));

  action = mToolBar->addAction (tr ("OpenDir"));
  action->setIcon (
      QApplication::style ()->standardIcon (QStyle::SP_DirOpenIcon));
  QObject::connect (action, SIGNAL (triggered (bool)), this,
                    SLOT (opendir ()));

  action = mToolBar->addAction (tr ("Previous Page"));
  action->setIcon (
      QApplication::style ()->standardIcon (QStyle::SP_ArrowLeft));
  QObject::connect (action, SIGNAL (triggered (bool)), this,
                    SLOT (previousPage ()));

  action = mToolBar->addAction (tr ("Next Page"));
  action->setIcon (
      QApplication::style ()->standardIcon (QStyle::SP_ArrowRight));
  QObject::connect (action, SIGNAL (triggered (bool)), this,
                    SLOT (nextPage ()));

  action = mToolBar->addAction (tr ("Toggle Content"));
  QObject::connect (action, SIGNAL (triggered (bool)), this,
                    SLOT (toggleContent ()));

  action = mToolBar->addAction (tr ("Quit"));
  QObject::connect (action, SIGNAL (triggered (bool)), this,
                    SLOT (quit (bool)));
}

void
ApvlvView::promptcommand (char ch)
{
  char s[2] = { 0 };
  s[0] = ch;
  mCommandBar->setText (s);
  cmd_show (CMD_CMD);
}

void
ApvlvView::promptcommand (const char *s)
{
  mCommandBar->setText (s);
  cmd_show (CMD_CMD);
}

void
ApvlvView::errormessage (const char *str, ...)
{
  char estr[512];
  va_list vap;
  va_start (vap, str);
  vsnprintf (estr, sizeof estr, str, vap);
  va_end (vap);
  mCommandBar->setText (QString ("ERROR: ") + estr);
  cmd_show (CMD_MESSAGE);
}

void
ApvlvView::infomessage (const char *str, ...)
{
  char estr[512];
  va_list vap;
  va_start (vap, str);
  vsnprintf (estr, sizeof estr, str, vap);
  va_end (vap);
  mCommandBar->setText (QString ("INFO: ") + estr);
  cmd_show (CMD_MESSAGE);
}

char *
ApvlvView::input (const char *str, int width, int height,
                  const string &content)
{
  // need impl
  return nullptr;
}

void
ApvlvView::cmd_show (int cmdtype)
{
  mCmdType = cmdtype;

  mCommandBar->show ();
  mCommandBar->setCursorPosition (1);
  mCommandBar->setFocus ();
}

void
ApvlvView::cmd_hide ()
{
  mCmdType = CMD_NONE;
  mCmdTime = chrono::steady_clock::now ();

  mCommandBar->hide ();
  mTabContainer->setFocus ();
}

void
ApvlvView::cmd_auto (const char *ps)
{
  stringstream ss (ps);
  string cmd, np, argu;
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

  qDebug ("find match: %s", np.c_str ());
  auto comtext = comp.complete (np);
  if (!comtext.empty ())
    {
      qDebug ("get a match: %s", comtext.c_str ());
      QString s = QString::fromLocal8Bit (comtext);
      s.replace (" ", "\\ ");
      QString linetext = QString::asprintf (":%s %s", cmd.c_str (),
                                            s.toStdString ().c_str ());
      mCommandBar->setText (linetext);
    }
  else
    {
      qDebug ("no get match");
    }
}

void
ApvlvView::fullscreen ()
{
  if (mHasFull == false)
    {
      ApvlvFrame *core = crtadoc ();
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
  crtadoc ()->nextpage (1);
}

void
ApvlvView::previousPage ()
{
  crtadoc ()->prepage (1);
}

void
ApvlvView::toggleContent ()
{
  crtadoc ()->toggleContent ();
}

void
ApvlvView::toggleToolBar ()
{
  if (mToolBar->isHidden ())
    {
      mToolBar->show ();
    }
  else
    {
      mToolBar->hide ();
    }
}

void
ApvlvView::newtab ()
{
  auto doc = crtadoc ()->copy ();
  newtab (doc);
}

void
ApvlvView::closetab ()
{
  quit (true);
}

void
ApvlvView::hsplit ()
{
  currentWindow ()->birth (ApvlvWindow::AW_SP, nullptr);
}

void
ApvlvView::vsplit ()
{
  currentWindow ()->birth (ApvlvWindow::AW_VSP, nullptr);
}

void
ApvlvView::unbirth ()
{
  currentWindow ()->unbirth ();
}

ApvlvFrame *
ApvlvView::crtadoc ()
{
  auto widget = QApplication::focusWidget ();
  if (widget)
    {
      // qDebug ("now focus in a %s widget", meta->className ());
      auto doc = ApvlvFrame::findByWidget (widget);
      if (doc)
        return doc;
    }

  ApvlvWindow *curwin = currentWindow ();
  Q_ASSERT (curwin);
  return curwin->getCore ();
}

ReturnType
ApvlvView::subprocess (int ct, uint key)
{
  uint procmd = mProCmd;
  mProCmd = 0;
  mProCmdCnt = 0;
  switch (procmd)
    {
    case 'Z':
      if (key == 'Z')
        quit (true);

    case CTRL ('w'):
      if (key == 'q' || key == CTRL ('Q'))
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
          ReturnType rv = currentWindow ()->process (ct, key);
          updatetabname ();
          return rv;
        }
      break;

    case 'g':
      if (key == 't')
        {
          if (ct == 0)
            switchtab (mTabContainer->currentIndex () + 1);
          else
            switchtab (ct - 1);
        }
      else if (key == 'T')
        {
          if (ct == 0)
            switchtab (mTabContainer->currentIndex () - 1);
          else
            switchtab (ct - 1);
        }
      else if (key == 'g')
        {
          if (ct == 0)
            ct = 1;
          crtadoc ()->showpage (ct - 1, 0.0);
        }
      break;

    default:
      return NO_MATCH;
    }

  return MATCH;
}

ReturnType
ApvlvView::process (int has, int ct, uint key)
{
  if (mProCmd != 0)
    {
      ReturnType ret = subprocess (mProCmdCnt, key);
      if (ret == MATCH)
        {
          saveKey (has, ct, key, true);
        }
      return ret;
    }

  switch (key)
    {
    case 'Z':
      mProCmd = 'Z';
      return NEED_MORE;

    case CTRL ('w'):
      mProCmd = CTRL ('w');
      mProCmdCnt = has ? ct : 1;
      saveKey (has, ct, key, false);
      return NEED_MORE;
    case 'q':
      quit (true);
      break;
    case 'f':
      fullscreen ();
      break;
    case '.':
      processLastKey ();
      break;
    case 'g':
      mProCmd = 'g';
      mProCmdCnt = has ? ct : 0;
      saveKey (has, ct, key, false);
      return NEED_MORE;
    default:
      ReturnType ret = crtadoc ()->process (has, ct, key);
      if (ret == NEED_MORE)
        {
          saveKey (has, ct, key, false);
        }
      else if (ret == MATCH)
        {
          saveKey (has, ct, key, true);
        }
    }

  return MATCH;
}

bool
ApvlvView::run (const char *str)
{
  bool ret;

  switch (*str)
    {
    case SEARCH:
      crtadoc ()->markposition ('\'');
      ret = crtadoc ()->search (str + 1, false);
      break;

    case BACKSEARCH:
      crtadoc ()->markposition ('\'');
      ret = crtadoc ()->search (str + 1, true);
      break;

    case COMMANDMODE:
      ret = runcmd (str + 1);
      break;

    case FIND:
      ret = crtadoc ()->search (str + 1, false);
      break;

    default:
      ret = false;
      break;
    }

  return ret;
}

void
ApvlvView::settitle (const string &title)
{
  setWindowTitle (QString::fromLocal8Bit (title));
}

bool
ApvlvView::runcmd (const char *str)
{
  bool ret = true;

  if (*str == '!')
    {
      system (str + 1);
    }
  else
    {
      stringstream ss (str);
      string cmd, subcmd, argu;
      ss >> cmd >> subcmd >> argu;

      if (subcmd.length () > 0 && subcmd[subcmd.length () - 1] == '\\')
        {
          subcmd.replace (subcmd.length () - 1, 1, 1, ' ');
          subcmd += argu;
          ss >> argu;
        }

      if (cmd == "set")
        {
          if (subcmd == "skip")
            {
              crtadoc ()->setskip (int (strtol (argu.c_str (), nullptr, 10)));
            }
          else
            {
              gParams->push (subcmd, argu);
            }
        }
      else if (cmd == "map" && !subcmd.empty ())
        {
          apvlv::ApvlvCmds::buildmap (subcmd.c_str (), argu.c_str ());
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
              ret = loaddir (subcmd);
            }
          else if (filesystem::is_regular_file (subcmd))
            {
              ret = loadfile (subcmd);
            }
          else
            {
              errormessage ("no file '%s'", subcmd.c_str ());
              ret = false;
            }
        }
      else if (cmd == "TOtext" && !subcmd.empty ())
        {
          crtadoc ()->totext (subcmd.c_str ());
        }
      else if ((cmd == "pr" || cmd == "print"))
        {
          crtadoc ()->print (
              subcmd.empty () ? -1
                              : int (strtol (subcmd.c_str (), nullptr, 10)));
        }
      else if (cmd == "sp")
        {
          if (currentWindow ()->birth (ApvlvWindow::AW_SP, nullptr))
            {
              windowadded ();
            }
        }
      else if (cmd == "vsp")
        {
          if (currentWindow ()->birth (ApvlvWindow::AW_VSP, nullptr))
            {
              windowadded ();
            }
        }
      else if ((cmd == "zoom" || cmd == "z") && !subcmd.empty ())
        {
          crtadoc ()->setzoom (subcmd.c_str ());
        }
      else if (cmd == "forwardpage" || cmd == "fp")
        {
          if (subcmd.empty ())
            crtadoc ()->nextpage (1);
          else
            crtadoc ()->nextpage (int (strtol (subcmd.c_str (), nullptr, 10)));
        }
      else if (cmd == "prewardpage" || cmd == "bp")
        {
          if (subcmd.empty ())
            crtadoc ()->prepage (1);
          else
            crtadoc ()->prepage (int (strtol (subcmd.c_str (), nullptr, 10)));
        }
      else if (cmd == "content")
        {
          crtadoc ()->toggleContent ();
        }
      else if (cmd == "goto" || cmd == "g")
        {
          crtadoc ()->markposition ('\'');
          auto p = strtol (subcmd.c_str (), nullptr, 10);
          p += crtadoc ()->getskip ();
          crtadoc ()->showpage (int (p - 1), 0.0);
        }
      else if (cmd == "help" || cmd == "h")
        {
          loadfile (helppdf);
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
          newtab (helppdf);
        }
      else if (cmd == "tabn" || cmd == "tabnext")
        {
          switchtab (mTabContainer->currentIndex () + 1);
        }
      else if (cmd == "tabp" || cmd == "tabprevious")
        {
          switchtab (mTabContainer->currentIndex () - 1);
        }
      else if (cmd == "toc")
        {
          loadfile (crtadoc ()->filename ());
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
          if (isn && crtadoc ())
            {
              auto p = strtol (cmd.c_str (), nullptr, 10);
              p += crtadoc ()->getskip ();
              if (p != crtadoc ()->pageNumber ())
                {
                  crtadoc ()->showpage (int (p - 1), 0.0);
                }
            }
          else
            {
              errormessage ("no command: '%s'", cmd.c_str ());
              ret = false;
            }
        }
    }

  return ret;
}

void
ApvlvView::keyPressEvent (QKeyEvent *evt)
{
  if (mCmdType != CMD_NONE)
    {
      return;
    }

  auto now = chrono::steady_clock::now ();
  auto millis
      = std::chrono::duration_cast<std::chrono::milliseconds> (now - mCmdTime)
            .count ();
  if (millis < 1000L)
    return;

  mCmds.append (evt);
}

void
ApvlvView::commandbar_edit_cb (const QString &str)
{
}

void
ApvlvView::commandbar_return_cb ()
{
  if (mCmdType == CMD_CMD)
    {
      auto str = mCommandBar->text ();
      if (!str.isEmpty ())
        {
          if (run (str.toStdString ().c_str ()))
            {
              mCmdHistroy.emplace_back (str.toStdString ());
              mCurrHistroy = mCmdHistroy.size () - 1;
              cmd_hide ();
            }
          else
            {
            }
        }
      else
        {
          cmd_hide ();
        }
    }
  else if (mCmdType == CMD_MESSAGE)
    {
      cmd_hide ();
    }
}

void
ApvlvView::commandbar_keypress_cb (QKeyEvent *gek)
{
  if (gek->key () == Qt::Key_Tab)
    {
      auto str = mCommandBar->text ();
      if (!str.isEmpty ())
        {
          cmd_auto (str.toStdString ().c_str () + 1);
        }
    }
  else if (gek->key () == Qt::Key_Backspace)
    {
      auto str = mCommandBar->text ();
      if (str.length () == 1)
        {
          cmd_hide ();
          mCurrHistroy = mCmdHistroy.size () - 1;
        }
    }
  else if (gek->key () == Qt::Key_Escape || isalt_escape (gek))
    {
      cmd_hide ();
      mCurrHistroy = mCmdHistroy.size () - 1;
    }
  else if (gek->key () == Qt::Key_Up)
    {
      if (!mCmdHistroy.empty ())
        {
          mCommandBar->setText (QString::fromLocal8Bit (
              mCurrHistroy > 0 ? mCmdHistroy[mCurrHistroy--]
                               : mCmdHistroy[0]));
        }
    }
  else if (gek->key () == Qt::Key_Down)
    {
      if (!mCmdHistroy.empty ())
        {
          mCommandBar->setText (QString::fromLocal8Bit (
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
ApvlvView::notebook_switch_cb (int pnum)
{
  qDebug ("tabwidget switch to: %d", pnum);
  if (pnum == -1)
    return;

  auto adoc = crtadoc ();
  if (adoc && adoc->filename ())
    {
      auto filename = filesystem::path (crtadoc ()->filename ()).filename ();
      settitle (filename.string ());
    }
}

void
ApvlvView::notebook_close_cb (int ind)
{
  qDebug ("tabwidget will close %d", ind);
  if (mTabContainer->currentIndex () == -1)
    {
      if (mTabContainer->count () > 0)
        mTabContainer->setCurrentIndex (0);
    }
}

void
ApvlvView::switchtab (long tabPos)
{
  int ntabs = int (mTabList.size ());
  while (tabPos < 0)
    tabPos += ntabs;

  tabPos = tabPos % ntabs;
  mTabContainer->setCurrentIndex (tabPos);
}

void
ApvlvView::windowadded ()
{
  auto index = mTabContainer->currentIndex ();
  Q_ASSERT (index != -1);
  updatetabname ();
}

void
ApvlvView::updatetabname ()
{
  char tagname[26];

  const char *filename = currentWindow ()->getCore ()->filename ();
  string gfilename;

  if (filename == nullptr)
    gfilename = "None";
  else
    gfilename = filesystem::path (filename).filename ().string ();

  settitle (gfilename);

  auto index = mTabContainer->currentIndex ();
  Q_ASSERT (index != -1);
  if (mTabList[index]->getWindowCount () > 1)
    snprintf (tagname, sizeof tagname, "[%d] %s",
              mTabList[index]->getWindowCount (), gfilename.c_str ());
  else
    snprintf (tagname, sizeof tagname, "%s", gfilename.c_str ());

  mTabContainer->setTabText (index, tagname);
}

void
ApvlvView::append_child (ApvlvView *view)
{
  mChildren.push_back (view);
}

void
ApvlvView::erase_child (ApvlvView *view)
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

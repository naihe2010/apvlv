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
/* @CPPFILE ApvlvSearchDialog.cc
 *
 *  Author: Alf <naihe2010@126.com>
 */
/* @date Created: 2024/07/13 17:27:00 Alf */

#include <QCheckBox>
#include <QFileDialog>
#include <QFileSystemModel>
#include <QPushButton>
#include <QScrollArea>
#include <QSplitter>
#include <QVBoxLayout>
#include <algorithm>
#include <stack>

#include "ApvlvFile.h"
#include "ApvlvSearch.h"

namespace apvlv
{
using namespace std;

bool
ApvlvSearchOptions::operator== (const ApvlvSearchOptions &other) const
{
  if (mText != other.mText)
    return false;
  if (mCaseSensitive != other.mCaseSensitive)
    return false;
  if (mRegex != other.mRegex)
    return false;
  if (mTypes != other.mTypes)
    return false;
  if (mFromDir != other.mFromDir)
    return false;
  return true;
}

Searcher::Searcher () : mRestart (false), mQuit (false)
{
  auto task = thread (&Searcher::dispatch, this);
  mTasks.emplace_back (move (task));

  auto cpu_count = thread::hardware_concurrency ();
  for (auto ind = 0u; ind < cpu_count - 1; ++ind)
    {
      task = thread (&Searcher::fileLoopFunc, this);
      mTasks.emplace_back (move (task));
    }
}

Searcher::~Searcher ()
{
  mQuit.store (true);
  for_each (mTasks.begin (), mTasks.end (),
            [] (thread &task) { task.join (); });
  qDebug ("all search threads ended");
}

void
Searcher::submit (const ApvlvSearchOptions &options)
{
  mOptions = options;
  auto path = filesystem::path (options.mFromDir);
  if (is_regular_file (path))
    {
      mFilenameQueue.push (move (absolute (path)));
    }
  mRestart.store (true);
}

bool
Searcher::get (ApvlvSearchResults &results)
{
  return mResults.pop (results);
}

void
Searcher::dispatch ()
{
  while (mQuit.load () == false)
    {
      if (mRestart.load () == true)
        {
          mRestart.store (false);
          mFilenameQueue.clear ();
          mResults.clear ();

          try
            {
              dirFunc ();
            }
          catch (const exception &ext)
            {
              qWarning ("search occurred error: %s", ext.what ());
            }
        }
      else
        {
          this_thread::sleep_for (1s);
        }
    }
}

void
Searcher::dirFunc ()
{
  qDebug ("searching %s from %s", mOptions.mText.c_str (),
          mOptions.mFromDir.c_str ());
  stack<string> dirs;
  dirs.push (mOptions.mFromDir);
  while (!dirs.empty ())
    {
      if (mRestart.load () == true || mQuit.load () == true)
        {
          return;
        }

      auto dir = dirs.top ();
      dirs.pop ();
      filesystem::directory_iterator itr (dir);
      for (const auto &entry : itr)
        {
          if (mRestart.load () == true || mQuit.load () == true)
            {
              return;
            }

          if (entry.is_directory ())
            {
              if (entry.path ().filename () != "."
                  && entry.path ().filename () != "..")
                dirs.push (entry.path ().string ());
            }
          else if (entry.is_regular_file ())
            {
              auto ext = entry.path ().extension ();
              if (ext.empty ())
                continue;

              auto titr
                  = find (mOptions.mTypes.begin (), mOptions.mTypes.end (),
                          entry.path ().extension ());
              if (titr != mOptions.mTypes.end ())
                {
                  mFilenameQueue.push (entry.path ().string ());
                }
            }
        }
    }
}

void
Searcher::fileLoopFunc ()
{
  while (mQuit.load () == false)
    {
      string name;
      if (mFilenameQueue.pop (name))
        {
          fileFunc (name);
        }
      else
        {
          this_thread::sleep_for (100ms);
        }
    }
}

void
Searcher::fileFunc (const string &path)
{
  unique_ptr<ApvlvFile> file{ ApvlvFile::newFile (path, false) };
  if (file)
    {
      auto results = file->search (mOptions.mText, mOptions.mCaseSensitive,
                                   mOptions.mRegex);
      if (!results.empty ())
        {
          mResults.push ({ path, results });
        }
    }
}

ApvlvSearchDialog::ApvlvSearchDialog (QWidget *parent)
{
  auto vbox = new QVBoxLayout;
  setLayout (vbox);

  // search line
  auto hbox = new QHBoxLayout;
  vbox->addLayout (hbox);

  hbox->addWidget (&mSearchEdit, 1);
  QObject::connect (&mSearchEdit, SIGNAL (returnPressed ()), this,
                    SLOT (search ()));

  mCaseSensitive.setText (tr ("Case sensitive"));
  hbox->addWidget (&mCaseSensitive);

  mRegex.setText (tr ("Regular expression"));
  hbox->addWidget (&mRegex);

  auto hbox2 = new QHBoxLayout;
  vbox->addLayout (hbox2);
  auto label = new QLabel (tr ("Find Directory: "));
  hbox2->addWidget (label, 0);
  hbox2->addWidget (&mFromDir, 1);
  mFromDir.setText (QDir::homePath ());
  auto dir_button = new QPushButton (tr ("..."));
  dir_button->setFocusPolicy (Qt::NoFocus);
  hbox2->addWidget (dir_button, 0);
  QObject::connect (dir_button, &QPushButton::clicked, this, [&] () {
    auto dir = QFileDialog::getExistingDirectory ();
    if (!dir.isEmpty ())
      mFromDir.setText (dir);
  });

  // file type line
  auto hbox3 = new QHBoxLayout;
  vbox->addLayout (hbox3);

  auto mime_types = ApvlvFile::supportMimeTypes ();
  for_each (mime_types.begin (), mime_types.end (), [&] (const auto &pair) {
    for_each (pair.second.begin (), pair.second.end (), [&] (const auto &ext) {
      auto checkbox = new QCheckBox (QString::fromStdString (ext));
      checkbox->setChecked (true);
      hbox3->addWidget (checkbox);
      mTypes.emplace_back (checkbox);
    });
  });

  auto spliter = new QSplitter;
  vbox->addWidget (spliter);

  spliter->setOrientation (Qt::Vertical);

  spliter->addWidget (&mResults);
  spliter->addWidget (&mPreview);
  mPreview.resize (400, 300);
  QObject::connect (
      &mResults,
      SIGNAL (currentItemChanged (QListWidgetItem *, QListWidgetItem *)), this,
      SLOT (previewItem (QListWidgetItem *)));
  QObject::connect (&mResults, SIGNAL (itemActivated (QListWidgetItem *)),
                    this, SLOT (activateItem (QListWidgetItem *)));
  QObject::connect (&mPreview, SIGNAL (loadFinished (bool)), this,
                    SLOT (loadFinish (bool)));

  QObject::connect (&mGetTimer, SIGNAL (timeout ()), this,
                    SLOT (getResults ()));
  mGetTimer.start (100);
}

void
ApvlvSearchDialog::search ()
{
  ApvlvSearchOptions options;
  options.mText = mSearchEdit.text ().trimmed ().toStdString ();
  options.mCaseSensitive = mCaseSensitive.isChecked ();
  options.mRegex = mRegex.isChecked ();
  options.mFromDir = mFromDir.text ().toStdString ();
  for (const auto &type : mTypes)
    {
      auto ext = type->text ().replace ("&", "");
      if (type->isChecked ())
        options.mTypes.emplace_back (ext.toStdString ());
    }
  if (mOptions == options)
    return;

  mSearcher.submit (options);
  mOptions = options;
}

void
ApvlvSearchDialog::getResults ()
{
  ApvlvSearchResults results;
  while (mSearcher.get (results))
    {
      displayResults (results);
    }
}

void
ApvlvSearchDialog::previewItem (QListWidgetItem *item)
{
  if (mPreviewIsFinished == false)
    return;

  auto words = item->data (Qt::UserRole).toStringList ();
  auto path = words[0].toStdString ();
  auto pn = words[1].toInt ();
  if (mPreviewFile && mPreviewFile->getFilename () != path)
    mPreviewFile = nullptr;

  if (mPreviewFile == nullptr)
    mPreviewFile = unique_ptr<ApvlvFile>{ ApvlvFile::newFile (path, false) };

  if (mPreviewFile)
    {
      double x, y;
      mPreview.setFile (mPreviewFile.get ());
      mPreviewFile->pagesize (pn, 0, &x, &y);
      mPreviewIsFinished = false;
      mPreviewFile->render (pn, int (x), int (y), 1.0, 0, &mPreview);
    }
}

void
ApvlvSearchDialog::activateItem (QListWidgetItem *item)
{
  auto words = item->data (Qt::UserRole).toStringList ();
  auto path = words[0];
  auto pn = words[1].toInt ();
  emit loadFile (path.toStdString (), pn);
  accept ();
}

void
ApvlvSearchDialog::displayResults (const ApvlvSearchResults &results)
{
  auto line = QString::fromStdString (results.filename);
  for (const auto &result : results.result_list)
    {
      auto pos = line + ':' + QString::number (result.page + 1);
      for_each (result.matches.begin (), result.matches.end (),
                [&] (const auto &match) {
                  auto matchitem = new QListWidgetItem (
                      { QString::fromStdString (match.line) });
                  matchitem->setToolTip (pos);
                  QStringList data{ line, QString::number (result.page) };
                  matchitem->setData (Qt::UserRole, data);
                  mResults.addItem (matchitem);
                });
    }
}

void
ApvlvSearchDialog::loadFinish (bool ret)
{
  mPreviewIsFinished = true;
}

}
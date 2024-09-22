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
/* @CPPFILE SearchDialog.cc
 *
 *  Author: Alf <naihe2010@126.com>
 */

#include <QCheckBox>
#include <QFileDialog>
#include <QFileSystemModel>
#include <QPushButton>
#include <QSplitter>
#include <QVBoxLayout>
#include <algorithm>
#include <regex>
#include <stack>

#include "ApvlvFile.h"
#include "ApvlvParams.h"
#include "ApvlvSearchDialog.h"

namespace apvlv
{
using namespace std;

SearchDialog::SearchDialog (QWidget *parent) : mPreviewIsFinished (true)
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
  QObject::connect (&mFromDir, SIGNAL (returnPressed ()), this,
                    SLOT (search ()));
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

  auto mime_types = File::supportMimeTypes ();
  for_each (mime_types.begin (), mime_types.end (), [&] (const auto &pair) {
    for_each (pair.second.begin (), pair.second.end (), [&] (const auto &ext) {
      auto checkbox = new QCheckBox (QString::fromLocal8Bit (ext));
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
SearchDialog::search ()
{
  SearchOptions options;
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
  mResults.clear ();
  mOptions = options;
}

void
SearchDialog::getResults ()
{
  unique_ptr<SearchFileMatch> result;
  while ((result = mSearcher.get ()) != nullptr)
    {
      displayResult (std::move (result));
    }
}

void
SearchDialog::previewItem (QListWidgetItem *item)
{
  if (item == nullptr)
    return;

  if (mPreviewIsFinished == false)
    return;

  auto words = item->data (Qt::UserRole).toStringList ();
  auto path = words[0].toStdString ();
  auto pn = words[1].toInt ();
  if (mPreviewFile && mPreviewFile->getFilename () != path)
    mPreviewFile = nullptr;

  if (mPreviewFile == nullptr)
    mPreviewFile = unique_ptr<File>{ File::loadFile (path, false) };

  if (mPreviewFile)
    {
      mPreview.setFile (mPreviewFile.get ());
      mPreviewIsFinished = false;
      mPreviewFile->pageRender (pn, 1.0, 0, &mPreview);
    }
}

void
SearchDialog::activateItem (QListWidgetItem *item)
{
  auto words = item->data (Qt::UserRole).toStringList ();
  auto path = words[0];
  auto pn = words[1].toInt ();
  emit loadFile (path.toStdString (), pn);
  accept ();
}

void
SearchDialog::displayResult (unique_ptr<SearchFileMatch> result)
{
  auto line = QString::fromLocal8Bit (result->filename);
  for (const auto &page : result->page_matches)
    {
      auto pos = line + ':' + QString::number (page.page + 1);
      for_each (page.matches.begin (), page.matches.end (),
                [&] (const auto &match) {
                  auto matchitem = new QListWidgetItem (
                      { QString::fromLocal8Bit (match.line) });
                  matchitem->setToolTip (pos);
                  QStringList data{ line, QString::number (page.page) };
                  matchitem->setData (Qt::UserRole, data);
                  mResults.addItem (matchitem);
                });
    }
}

void
SearchDialog::loadFinish (bool ret)
{
  mPreviewIsFinished = true;
}

}

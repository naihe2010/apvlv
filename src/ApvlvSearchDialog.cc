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
#include <QFileSystemModel>
#include <QSplitter>
#include <QVBoxLayout>

#include "ApvlvFile.h"
#include "ApvlvSearchDialog.h"

namespace apvlv
{
ASFileModel::ASFileModel ()
{
  setRootPath (QDir::homePath ());
  setFilter (QDir::AllEntries | QDir::NoDotAndDotDot);
  auto mime_types = ApvlvFile::supportMimeTypes ();
  QStringList name_filters;
  for (const auto &mime_type : mime_types)
    {
      for (const auto &ext : mime_type.second)
        name_filters << QString ("*") + ext.c_str ();
    }
  setNameFilters (name_filters);
}

ApvlvSearchDialog::ApvlvSearchDialog (QWidget *parent)
{
  auto vbox = new QVBoxLayout;
  setLayout (vbox);

  // search line
  auto hbox = new QHBoxLayout;
  vbox->addLayout (hbox);

  hbox->addWidget (&mSearchEdit, 1);

  mCaseSensitive.setText (tr ("Case sensitive"));
  hbox->addWidget (&mCaseSensitive);

  mRegex.setText (tr ("Regular expression"));
  hbox->addWidget (&mRegex);

  // file type line
  auto hbox2 = new QHBoxLayout;
  vbox->addLayout (hbox2);

  auto mime_types = ApvlvFile::supportMimeTypes ();
  for (const auto &pair : mime_types)
    {
      auto checkbox = new QCheckBox (QString::fromStdString (pair.first));
      checkbox->setChecked (true);
      QString exts = QString::fromStdString (pair.second[0]);
      for (decltype (pair.second.size ()) index = 1;
           index < pair.second.size (); index++)
        exts += QString (", %1").arg (
            QString::fromStdString (pair.second[index]));
      checkbox->setToolTip (exts);
      hbox2->addWidget (checkbox);
      mTypes.emplace_back (checkbox);
    }

  auto spliter = new QSplitter;
  vbox->addWidget (spliter);

  spliter->setOrientation (Qt::Vertical);

  spliter->addWidget (&mDirView);
  spliter->addWidget (&mResults);
  spliter->addWidget (&mPreview);

  mDirModel.setRootPath (QDir::homePath ());
  mDirView.setModel (&mDirModel);

  QObject::connect (&mSearchEdit, SIGNAL (returnPressed ()), this,
                    SLOT (search ()));
}

void
ApvlvSearchDialog::search ()
{
}
}

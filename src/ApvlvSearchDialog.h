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
/* @CPPFILE ApvlvSearchDialog.h
 *
 *  Author: Alf <naihe2010@126.com>
 */
/* @date Created: 2024/07/13 17:26:00 Alf */

#ifndef _APVLV_SEARCH_DIALOG_H_
#define _APVLV_SEARCH_DIALOG_H_

#include <QCheckBox>
#include <QDialog>
#include <QFileSystemModel>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QTreeView>

namespace apvlv
{
class ASFileModel : public QFileSystemModel
{
  Q_OBJECT
public:
  ASFileModel ();
  int
  columnCount (const QModelIndex &parent = QModelIndex ()) const override
  {
    return 1;
  };
};
class ApvlvSearchDialog : public QDialog
{
  Q_OBJECT
public:
  explicit ApvlvSearchDialog (QWidget *parent = 0);
  ~ApvlvSearchDialog () {}

signals:
  void loadFile (const QString &path);

private slots:
  void search ();

private:
  QLineEdit mSearchEdit;
  QCheckBox mCaseSensitive;
  QCheckBox mRegex;
  vector<QCheckBox *> mTypes;
  ASFileModel mDirModel;
  QTreeView mDirView;
  QListWidget mResults;
  QLabel mPreview;
};

}

#endif

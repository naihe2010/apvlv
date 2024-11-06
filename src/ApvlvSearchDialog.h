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
/* @CPPFILE SearchDialog.h
 *
 *  Author: Alf <naihe2010@126.com>
 */

#ifndef _APVLV_SEARCH_DIALOG_H_
#define _APVLV_SEARCH_DIALOG_H_

#include <QCheckBox>
#include <QDialog>
#include <QFileSystemModel>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QSplitter>
#include <QTimer>
#include <QVBoxLayout>
#include <string>
#include <thread>
#include <vector>

#include "ApvlvQueue.h"
#include "ApvlvWebViewWidget.h"

namespace apvlv
{

class File;
class SearchDialog : public QDialog
{
  Q_OBJECT
public:
  explicit SearchDialog (QWidget *parent = nullptr);
  ~SearchDialog () override = default;

signals:
  void loadFile (const std::string &path, int pn);

private slots:
  void search ();
  void getResults ();
  void previewItem (QListWidgetItem *item);
  void activateItem (QListWidgetItem *item);
  void loadFinish (bool ret);

private:
  void displayResult (std::unique_ptr<SearchFileMatch> result);

  QVBoxLayout mVBox;
  QHBoxLayout mHBox;
  QHBoxLayout mHBox2;
  QHBoxLayout mHBox3;
  QSplitter mSplitter;

  QLabel mLabel;

  QPushButton mDirButton;

  SearchOptions mOptions;

  Searcher mSearcher;

  QTimer mGetTimer;

  QLineEdit mSearchEdit;
  QCheckBox mCaseSensitive;
  QCheckBox mRegex;
  std::vector<QCheckBox *> mTypes;
  QLineEdit mFromDir;
  QListWidget mResults;
  WebView mPreview;

  std::unique_ptr<File> mPreviewFile;
  bool mPreviewIsFinished;
};

}

#endif

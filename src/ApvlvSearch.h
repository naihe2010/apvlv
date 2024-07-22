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
#include <QTimer>
#include <string>
#include <thread>
#include <vector>

#include "ApvlvFile.h"
#include "ApvlvQueue.h"

namespace apvlv
{
class ApvlvSearchOptions
{
public:
  bool operator== (const ApvlvSearchOptions &other) const;

  string mText;
  bool mCaseSensitive;
  bool mRegex;
  string mFromDir;
  vector<string> mTypes;
};

class Searcher
{
public:
  Searcher ();
  ~Searcher ();

  void submit (const ApvlvSearchOptions &options);
  bool get (ApvlvSearchResults &results);

private:
  void dispatch ();
  void dirFunc ();
  void fileLoopFunc ();
  void fileFunc (const string &path);

  vector<thread> mTasks;

  ApvlvSearchOptions mOptions;
  LockQueue<string> mFilenameQueue;
  LockQueue<ApvlvSearchResults> mResults;
  atomic<bool> mRestart;
  atomic<bool> mQuit;
};

class ApvlvSearchDialog : public QDialog
{
  Q_OBJECT
public:
  explicit ApvlvSearchDialog (QWidget *parent = nullptr);
  ~ApvlvSearchDialog () {}

signals:
  void loadFile (const string &path, int pn);

private slots:
  void search ();
  void getResults ();
  void previewItem (QListWidgetItem *item);
  void activateItem (QListWidgetItem *item);
  void displayResults (const ApvlvSearchResults &results);

private:
  ApvlvSearchOptions mOptions;

  Searcher mSearcher;

  QTimer mGetTimer;

  QLineEdit mSearchEdit;
  QCheckBox mCaseSensitive;
  QCheckBox mRegex;
  vector<QCheckBox *> mTypes;
  QLineEdit mFromDir;
  QListWidget mResults;
  QLabel mPreview;
};

}

#endif

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
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QTimer>
#include <string>
#include <thread>
#include <vector>

#include "ApvlvQueue.h"
#include "ApvlvWebView.h"

namespace apvlv
{

struct SearchMatch
{
  string match;
  string line;
  size_t pos, length;
};

using SearchMatchList = vector<SearchMatch>;

struct SearchPageMatch
{
  int page;
  SearchMatchList matches;
};

struct SearchFileMatch
{
  string filename;
  vector<SearchPageMatch> page_matches;
};

class SearchOptions
{
public:
  bool operator== (const SearchOptions &other) const;

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

  void submit (const SearchOptions &options);
  unique_ptr<SearchFileMatch> get ();

private:
  void dispatch ();
  void dirFunc ();
  void fileLoopFunc ();
  void fileFunc (const string &path);

  vector<thread> mTasks;

  SearchOptions mOptions;
  LockQueue<string> mFilenameQueue;
  LockQueue<unique_ptr<SearchFileMatch> > mResults;
  atomic<bool> mRestart;
  atomic<bool> mQuit;
};

class File;
class SearchDialog : public QDialog
{
  Q_OBJECT
public:
  explicit SearchDialog (QWidget *parent = nullptr);
  ~SearchDialog () {}

signals:
  void loadFile (const string &path, int pn);

private slots:
  void search ();
  void getResults ();
  void previewItem (QListWidgetItem *item);
  void activateItem (QListWidgetItem *item);
  void loadFinish (bool ret);

private:
  void displayResult (unique_ptr<SearchFileMatch> result);

  SearchOptions mOptions;

  Searcher mSearcher;

  QTimer mGetTimer;

  QLineEdit mSearchEdit;
  QCheckBox mCaseSensitive;
  QCheckBox mRegex;
  vector<QCheckBox *> mTypes;
  QLineEdit mFromDir;
  QListWidget mResults;
  ApvlvWebview mPreview;

  unique_ptr<File> mPreviewFile;
  bool mPreviewIsFinished;
};

vector<pair<size_t, size_t> > grep (const string &source, const string &text,
                                    bool is_case, bool is_regex);

}

#endif

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

#ifndef _APVLV_SEARCH_H_
#define _APVLV_SEARCH_H_

#include <string>
#include <thread>
#include <vector>

#include "ApvlvQueue.h"

namespace apvlv
{

using std::atomic;
using std::pair;
using std::string;
using std::thread;
using std::unique_ptr;
using std::vector;

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

vector<pair<size_t, size_t> > grep (const string &source, const string &text,
                                    bool is_case, bool is_regex);

}

#endif

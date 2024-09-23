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

struct SearchMatch
{
  std::string match;
  std::string line;
  size_t pos, length;
};

using SearchMatchList = std::vector<SearchMatch>;

struct SearchPageMatch
{
  int page;
  SearchMatchList matches;
};

struct SearchFileMatch
{
  std::string filename;
  std::vector<SearchPageMatch> page_matches;
};

class SearchOptions
{
public:
  bool operator== (const SearchOptions &other) const;

  std::string mText;
  bool mCaseSensitive;
  bool mRegex;
  std::string mFromDir;
  std::vector<std::string> mTypes;
};

class Searcher
{
public:
  Searcher ();
  ~Searcher ();

  void submit (const SearchOptions &options);
  std::unique_ptr<SearchFileMatch> get ();

private:
  void dispatch ();
  void dirFunc ();
  void fileLoopFunc ();
  void fileFunc (const std::string &path);

  std::vector<std::thread> mTasks;

  SearchOptions mOptions;
  LockQueue<std::string> mFilenameQueue;
  LockQueue<std::unique_ptr<SearchFileMatch> > mResults;
  std::atomic<bool> mRestart;
  std::atomic<bool> mQuit;
};

std::vector<std::pair<size_t, size_t> > grep (const std::string &source,
                                              const std::string &text,
                                              bool is_case, bool is_regex);

}

#endif

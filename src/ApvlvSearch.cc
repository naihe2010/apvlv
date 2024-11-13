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

#include <QDebug>
#include <algorithm>
#include <filesystem>
#include <regex>
#include <stack>

#include "ApvlvFile.h"
#include "ApvlvParams.h"
#include "ApvlvSearch.h"

namespace apvlv
{
using namespace std;

Searcher::Searcher () : mRestart (false), mQuit (false)
{
  auto task = thread (&Searcher::dispatch, this);
  mTasks.emplace_back (std::move (task));

  auto thread_count = thread::hardware_concurrency () - 1;
  auto thread_value
      = ApvlvParams::instance ()->getStringOrDefault ("thread_count", "auto");
  if (thread_value != "auto")
    {
      thread_count = ApvlvParams::instance ()->getIntOrDefault ("thread_count",
                                                                thread_count);
    }

  for (auto ind = 0u; ind < thread_count; ++ind)
    {
      task = thread (&Searcher::fileLoopFunc, this);
      mTasks.emplace_back (std::move (task));
    }
}

Searcher::~Searcher ()
{
  mRestart.store (true);
  mQuit.store (true);
  for_each (mTasks.begin (), mTasks.end (),
            [] (thread &task) { task.join (); });
  qDebug ("all search threads ended");
}

void
Searcher::submit (const SearchOptions &options)
{
  mOptions = options;
  auto path = filesystem::path (options.mFromDir);
  if (is_regular_file (path))
    {
      mFilenameQueue.push (absolute (path).string ());
    }
  mRestart.store (true);
}

unique_ptr<SearchFileMatch>
Searcher::get ()
{
  unique_ptr<SearchFileMatch> ptr;
  mResults.pop (ptr);
  return ptr;
}

void
Searcher::dispatch ()
{
  while (mQuit.load () == false)
    {
      if (mRestart.load () == true)
        {
          this_thread::sleep_for (2s);

          mFilenameQueue.clear ();
          mResults.clear ();

          mRestart.store (false);

          try
            {
              dirFunc ();
            }
          catch (const exception &ext)
            {
              qWarning () << "search occurred error: " << ext.what ();
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
  qDebug () << "searching " << mOptions.mText << " from " << mOptions.mFromDir;
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
          this_thread::sleep_for (1s);
        }
    }
}

void
Searcher::fileFunc (const string &path)
{
  auto file = FileFactory::loadFile (path);
  if (file)
    {
      auto result = file->grepFile (mOptions.mText, mOptions.mCaseSensitive,
                                    mOptions.mRegex, mRestart);
      if (result)
        mResults.push (std::move (result));
    }
}

vector<pair<size_t, size_t> >
grep (const string &source, const string &text, bool is_case, bool is_regex)
{
  vector<pair<size_t, size_t> > results;
  if (is_regex == true)
    {
      regex regex_1{ text };
      const sregex_token_iterator end;
      sregex_token_iterator iter;
      vector<string> regex_texts;
      while ((iter
              = regex_token_iterator (source.begin (), source.end (), regex_1))
             != end)
        {
          regex_texts.push_back (iter->str ());
        }
      size_t pos = 0;
      for (auto const &r_text : regex_texts)
        {
          pos = source.find (r_text, pos);
          pair res{ pos, r_text.size () };
          results.emplace_back (std::move (res));
        }
    }
  else
    {
      auto p_source = &source;
      auto p_text = &text;
      if (is_case == false)
        {
          auto nsource = source;
          auto ntext = text;
          transform (nsource.begin (), nsource.end (), nsource.begin (),
                     ::tolower);
          transform (ntext.begin (), ntext.end (), ntext.begin (), ::tolower);
          p_source = &nsource;
          p_text = &ntext;
        }

      auto pos = p_source->find (*p_text);
      while (pos != string::npos)
        {
          pair res{ pos, p_text->size () };
          results.emplace_back (std::move (res));
          pos = p_source->find (*p_text, pos + 1);
        }
    }

  return results;
}

}

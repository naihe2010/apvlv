/*
 * This file is part of the apvlv package
 * Copyright (C) <2008> Alf
 *
 * Contact: Alf <naihe2010@126.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */
/* @CPPFILE ApvlvCompletion.cc
 *
 *  Author: Alf <naihe2010@126.com>
 */

#include <QDebug>
#include <algorithm>
#include <filesystem>
#include <string>

#include "ApvlvCompletion.h"
#include "ApvlvUtil.h"

namespace apvlv
{
using namespace std;

string
ApvlvCompletion::complete (const string &prefix)
{
  auto iter = find_if (
      mItems.cbegin (), mItems.cend (),
      [prefix] (const string &item) { return item.find (prefix) == 0; });
  if (iter != mItems.cend ())
    return *iter;
  else
    return "";
}

void
ApvlvCompletion::addItems (const vector<string> &items)
{
  mItems.insert (mItems.end (), items.begin (), items.end ());
}

void
ApvlvCompletion::addPath (const string &path)
{
  vector<string> items;

  auto fspath = filesystem::path{ path };
  auto filename = fspath.filename ();
  auto dirname = fspath.parent_path ();
  for (auto &entry : filesystem::directory_iterator (dirname))
    {
      auto entry_filename = entry.path ().filename ().string ();
      if (filename.empty () || entry_filename.find (filename.string ()) == 0)
        {
          auto item = entry.path ().string ()
                      + (entry.is_directory () ? PATH_SEP_S : "");
          qDebug () << "add a item: " << item;
          items.emplace_back (item);
        }
    }

  addItems (items);
}

}

// Local Variables:
// mode: c++
// End:

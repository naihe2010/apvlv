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
/* @CPPFILE ApvlvEditor.h
 *
 *  Author: Alf <naihe2010@126.com>
 */

#ifndef _APVLV_NOTE_H_
#define _APVLV_NOTE_H_

#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_set>

#include "ApvlvEditor.h"

namespace apvlv
{
const float NoteScoreMin = 0.0f;
const float NoteScoreMax = 10.0f;

struct ApvlvPoint;
struct Location
{
  int page{ 0 };
  double x{ 0 };
  double y{ 0 };
  int offset{ -1 };
  std::string path;
  std::string anchor;

  friend bool
  operator< (const Location &a, const Location &b)
  {
    if (a.page != b.page)
      return a.page < b.page;
    if (a.y != b.y)
      return a.y < b.y;
    if (a.x != b.x)
      return a.x < b.x;
    if (a.offset != b.offset)
      return a.offset < b.offset;
    if (a.path != b.path)
      return a.path < b.path;
    return false;
  }

  friend std::istream &
  operator>> (std::istream &is, Location &location)
  {
    is >> location.page >> location.x >> location.y >> location.offset;

    std::string path;
    while (!is.eof ())
      {
        is >> path;
        if (path.empty ())
          {
            return is;
          }

        if (path.ends_with ("||"))
          {
            path = path.substr (0, path.length () - 2);
            location.path += path;
            break;
          }
        else
          {
            location.path += path;
          }
      }

    while (!is.eof ())
      {
        is >> path;
        if (path.empty ())
          {
            break;
          }
        location.anchor += path;
      }

    return is;
  }

  friend std::ostream &
  operator<< (std::ostream &os, const Location &location)
  {
    os << location.page << " " << location.x << " " << location.y << " "
       << location.offset;
    if (!location.path.empty ())
      {
        os << " " << location.path << "|| " << location.anchor << "||";
      }
    return os;
  }

  void set (int page1, const ApvlvPoint *point1, int offset1 = 0,
            const std::string &path1 = "", const std::string &anchor1 = "");
};

class File;
class Note;
struct Comment
{
  Comment ()
      : time{ std::chrono::system_clock::to_time_t (
            std::chrono::system_clock::now ()) }
  {
  }

  std::string quoteText;
  std::string commentText;
  Location begin;
  Location end;
  time_t time;

  friend std::istream &
  operator>> (std::istream &is, Comment &comment)
  {
    std::string line;
    getline (is, line);
    while (line.starts_with ("> "))
      {
        auto content = line.substr (2);
        content += '\n';
        comment.quoteText += content;
        getline (is, line);
      }

    if (line.starts_with ("```"))
      {
        do
          {
            getline (is, line);

            line += '\n';
            comment.commentText += line;
          }
        while (!line.starts_with ("```"));
      }

    getline (is, line);
    std::stringstream bs (line.substr (2));
    bs >> comment.begin;

    getline (is, line);
    std::stringstream es (line.substr (2));
    es >> comment.end;

    getline (is, line);
    std::stringstream ts (line.substr (2));
    std::tm tm;
    ts >> std::get_time (&tm, "%a %b %d %H:%M:%S %Y");
    comment.time = std::mktime (&tm);

    return is;
  }

  friend std::ostream &
  operator<< (std::ostream &os, const Comment &comment)
  {
    // put "> " at beginning of every line
    auto remain = comment.quoteText + '\n';
    auto pos = remain.find ('\n');
    while (pos != std::string::npos)
      {
        os << "> " << remain.substr (0, pos) << std::endl;
        remain = remain.substr (pos + 1);
        pos = remain.find ('\n');
      }

    os << "```" << std::endl;
    os << comment.commentText << std::endl;
    os << "```" << std::endl;

    os << "- " << comment.begin << std::endl;
    os << "- " << comment.end << std::endl;
    os << "- " << std::ctime (&comment.time) << std::endl;
    os << std::endl;
    return os;
  }
};

class Note : public QObject
{
  Q_OBJECT

public:
  explicit Note (File *file);
  ~Note () override;

  bool loadStreamV1 (std::ifstream &is);
  bool loadStream (std::ifstream &is);
  bool load (std::string_view path = "");

  bool dumpStream (std::ostream &os);
  bool dump (std::string_view path = "");

  void
  setScore (float score)
  {
    mScore = score;
    dump ();
  }

  float
  score ()
  {
    return mScore;
  }

  void
  addTag (const std::string &tag)
  {
    mTagSet.insert (tag);
    dump ();
  }

  void
  removeTag (const std::string &tag)
  {
    mTagSet.erase (tag);
    dump ();
  }

  const std::unordered_set<std::string> &
  tag ()
  {
    return mTagSet;
  }

  void
  setRemark (const std::string &remark)
  {
    mRemark = remark;
    dump ();
  }

  const std::string &
  remark ()
  {
    return mRemark;
  }

  void
  addReference (const std::string &ref)
  {
    mReferences.insert (ref);
    dump ();
  }

  void
  removeReference (const std::string &ref)
  {
    mReferences.erase (ref);
    dump ();
  }

  const std::unordered_set<std::string> &
  references ()
  {
    return mReferences;
  }

  void
  addLink (const std::string &link)
  {
    mLinks.insert (link);
    dump ();
  }

  void
  removeLink (const std::string &link)
  {
    mLinks.erase (link);
    dump ();
  }

  const std::unordered_set<std::string> &
  links ()
  {
    return mLinks;
  }

  void
  addComment (const Comment &comment)
  {
    mCommentList.insert ({ comment.begin, comment });
    dump ();
  }

  void
  removeComment (const Comment &comment)
  {
    mCommentList.erase (comment.begin);
    dump ();
  }

  std::vector<Comment>
  getCommentsInPage (int page)
  {
    std::vector<Comment> comments;
    std::ranges::for_each (
        mCommentList,
        [page, &comments] (const std::pair<Location, Comment> &pair1) {
          if (pair1.first.page == page)
            comments.push_back (pair1.second);
        });
    return comments;
  }

  std::vector<Comment>
  getCommentsInPath (const std::string &path)
  {
    std::vector<Comment> comments;
    std::ranges::for_each (
        mCommentList,
        [path, &comments] (const std::pair<Location, Comment> &pair1) {
          if (pair1.first.path == path)
            comments.push_back (pair1.second);
        });
    return comments;
  }

private:
  std::string notePathOfFile (File *file);

  std::string mPath;
  File *mFile{ nullptr };

  float mScore{ 0.0f };

  std::unordered_set<std::string> mTagSet;

  std::string mRemark;

  std::unordered_set<std::string> mReferences;
  std::unordered_set<std::string> mLinks;

  std::map<Location, Comment> mCommentList;
};

class CommentEdit : public Editor
{
  Q_OBJECT

public:
private:
  Comment *mComment;
};

class NoteEdit : public QFrame
{
  Q_OBJECT

public:
private:
  Note *mNote;
};
}

#endif

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
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

namespace apvlv
{
constexpr float NoteScoreMin = 0.0f;
constexpr float NoteScoreMax = 10.0f;

class MarkdownNode;
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

  void set (int page1, const ApvlvPoint *point1, int offset1 = 0,
            const std::string &path1 = "", const std::string &anchor1 = "");

  void fromMarkdownNode (MarkdownNode *node);
  void toMarkdownNode (MarkdownNode *node) const;
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

  void fromMarkdownNode (MarkdownNode *node);
  void toMarkdownNode (MarkdownNode *node) const;
};

class Note
{
public:
  Note () {}
  ~Note ();

  static std::string notePathOfFile (File *file);

  static std::string notePathOfPath (std::string_view sv);

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

  std::string
  tagString ()
  {
    if (mTagSet.empty ())
      {
        return "";
      }

    std::ostringstream oss;
    auto itr = mTagSet.begin ();
    oss << *itr;
    ++itr;
    while (itr != mTagSet.end ())
      {
        oss << "," << *itr;
        ++itr;
      }
    return oss.str ();
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
    for (const auto &pair1 : mCommentList)
      {
        if (pair1.first.page == page)
          comments.push_back (pair1.second);
      };
    return comments;
  }

  std::vector<Comment>
  getCommentsInPath (const std::string &path)
  {
    std::vector<Comment> comments;
    for (const auto &pair1 : mCommentList)
      {
        if (pair1.first.path == path)
          comments.push_back (pair1.second);
      }
    return comments;
  }

private:
  void loadV1Version (MarkdownNode *node);
  void loadV1MetaData (MarkdownNode *node);
  void loadV1Comments (MarkdownNode *node);
  void loadV1References (MarkdownNode *node);
  void loadV1Links (MarkdownNode *node);
  void appendV1Version (MarkdownNode *doc);
  void appendV1MetaData (MarkdownNode *doc);
  void appendV1Comments (MarkdownNode *doc);
  void appendV1References (MarkdownNode *doc);
  void appendV1Links (MarkdownNode *doc);

  std::string mPath;

  float mScore{ 0.0f };

  std::unordered_set<std::string> mTagSet;

  std::string mRemark;

  std::unordered_set<std::string> mReferences;
  std::unordered_set<std::string> mLinks;

  std::map<Location, Comment> mCommentList;
};

}

#endif

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
/* @CPPFILE ApvlvNote.cc
 *
 *  Author: Alf <naihe2010@126.com>
 */

#include <QDir>
#include <cmark.h>
#include <filesystem>
#include <fstream>

#include "ApvlvFile.h"
#include "ApvlvMarkdown.h"
#include "ApvlvNote.h"
#include "ApvlvUtil.h"

namespace apvlv
{
using namespace std;

void
Location::set (int page1, const ApvlvPoint *point1, int offset1,
               const std::string &path1, const std::string &anchor1)
{
  page = page1;
  x = point1->x;
  y = point1->y;
  offset = offset1;
  path = path1;
  anchor = anchor1;
}

void
Location::fromMarkdownNode (MarkdownNode *node)
{
  stringstream ss{ node->literal };
  ss >> page >> x >> y >> offset;
  if (!ss.eof ())
    {
      auto str = ss.str ();
      auto pos = str.find ("[[");
      auto end = str.find ("]]");
      path = str.substr (pos + 2, end - pos - 2);
      anchor = str.substr (end + 2);
    }
}

void
Location::toMarkdownNode (MarkdownNode *node) const
{
  stringstream ss;
  ss << page << " " << x << " " << y << " " << offset;
  if (!path.empty ())
    {
      ss << " [[" << path << "]]#" << anchor;
    }
  node->node_type = CMARK_NODE_TEXT;
  node->literal = ss.str ();
}

void
Comment::fromMarkdownNode (MarkdownNode *node)
{
  auto qn = node->childAt (0);
  auto qp = qn->childAt (0);
  auto qt = qp->childAt (0);
  quoteText = qt->literal;

  auto cn = node->childAt (1);
  commentText = cn->literal;

  auto list = node->childAt (2);

  auto i = list->childAt (0);
  auto p = i->childAt (0);
  auto t = p->childAt (0);
  begin.fromMarkdownNode (t);

  i = list->childAt (1);
  p = i->childAt (0);
  t = p->childAt (0);
  end.fromMarkdownNode (t);

  i = list->childAt (2);
  p = i->childAt (0);
  t = p->childAt (0);
  struct tm tm{};
  strptime (t->literal.c_str (), "%a %b %d %H:%M:%S %Y", &tm);
  time = std::mktime (&tm);
}

void
Comment::toMarkdownNode (MarkdownNode *node) const
{
  auto qn = MarkdownNode::create (node, CMARK_NODE_BLOCK_QUOTE);
  auto qp = MarkdownNode::create (qn, CMARK_NODE_PARAGRAPH);
  MarkdownNode::create (qp, CMARK_NODE_TEXT, quoteText);

  MarkdownNode::create (node, CMARK_NODE_CODE_BLOCK, commentText);

  auto list = MarkdownNode::create (node, CMARK_NODE_LIST);
  list->list_type = CMARK_NO_LIST;

  auto i = MarkdownNode::create (list, CMARK_NODE_ITEM);
  auto p = MarkdownNode::create (i, CMARK_NODE_PARAGRAPH);
  auto t = MarkdownNode::create (p, CMARK_NODE_TEXT);
  begin.toMarkdownNode (t);

  i = MarkdownNode::create (list, CMARK_NODE_ITEM);
  p = MarkdownNode::create (i, CMARK_NODE_PARAGRAPH);
  t = MarkdownNode::create (p, CMARK_NODE_TEXT);
  end.toMarkdownNode (t);

  i = MarkdownNode::create (list, CMARK_NODE_ITEM);
  p = MarkdownNode::create (i, CMARK_NODE_PARAGRAPH);
  t = MarkdownNode::create (p, CMARK_NODE_TEXT);
  t->literal = std::ctime (&time);
}

Note::Note (File *file) : mFile (file) {}

Note::~Note () = default;

bool
Note::loadStreamV1 (std::ifstream &is)
{
  auto doc = Markdown::create ();
  doc->loadFromStream (is);
  if (!doc)
    {
      return false;
    }

  auto node = doc->root ();
  auto n = node->childAt (0);
  auto list = node->childAt (1);
  loadV1Version (list);

  for (auto i = 2; i < node->childrenCount (); i++)
    {
      n = node->childAt (i);
      if (n->node_type == CMARK_NODE_THEMATIC_BREAK)
        {
          continue;
        }

      if (i >= node->childrenCount () - 1)
        break;

      list = node->childAt (i + 1);
      i++;
      auto res = n->headText ();
      if (res.second == "Meta Data")
        {
          loadV1MetaData (list);
        }
      else if (res.second == "Comments")
        {
          loadV1Comments (list);
        }
      else if (res.second == "References")
        {
          loadV1References (list);
        }
      else if (res.second == "Links")
        {
          loadV1Links (list);
        }
      else
        {
          qDebug () << "Unknown head \"" << res.second << "\"";
        }
    }

  return true;
}

bool
Note::loadStream (std::ifstream &is)
{
  string line;

  getline (is, line);
  if (!line.starts_with ("---"))
    {
      qWarning () << "note header not found";
      return false;
    }

  string version;
  is >> version >> version;
  if (version == "1")
    {
      is.seekg (0, ios::beg);
      return loadStreamV1 (is);
    }

  return false;
}

bool
Note::load (std::string_view sv)
{
  string path = string (sv);
  if (path.empty ())
    path = notePathOfFile (mFile);

  ifstream ifs{ path };
  if (!ifs.is_open ())
    return false;

  auto ret = loadStream (ifs);
  ifs.close ();
  return ret;
}

void
Note::loadV1Version (MarkdownNode *node)
{
  for (auto i = 0; i < node->childrenCount (); i++)
    {
      auto n = node->childAt (i);
      if (n->node_type == CMARK_NODE_SOFTBREAK)
        continue;

      auto s = QString::fromLocal8Bit (n->literal);
      auto vs = s.split (":");
      if (vs.size () == 2)
        {
          if (vs[0].trimmed () == "version")
            {
              auto version = vs[1].trimmed ();
              Q_ASSERT (version == "1");
            }
        }
    }
}

void
Note::loadV1MetaData (MarkdownNode *node)
{
  auto texts = node->getListTexts ();
  for (const auto &text : texts)
    {
      auto tokens = QString::fromLocal8Bit (text).split (":");
      if (tokens.size () == 2)
        {
          auto k = tokens[0].trimmed ();
          auto v = tokens[1].trimmed ();
          if (k == "tag")
            {
              auto ts = v.split (",");
              for (auto const &t : ts)
                {
                  auto tag = t.trimmed ();
                  if (!tag.isEmpty ())
                    {
                      qDebug () << "got tag: " << tag;
                      mTagSet.insert (tag.toStdString ());
                    }
                }
            }
          else if (k == "score")
            {
              mScore = v.toFloat ();
            }
        }
    }
}

void
Note::loadV1Comments (MarkdownNode *node)
{
  for (auto i = 0; i < node->childrenCount (); i++)
    {
      auto ni = node->childAt (i);
      auto comment = Comment{};
      comment.fromMarkdownNode (ni);
      addComment (comment);
    }
}

void
Note::loadV1References (MarkdownNode *node)
{
  auto texts = node->getListTexts ();
  for (const auto &text : texts)
    {
      if (!text.empty ())
        {
          mReferences.insert (text);
        }
    }
}

void
Note::loadV1Links (MarkdownNode *node)
{
  auto texts = node->getListTexts ();
  for (const auto &text : texts)
    {
      if (!text.empty ())
        {
          mLinks.insert (text);
        }
    }
}

void
Note::appendV1Version (MarkdownNode *doc)
{
  MarkdownNode::create (doc, CMARK_NODE_THEMATIC_BREAK);

  auto g = MarkdownNode::create (doc, CMARK_NODE_PARAGRAPH);
  MarkdownNode::create (g, CMARK_NODE_TEXT, "version: 1");
  MarkdownNode::create (g, CMARK_NODE_LINEBREAK);
  MarkdownNode::create (g, CMARK_NODE_TEXT, "path: " + mPath);

  MarkdownNode::create (doc, CMARK_NODE_THEMATIC_BREAK);
}

void
Note::appendV1MetaData (MarkdownNode *doc)
{
  MarkdownNode::HeadAndList headAndList;
  headAndList.first = "Meta Data";
  string tags;
  for (const auto &cp : mTagSet)
    {
      tags += cp + ",";
    }
  headAndList.second.push_back ("score: "
                                + QString::number (mScore).toStdString ());
  headAndList.second.push_back ("tag: " + tags);
  doc->appendHeadAndBulletList (1, headAndList);
}

void
Note::appendV1Comments (MarkdownNode *doc)
{
  doc->appendHeadText (1, "Comments");

  auto list = MarkdownNode::create (doc, CMARK_NODE_LIST);
  list->list_type = CMARK_ORDERED_LIST;
  for (auto const &loc_comment : mCommentList)
    {
      auto comment = loc_comment.second;
      auto n = MarkdownNode::create (list, CMARK_NODE_ITEM);
      comment.toMarkdownNode (n);
    }
}

void
Note::appendV1References (MarkdownNode *doc)
{
  MarkdownNode::HeadAndList headAndList;
  headAndList.first = "References";
  for (auto const &r : mReferences)
    {
      headAndList.second.push_back (r);
    }
  doc->appendHeadAndBulletList (1, headAndList);
}

void
Note::appendV1Links (MarkdownNode *doc)
{
  MarkdownNode::HeadAndList headAndList;
  headAndList.first = "Links";
  for (auto const &r : mLinks)
    {
      headAndList.second.push_back (r);
    }
  doc->appendHeadAndBulletList (1, headAndList);
}

bool
Note::dumpStream (std::ostream &os)
{
  /* handmade version is ugly */
#if 0
  os << "---" << endl;
  os << "version: 1" << endl;
  os << "path: " << mFile->getFilename () << endl;

  os << "---" << endl;
  os << "# Meta Data" << endl;
  os << "- tag: ";
  std::ranges::for_each (mTagSet,
                         [&os] (const string &tag) { os << tag << ","; });
  os << endl;
  os << "- score: " << mScore << endl;
  os << endl;

  os << "# Comments" << endl;
  auto index = 0;
  std::ranges::for_each (mCommentList,
                         [&os, &index] (const pair<Location, Comment> &pair1) {
                           os << " - " << index++ << endl;
                           os << pair1.second;
                         });
  os << endl;

  os << "# References" << endl;
  std::ranges::for_each (
      mReferences, [&os] (const string &ref) { os << "- " << ref << endl; });
  os << endl;

  os << "# Links" << endl;
  std::ranges::for_each (
      mLinks, [&os] (const string &link) { os << "- " << link << endl; });
  os << endl;

#else
  auto doc = Markdown::create ();
  auto node = doc->root ();

  appendV1Version (node);

  appendV1MetaData (node);

  appendV1Comments (node);

  appendV1References (node);

  appendV1Links (node);

  doc->saveToStream (os);
#endif

  return true;
}

bool
Note::dump (std::string_view sv)
{
  if (mFile == nullptr)
    return false;

  string path = string (sv);
  if (path.empty ())
    path = notePathOfFile (mFile);

  auto fspath = filesystem::path (path).parent_path ();
  std::error_code code;
  filesystem::create_directories (fspath, code);

  ofstream ofs{ path };
  if (!ofs.is_open ())
    return false;

  auto ret = dumpStream (ofs);
  ofs.close ();
  return ret;
}

std::string
Note::notePathOfFile (File *file)
{
  auto homedir = QDir::home ().filesystemAbsolutePath ().string ();
  auto filename = file->getFilename ();
  if (filename.find (homedir) == 0)
    filename = filename.substr (homedir.size () + 1);
  if (filename[0] == filesystem::path::preferred_separator)
    filename = filename.substr (1);
  if (filename[1] == ':')
    filename[1] = '-';
  auto path = NotesDir + filesystem::path::preferred_separator + filename;
  return path + ".md";
}
}

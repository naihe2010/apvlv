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
/* @CPPFILE ApvlvMarkdown.cc
 *
 *  Author: Alf <naihe2010@126.com>
 */

#include <algorithm>
#include <array>
#include <fstream>

#include "ApvlvMarkdown.h"

#include <qassert.h>

namespace apvlv
{
using namespace std;

MarkdownNode::MarkdownNode (const MarkdownNode &other)
{
  node_type = other.node_type;
  list_type = other.list_type;
  heading_level = other.heading_level;
  literal = other.literal;
  title = other.title;
  url = other.url;
  for (auto &child : other.children)
    {
      auto nchild = new MarkdownNode (*child);
      children.push_back (unique_ptr<MarkdownNode> (nchild));
    }
}

MarkdownNode::MarkdownNode (MarkdownNode &&other) noexcept
{
  node_type = other.node_type;
  list_type = other.list_type;
  heading_level = other.heading_level;
  literal.swap (other.literal);
  title.swap (other.title);
  url.swap (other.url);
  children.swap (other.children);
}

MarkdownNode &
MarkdownNode::operator= (const MarkdownNode &other)
{
  node_type = other.node_type;
  list_type = other.list_type;
  heading_level = other.heading_level;
  literal = other.literal;
  title = other.title;
  url = other.url;
  for (const auto &child : other.children)
    {
      auto nchild = new MarkdownNode (*child);
      children.push_back (unique_ptr<MarkdownNode> (nchild));
    }
  return *this;
}

MarkdownNode &
MarkdownNode::operator= (MarkdownNode &&other) noexcept
{
  node_type = other.node_type;
  list_type = other.list_type;
  heading_level = other.heading_level;
  literal.swap (other.literal);
  title.swap (other.title);
  url.swap (other.url);
  children.swap (other.children);
  return *this;
}

int
MarkdownNode::childrenCount ()
{
  return static_cast<int> (children.size ());
}

MarkdownNode *
MarkdownNode::childAt (int index)
{
  return children[index].get ();
}

void
MarkdownNode::appendChildPtr (std::unique_ptr<MarkdownNode> ptr)
{
  children.push_back (std::move (ptr));
}

void
MarkdownNode::appendChild (MarkdownNode *n)
{
  children.push_back (unique_ptr<MarkdownNode> (n));
}

void
MarkdownNode::removeChild (MarkdownNode *n)
{
  auto iter = std::find_if (children.begin (), children.end (),
                            [n] (const auto &i) { return i.get () == n; });
  if (iter != children.end ())
    {
      children.erase (iter);
    }
}

std::vector<std::string>
MarkdownNode::getListTexts ()
{
  std::vector<std::string> texts;
  for (const auto &i : children)
    {
      auto p = i->children.front ().get ();
      auto t = p->children.front ().get ();
      texts.push_back (t->literal);
    }
  return texts;
}

void
MarkdownNode::setListTexts (cmark_list_type _type,
                            const std::vector<std::string> &texts)
{
  children.clear ();
  node_type = CMARK_NODE_LIST;
  list_type = _type;
  for (const auto &text : texts)
    {
      auto i = create (this, CMARK_NODE_ITEM);
      auto p = create (i, CMARK_NODE_PARAGRAPH);
      auto t = create (p, CMARK_NODE_TEXT);
      t->literal = text;
    }
}

void
MarkdownNode::setNoListTexts (const std::vector<std::string> &texts)
{
  setListTexts (CMARK_NO_LIST, texts);
}

void
MarkdownNode::setBulletListTexts (const std::vector<std::string> &texts)
{
  setListTexts (CMARK_BULLET_LIST, texts);
}

void
MarkdownNode::setOrderedListTexts (const std::vector<std::string> &texts)
{
  setListTexts (CMARK_ORDERED_LIST, texts);
}

std::pair<int, std::string>
MarkdownNode::headText ()
{
  if (node_type != CMARK_NODE_HEADING || childrenCount () < 1)
    {
      throw std::invalid_argument ("MarkdownNode::headText");
    }

  auto n = childAt (0);
  std::pair<int, std::string> res;
  res.first = n->heading_level;
  res.second = n->literal;
  return res;
}

void
MarkdownNode::appendHeadText (int _level, const std::string &_text)
{
  auto h = create (this, CMARK_NODE_HEADING);
  h->heading_level = _level;
  auto ht = create (h, CMARK_NODE_TEXT);
  ht->literal = _text;
}

void
MarkdownNode::appendHeadAndList (int _level, cmark_list_type _type,
                                 const HeadAndList &headAndList)
{
  appendHeadText (_level, headAndList.first);
  auto list = create (this, CMARK_NODE_LIST);
  list->setListTexts (_type, headAndList.second);
}

void
MarkdownNode::appendHeadAndNoList (int _level, const HeadAndList &headAndList)
{
  appendHeadAndList (_level, CMARK_NO_LIST, headAndList);
}

void
MarkdownNode::appendHeadAndBulletList (int _level,
                                       const HeadAndList &headAndList)
{
  appendHeadAndList (_level, CMARK_BULLET_LIST, headAndList);
}

void
MarkdownNode::appendHeadAndOrderedList (int _level,
                                        const HeadAndList &headAndList)
{
  appendHeadAndList (_level, CMARK_ORDERED_LIST, headAndList);
}

std::unique_ptr<MarkdownNode>
MarkdownNode::fromCmarkNode (cmark_node *node)
{
  auto mn = make_unique<MarkdownNode> (cmark_node_get_type (node));
  switch (mn->node_type)
    {
    case CMARK_NODE_HEADING:
      mn->heading_level = cmark_node_get_heading_level (node);
      break;
    case CMARK_NODE_LIST:
      mn->list_type = cmark_node_get_list_type (node);
      break;
    case CMARK_NODE_IMAGE:
    case CMARK_NODE_LINK:
      mn->title = cmark_node_get_title (node);
      mn->url = cmark_node_get_url (node);
      break;
    default:
      auto l = cmark_node_get_literal (node);
      if (l)
        mn->literal = l;
    }
  for (auto n = cmark_node_first_child (node); n != nullptr;
       n = cmark_node_next (n))
    {
      auto cmn = MarkdownNode::fromCmarkNode (n);
      mn->children.emplace_back (std::move (cmn));
    }
  return mn;
}

MarkdownNode *
MarkdownNode::create (MarkdownNode *parent, cmark_node_type _type,
                      string_view literal)
{
  Q_ASSERT (parent != nullptr);
  auto mn = make_unique<MarkdownNode> (_type);
  mn->literal = literal;
  auto node = mn.get ();
  parent->appendChildPtr (std::move (mn));
  return node;
}

cmark_node *
MarkdownNode::toCmarkNode ()
{
  auto doc = cmark_node_new (node_type);
  switch (node_type)
    {
    case CMARK_NODE_HEADING:
      cmark_node_set_heading_level (doc, heading_level);
      break;

    case CMARK_NODE_LIST:
      cmark_node_set_list_type (doc, list_type);
      break;

    case CMARK_NODE_IMAGE:
    case CMARK_NODE_LINK:
      cmark_node_set_title (doc, title.c_str ());
      cmark_node_set_url (doc, url.c_str ());
      break;

    default:
      break;
    }
  cmark_node_set_literal (doc, literal.c_str ());

  for (auto const &child : children)
    {
      auto n = child.get ();
      auto d = n->toCmarkNode ();
      cmark_node_append_child (doc, d);
    }

  return doc;
}

Markdown::Markdown ()
    : mRoot{ make_unique<MarkdownNode> (CMARK_NODE_DOCUMENT) }
{
  mRoot->node_type = CMARK_NODE_DOCUMENT;
}

Markdown::Markdown (const Markdown &other)
{
  if (this != &other)
    {
      mRoot = make_unique<MarkdownNode> (MarkdownNode (*other.mRoot));
    }
}

Markdown::Markdown (Markdown &&other) noexcept
{
  if (this != &other)
    {
      mRoot.swap (other.mRoot);
    }
}

Markdown &
Markdown::operator= (const Markdown &other)
{
  if (this != &other)
    {
      mRoot = make_unique<MarkdownNode> (MarkdownNode (*other.mRoot));
    }

  return *this;
}

Markdown &
Markdown::operator= (Markdown &&other) noexcept
{
  if (this != &other)
    {
      mRoot.swap (other.mRoot);
    }
  return *this;
}

bool
Markdown::loadFromFile (const std::string &filename)
{
  ifstream ifs (filename, ifstream::binary);
  if (!ifs.is_open ())
    {
      return false;
    }

  return loadFromStream (ifs);
}

bool
Markdown::loadFromStream (std::istream &is)
{
  auto parser = cmark_parser_new (CMARK_OPT_DEFAULT);
  array<char, 16384> buffer{};
  while (is.good ())
    {
      is.read (buffer.data (), buffer.size ());
      auto got = is.gcount ();
      if (got == 0)
        {
          break;
        }
      cmark_parser_feed (parser, buffer.data (), got);
    }

  auto doc = cmark_parser_finish (parser);
  cmark_parser_free (parser);

  if (doc)
    {
      mRoot = MarkdownNode::fromCmarkNode (doc);
      cmark_node_free (doc);
      return true;
    }
  else
    {
      return false;
    }
}

bool
Markdown::saveToFile (const std::string &filename)
{
  ofstream ofs (filename, ofstream::binary);
  if (!ofs.is_open ())
    {
      return false;
    }

  return saveToStream (ofs);
}

bool
Markdown::saveToStream (std::ostream &os)
{
  auto doc = mRoot->toCmarkNode ();
  auto text = cmark_render_commonmark (doc, CMARK_OPT_DEFAULT, 78);
  os << text;
  free (text);
  cmark_node_free (doc);
  return false;
}

MarkdownNode *
Markdown::root () const
{
  return mRoot.get ();
}

}

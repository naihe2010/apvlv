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

#ifndef _APVLV_MARKDOWN_H_
#define _APVLV_MARKDOWN_H_

#include <cmark.h>
#include <list>
#include <memory>
#include <queue>
#include <string>

namespace apvlv
{
struct MarkdownNode
{
  cmark_node_type node_type{ CMARK_NODE_TEXT };
  cmark_list_type list_type{ CMARK_NO_LIST };
  int heading_level{ 1 };
  std::string literal;
  std::string title;
  std::string url;
  std::vector<std::unique_ptr<MarkdownNode>> children;

  explicit MarkdownNode (cmark_node_type _type) : node_type (_type) {};
  ~MarkdownNode () = default;
  MarkdownNode (const MarkdownNode &other);
  MarkdownNode (MarkdownNode &&other) noexcept;
  MarkdownNode &operator= (const MarkdownNode &other);
  MarkdownNode &operator= (MarkdownNode &&other) noexcept;

  int childrenCount ();
  MarkdownNode *childAt (int index);
  void appendChildPtr (std::unique_ptr<MarkdownNode> ptr);
  void appendChild (MarkdownNode *n);
  void removeChild (MarkdownNode *n);

  std::vector<std::string> getListTexts ();
  void setListTexts (cmark_list_type _type,
                     const std::vector<std::string> &texts);
  void setNoListTexts (const std::vector<std::string> &texts);
  void setBulletListTexts (const std::vector<std::string> &texts);
  void setOrderedListTexts (const std::vector<std::string> &texts);

  using HeadAndList = std::pair<std::string, std::vector<std::string>>;
  std::pair<int, std::string> headText ();
  void appendHeadText (int _level, const std::string &_text);
  void appendHeadAndList (int _level, cmark_list_type _type,
                          const HeadAndList &headAndList);
  void appendHeadAndNoList (int _level, const HeadAndList &headAndList);
  void appendHeadAndBulletList (int _level, const HeadAndList &headAndList);
  void appendHeadAndOrderedList (int _level, const HeadAndList &headAndList);

  static std::unique_ptr<MarkdownNode> fromCmarkNode (cmark_node *node);
  static MarkdownNode *create (MarkdownNode *parent, cmark_node_type _type,
                               std::string_view literal = "");
  cmark_node *toCmarkNode ();
};

class Markdown
{
public:
  Markdown ();
  ~Markdown () = default;
  Markdown (const Markdown &other);
  Markdown (Markdown &&other) noexcept;
  Markdown &operator= (const Markdown &other);
  Markdown &operator= (Markdown &&other) noexcept;

  bool loadFromFile (const std::string &filename);
  bool loadFromStream (std::istream &is);
  bool saveToFile (const std::string &filename);
  bool saveToStream (std::ostream &os);
  [[nodiscard]] MarkdownNode *root () const;
  static std::unique_ptr<Markdown>
  create ()
  {
    return std::make_unique<Markdown> ();
  }

private:
  std::unique_ptr<MarkdownNode> mRoot;
};

}

#endif

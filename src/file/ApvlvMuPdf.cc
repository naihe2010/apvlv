/*
 * This file is part of the apvlv package
 * Copyright (C) <2010>  <Alf>
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
/* @CPPFILE ApvlvPdf.cc
 *
 *  Author: Alf <naihe2010@126.com>
 */

#include <QMessageBox>
#include <filesystem>
#include <fstream>
#include <mupdf/fitz.h>

#include "ApvlvMuPdf.h"

namespace apvlv
{
using namespace std;

FILE_TYPE_DEFINITION ("MuPDF", ApvlvMuPDF,
                      { ".pdf", ".xps", ".epub", ".mobi", ".fb2", ".cbz",
                        ".svg", ".txt" });

ApvlvMuPDF::ApvlvMuPDF () : mDoc{ nullptr }
{
  mContext = fz_new_context (nullptr, nullptr, FZ_STORE_UNLIMITED);
  fz_register_document_handlers (mContext);
}

ApvlvMuPDF::~ApvlvMuPDF ()
{
  fz_drop_document (mContext, mDoc);
  fz_drop_context (mContext);
}

bool
ApvlvMuPDF::load (const string &filename)
{
  mDoc = fz_open_document (mContext, filename.c_str ());
  if (mDoc == nullptr)
    {
      return false;
    }

  generateIndex ();
  return true;
}

SizeF
ApvlvMuPDF::pageSizeF (int pn, int rot)
{
  auto page = fz_load_page (mContext, mDoc, pn);
  auto rect = fz_bound_page (mContext, page);
  fz_drop_page (mContext, page);
  SizeF sizef{ rect.x1 - rect.x0, rect.y1 - rect.y0 };
  return sizef;
}

int
ApvlvMuPDF::sum ()
{
  auto pages = fz_count_pages (mContext, mDoc);
  return pages;
}

bool
ApvlvMuPDF::pageIsOnlyImage (int pn)
{
  auto text_page
      = fz_new_stext_page_from_page_number (mContext, mDoc, pn, nullptr);
  if (text_page == nullptr)
    {
      return true;
    }

  auto only_image = (text_page->first_block == nullptr);

  fz_drop_stext_page (mContext, text_page);
  return only_image;
}

bool
ApvlvMuPDF::pageRenderToImage (int pn, double zm, int rot, QImage *pix)
{
  auto scale = fz_scale (static_cast<float> (zm), static_cast<float> (zm));
  auto mat = fz_pre_rotate (scale, static_cast<float> (rot));
  auto color = fz_device_rgb (mContext);
  auto pixmap
      = fz_new_pixmap_from_page_number (mContext, mDoc, pn, mat, color, 0);

  QImage img{ pixmap->w, pixmap->h, QImage::Format_RGB32 };
  for (auto y = 0; y < pixmap->h; ++y)
    {
      auto p = pixmap->samples + y * pixmap->stride;
      for (auto x = 0; x < pixmap->w; ++x)
        {
          QColor c{ int (p[0]), int (p[1]), int (p[2]) };
          img.setPixelColor (x, y, c);
          p += pixmap->n;
        }
    }

  fz_drop_pixmap (mContext, pixmap);

  *pix = img;
  return true;
}

optional<vector<Rectangle>>
ApvlvMuPDF::pageHighlight (int pn, const ApvlvPoint &pa, const ApvlvPoint &pb)
{
  auto options = fz_stext_options{};
  auto text_page
      = fz_new_stext_page_from_page_number (mContext, mDoc, pn, &options);
  auto fa = fz_point{ static_cast<float> (pa.x), static_cast<float> (pa.y) };
  auto fb = fz_point{ static_cast<float> (pb.x), static_cast<float> (pb.y) };
  fz_quad quad_array[1024];
  auto quads
      = fz_highlight_selection (mContext, text_page, fa, fb, quad_array, 1024);
  fz_drop_stext_page (mContext, text_page);
  if (quads == 0)
    return nullopt;

  auto rect_list = vector<Rectangle>{};
  for (auto i = 0; i < quads; ++i)
    {
      auto quad = quad_array + i;
      Rectangle r{ quad->ul.x, quad->ul.y, quad->lr.x, quad->lr.y };
      rect_list.emplace_back (r);
    }
  return rect_list;
}

bool
ApvlvMuPDF::pageText (int pn, const Rectangle &rect, string &text)
{
  auto text_page
      = fz_new_stext_page_from_page_number (mContext, mDoc, pn, nullptr);
  auto fzrect = fz_rect{
    .x0 = static_cast<float> (rect.p1x),
    .y0 = static_cast<float> (rect.p1y),
    .x1 = static_cast<float> (rect.p2x),
    .y1 = static_cast<float> (rect.p2y),
  };
  text = fz_copy_rectangle (mContext, text_page, fzrect, 0);
  fz_drop_stext_page (mContext, text_page);
  return true;
}

unique_ptr<WordListRectangle>
ApvlvMuPDF::pageSearch (int pn, const char *str)
{
  int hit;
  fz_quad quad_array[1024];
  auto count = fz_search_page_number (mContext, mDoc, pn, str, &hit,
                                      quad_array, 1024);
  if (count == 0)
    return nullptr;

  auto list = make_unique<WordListRectangle> ();
  for (auto i = 0; i < count; ++i)
    {
      WordRectangle rectangle;
      rectangle.word = str;
      auto quad = quad_array[i];
      Rectangle rect{ quad.ul.x, quad.lr.y, quad.lr.x, quad.ul.y };
      rectangle.rect_list.push_back (rect);
      list->push_back (rectangle);
    }
  return list;
}

void
ApvlvMuPDF::generateIndex ()
{
  mIndex = { "", 0, "", FileIndexType::FILE };

  fz_outline *top_toc;
  fz_try (mContext) top_toc = fz_load_outline (mContext, mDoc);
  fz_catch (mContext)
  {
    qCritical () << "load " << mFilename << " outline error";
    fz_report_error (mContext);
    top_toc = nullptr;
  }

  if (top_toc == nullptr)
    return;

  auto toc = top_toc;
  while (toc != nullptr)
    {
      auto child_index = FileIndex{};
      generateIndexRecursively (child_index, toc);
      mIndex.mChildrenIndex.push_back (child_index);
      toc = toc->next;
    }

  fz_drop_outline (mContext, top_toc);
}

void
ApvlvMuPDF::generateIndexRecursively (FileIndex &index,
                                      const fz_outline *outline)
{
  index.type = FileIndexType::PAGE;
  if (outline->title)
    index.title = outline->title;
  index.page = fz_page_number_from_location (mContext, mDoc, outline->page);
  if (outline->uri != nullptr)
    {
      index.path = outline->uri;
      auto pos = index.path.find ('#');
      if (pos != string::npos)
        {
          index.anchor = index.path.substr (pos);
          index.path = index.path.substr (0, pos);
        }
      if (index.page == -1)
        {
          auto dest = fz_resolve_link (mContext, mDoc, outline->uri, nullptr,
                                       nullptr);
          index.page = fz_page_number_from_location (mContext, mDoc, dest);
        }
    }

  auto toc = outline->down;
  while (toc != nullptr)
    {
      auto child_index = FileIndex{};
      generateIndexRecursively (child_index, toc);
      index.mChildrenIndex.push_back (child_index);
      toc = toc->next;
    }
}
}

// Local Variables:
// mode: c++
// End:

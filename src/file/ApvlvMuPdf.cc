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

#include <QInputDialog>
#include <QMessageBox>
#include <filesystem>
#include <fstream>
#include <mupdf/classes.h>
#include <mupdf/classes2.h>

#include "ApvlvMuPdf.h"

namespace apvlv
{
using namespace std;
using namespace mupdf;

FILE_TYPE_DEFINITION (ApvlvPDF, { ".pdf", ".xps", ".epub", ".mobi", ".fb2",
                                  ".cbz", ".svg", ".txt" });

bool
ApvlvPDF::load (const string &filename)
{
  mDoc = make_unique<FzDocument> (filename.c_str ());
  if (mDoc == nullptr)
    {
      return false;
    }

  generateIndex ();
  return true;
}

SizeF
ApvlvPDF::pageSizeF (int pn, int rot)
{
  auto page = mDoc->fz_load_page (pn);
  auto rect = page.fz_bound_page ();
  SizeF sizef{ rect.x1 - rect.x0, rect.y1 - rect.y0 };
  return sizef;
}

int
ApvlvPDF::sum ()
{
  return mDoc->fz_count_pages ();
}

bool
ApvlvPDF::pageRenderToImage (int pn, double zm, int rot, QImage *pix)
{
  auto matrix
      = FzMatrix::fz_scale (static_cast<float> (zm), static_cast<float> (zm));
  matrix = fz_pre_rotate (matrix, static_cast<float> (rot));
  auto pixmap
      = mDoc->fz_new_pixmap_from_page_number (pn, matrix, fz_device_rgb (), 0);
  QImage img{ pixmap.w (), pixmap.h (), QImage::Format_RGB32 };
  for (auto y = 0; y < pixmap.h (); ++y)
    {
      auto p = pixmap.samples () + y * pixmap.stride ();
      for (auto x = 0; x < pixmap.w (); ++x)
        {
          QColor c{ int (p[0]), int (p[1]), int (p[2]) };
          img.setPixelColor (x, y, c);
          p += pixmap.n ();
        }
    }

  *pix = img;
  return true;
}

optional<vector<Rectangle> >
ApvlvPDF::pageHighlight (int pn, const ApvlvPoint &pa, const ApvlvPoint &pb)
{
  auto options = FzStextOptions{};
  auto text_page = FzStextPage (*mDoc, pn, options);
  auto fa = FzPoint (static_cast<float> (pa.x), static_cast<float> (pa.y));
  auto fb = FzPoint (static_cast<float> (pb.x), static_cast<float> (pb.y));
  auto quads = text_page.fz_highlight_selection2 (fa, fb, 1024);
  if (quads.empty ())
    return nullopt;

  auto rect_list = vector<Rectangle>{};
  for (auto const &quad : quads)
    {
      Rectangle r{ quad.ul.x, quad.ul.y, quad.lr.x, quad.lr.y };
      rect_list.emplace_back (r);
    }
  return rect_list;
}

bool
ApvlvPDF::pageText (int pn, const Rectangle &rect, string &text)
{
  auto options = FzStextOptions{};
  auto text_page = FzStextPage (*mDoc, pn, options);
  auto fzrect = FzRect (rect.p1x, rect.p1y, rect.p2x, rect.p2y);
  text = text_page.fz_copy_rectangle (fzrect, 0);
  return true;
}

unique_ptr<WordListRectangle>
ApvlvPDF::pageSearch (int pn, const char *str)
{
  auto results = mDoc->fz_search_page2 (pn, str, 1024);
  if (results.empty ())
    return nullptr;

  auto list = make_unique<WordListRectangle> ();
  for (auto const &res : results)
    {
      WordRectangle rectangle;
      rectangle.word = str;
      Rectangle rect{ res.quad.ul.x, res.quad.lr.y, res.quad.lr.x,
                      res.quad.ul.y };
      rectangle.rect_list.push_back (rect);
      list->push_back (rectangle);
    }
  return list;
}

void
ApvlvPDF::generateIndex ()
{
  mIndex = { "", 0, "", FileIndexType::FILE };
  auto toc = mDoc->fz_load_outline ();
  while (toc.m_internal != nullptr)
    {
      auto child_index = FileIndex{};
      generateIndexRecursively (child_index, toc);
      mIndex.mChildrenIndex.push_back (child_index);
      toc = toc.next ();
    }
}

void
ApvlvPDF::generateIndexRecursively (FileIndex &index,
                                    mupdf::FzOutline &outline)
{
  index.type = FileIndexType::PAGE;
  index.title = outline.title ();
  index.page = mDoc->fz_page_number_from_location (outline.page ());
  if (outline.m_internal->uri != nullptr)
    {
      index.path = outline.uri ();
      auto pos = index.path.find ('#');
      if (pos != string::npos)
        {
          index.anchor = index.path.substr (pos);
          index.path = index.path.substr (0, pos);
        }
      if (index.page == -1)
        {
          auto dest = mDoc->fz_resolve_link (outline.uri (), nullptr, nullptr);
          index.page = mDoc->fz_page_number_from_location (dest);
        }
    }

  auto toc = outline.down ();
  while (toc.m_internal != nullptr)
    {
      auto child_index = FileIndex{};
      generateIndexRecursively (child_index, toc);
      index.mChildrenIndex.push_back (child_index);
      toc = toc.next ();
    }
}

}

// Local Variables:
// mode: c++
// End:

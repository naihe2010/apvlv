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
#include <qt6/poppler-qt6.h>

#include "ApvlvPopplerPdf.h"
#include "ApvlvView.h"

namespace apvlv
{
using namespace std;

FILE_TYPE_DEFINITION (ApvlvPDF, { ".pdf" });

ApvlvPDF::ApvlvPDF (const string &filename, bool check)
    : File (filename, check)
{
  mDoc = Document::load (QString::fromLocal8Bit (filename));
  if (mDoc == nullptr)
    {
      auto text
          = QInputDialog::getText (nullptr, "password", "input password");
      auto pass = QByteArray::fromStdString (text.toStdString ());
      mDoc = Document::load (QString::fromStdString (filename), pass, pass);
    }

  if (mDoc == nullptr)
    {
      throw std::bad_alloc ();
    }

  pdf_get_index ();
}

bool
ApvlvPDF::pageSize (int pn, int rot, double *x, double *y)
{
  if (mDoc == nullptr)
    return false;

  auto page = mDoc->page (pn);
  auto size = page->pageSizeF ();
  if (rot == 0 || rot == 180)
    {
      *x = size.width ();
      *y = size.height ();
    }
  else
    {
      *x = size.height ();
      *y = size.width ();
    }
  return true;
}

int
ApvlvPDF::sum ()
{
  return mDoc ? mDoc->numPages () : 0;
}

unique_ptr<ApvlvPoses>
ApvlvPDF::pageSearch (int pn, const char *str, bool is_reverse)
{
  if (mDoc == nullptr)
    return nullptr;

  auto page = mDoc->page (pn);
  auto results = page->search (str);
  if (is_reverse)
    reverse (results.begin (), results.end ());
  auto poses = make_unique<ApvlvPoses> ();
  for (auto res : results)
    {
      poses->push_back (
          { res.left (), res.bottom (), res.right (), res.top () });
    }
  return poses;
}

bool
ApvlvPDF::pageRender (int pn, int ix, int iy, double zm, int rot, QImage *pix)
{
  if (mDoc == nullptr)
    return false;

  auto xres = 72.0, yres = 72.0;
  xres *= zm;
  yres *= zm;

  auto prot = Poppler::Page::Rotate0;
  if (rot == 90)
    prot = Poppler::Page::Rotate90;
  if (rot == 180)
    prot = Poppler::Page::Rotate180;
  if (rot == 270)
    prot = Poppler::Page::Rotate270;

  auto page = mDoc->page (pn);
  auto image = page->renderToImage (xres, yres, 0, 0, ix * zm, iy * zm, prot);
  *pix = std::move (image);
  return true;
}

bool
ApvlvPDF::pdf_get_index ()
{
  auto outlines = mDoc->outline ();
  if (outlines.empty ())
    return false;

  mIndex = { "", 0, getFilename (), FILE_INDEX_FILE };
  pdf_get_children_index (mIndex, outlines);
  return true;
}

void
ApvlvPDF::pdf_get_children_index (FileIndex &root_index,
                                  QVector<OutlineItem> &outlines)
{
  for (auto const &outline : outlines)
    {
      FileIndex index{ outline.name ().toStdString (),
                       outline.destination ()->pageNumber () - 1, "",
                       FILE_INDEX_PAGE };
      auto child_outlines = outline.children ();
      pdf_get_children_index (index, child_outlines);
      root_index.mChildrenIndex.emplace_back (index);
    }
}

bool
ApvlvPDF::pageAnnotUnderline (int pn, double x1, double y1, double x2,
                              double y2)
{
  return true;
}

bool
ApvlvPDF::pageAnnotText (int pn, double x1, double y1, double x2, double y2,
                         const char *text)
{
  return true;
}

ApvlvAnnotTexts
ApvlvPDF::pageAnnotTexts (int pn)
{
  return {};
}

bool
ApvlvPDF::pageAnnotUpdate (int pn, ApvlvAnnotText *text)
{
  return false;
}

}

// Local Variables:
// mode: c++
// End:

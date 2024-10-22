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
FILE_TYPE_DEFINITION (ApvlvPDF, { ".pdf" });

using namespace std;
using namespace Poppler;

bool
ApvlvPDF::load (const string &filename)
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
      return false;
    }

  generateIndex ();
  return true;
}

SizeF
ApvlvPDF::pageSizeF (int pn, int rot)
{
  auto page = mDoc->page (pn);
  auto qsize = page->pageSizeF ();
  if (rot == 0 || rot == 180)
    {
      return { qsize.width (), qsize.height () };
    }
  else
    {
      return { qsize.height (), qsize.width () };
    }
}

int
ApvlvPDF::sum ()
{
  return mDoc ? mDoc->numPages () : 0;
}

unique_ptr<WordListRectangle>
ApvlvPDF::pageSearch (int pn, const char *str)
{
  if (mDoc == nullptr)
    return nullptr;

  auto page = mDoc->page (pn);
  auto results = page->search (str);
  if (results.empty ())
    return nullptr;

  auto poses = make_unique<WordListRectangle> ();
  for (auto const &res : results)
    {
      WordRectangle wr;
      wr.rect_list.push_back (
          { res.left (), res.top (), res.right (), res.bottom () });
      poses->push_back (wr);
    }
  return poses;
}

bool
ApvlvPDF::pageRenderToImage (int pn, double zm, int rot, QImage *pix)
{
  if (mDoc == nullptr)
    return false;

  auto xres = 72.0 * zm;
  auto yres = 72.0 * zm;

  auto prot = Poppler::Page::Rotate0;
  if (rot == 90)
    prot = Poppler::Page::Rotate90;
  if (rot == 180)
    prot = Poppler::Page::Rotate180;
  if (rot == 270)
    prot = Poppler::Page::Rotate270;

  auto page = mDoc->page (pn);
  auto size = page->pageSizeF ();
  auto image = page->renderToImage (xres, yres, 0, 0, size.width () * zm,
                                    size.height () * zm, prot);
  *pix = std::move (image);
  return true;
}

bool
ApvlvPDF::generateIndex ()
{
  auto outlines = mDoc->outline ();
  if (outlines.empty ())
    return false;

  mIndex = { "", 0, getFilename (), FileIndexType::FILE };
  generateChildrenIndex (mIndex, outlines);
  return true;
}

void
ApvlvPDF::generateChildrenIndex (FileIndex &root_index,
                                 const QVector<OutlineItem> &outlines)
{
  for (auto const &outline : outlines)
    {
      FileIndex index{ outline.name ().toStdString (),
                       outline.destination ()->pageNumber () - 1, "",
                       FileIndexType::PAGE };
      auto child_outlines = outline.children ();
      generateChildrenIndex (index, child_outlines);
      root_index.mChildrenIndex.emplace_back (index);
    }
}

}

// Local Variables:
// mode: c++
// End:

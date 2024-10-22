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
/* @CPPFILE ApvlvDjvu.cc
 *
 *  Author: Alf <naihe2010@126.com>
 */

#include "ApvlvDjvu.h"
#include "ApvlvUtil.h"

namespace apvlv
{
using namespace std;

void
handleDdjvuMessages (ddjvu_context_t *ctx, int wait)
{
  const ddjvu_message_t *msg;
  if (wait)
    ddjvu_message_wait (ctx);
  while ((msg = ddjvu_message_peek (ctx)))
    {
      qDebug ("tag: %d", msg->m_any.tag);
      switch (msg->m_any.tag)
        {
        case DDJVU_ERROR:
          break;
        case DDJVU_INFO:
          break;
        case DDJVU_PAGEINFO:
          break;
        default:
          break;
        }
      ddjvu_message_pop (ctx);
    }
}

FILE_TYPE_DEFINITION (ApvlvDJVU, { ".djv", ".djvu" });

bool
ApvlvDJVU::load (const string &filename)
{
  mContext = ddjvu_context_create ("apvlv");
  if (mContext == nullptr)
    {
      qCritical ("djvu context error");
      return false;
    }

  mDoc
      = ddjvu_document_create_by_filename (mContext, filename.c_str (), false);
  if (mDoc == nullptr)
    {
      qCritical ("djvu create document error");
      ddjvu_context_release (mContext);
      mContext = nullptr;
      return false;
    }

  if (ddjvu_document_get_type (mDoc) == DDJVU_DOCTYPE_SINGLEPAGE)
    {
      qDebug ("djvu type: %d", ddjvu_document_get_type (mDoc));
      return true;
    }
  else
    {
      ddjvu_document_release (mDoc);
      mDoc = nullptr;
      ddjvu_context_release (mContext);
      mContext = nullptr;
      return false;
    }
}

ApvlvDJVU::~ApvlvDJVU ()
{
  if (mContext)
    {
      ddjvu_context_release (mContext);
    }

  if (mDoc)
    {
      ddjvu_document_release (mDoc);
    }
}

SizeF
ApvlvDJVU::pageSizeF (int pn, int rot)
{
  ddjvu_status_t t;
  ddjvu_pageinfo_t info;
  while ((t = ddjvu_document_get_pageinfo (mDoc, 0, &info)) < DDJVU_JOB_OK)
    {
      handleDdjvuMessages (mContext, true);
    }

  SizeF sizef{ 0.0f, 0.0f };
  if (t == DDJVU_JOB_OK)
    {
      sizef.width = static_cast<double> (info.width);
      sizef.height = static_cast<double> (info.height);
    }
  return sizef;
}

int
ApvlvDJVU::sum ()
{
  return mDoc ? ddjvu_document_get_pagenum (mDoc) : 0;
}

bool
ApvlvDJVU::pageRenderToImage (int pn, double zm, int rot, QImage *pix)
{
  ddjvu_page_t *tpage;

  if ((tpage = ddjvu_page_create_by_pageno (mDoc, pn)) == nullptr)
    {
      qDebug ("no this page: %d", pn);
      return false;
    }

  auto size = pageSizeF (pn, rot);

  auto dx = size.width * zm;
  auto dy = size.height * zm;

  auto ix = static_cast<int> (dx);
  auto iy = static_cast<int> (dy);
  ddjvu_rect_t prect = { 0, 0, static_cast<unsigned int> (ix),
                         static_cast<unsigned int> (iy) };
  ddjvu_rect_t rrect = { 0, 0, static_cast<unsigned int> (ix),
                         static_cast<unsigned int> (iy) };
  ddjvu_format_t *format
      = ddjvu_format_create (DDJVU_FORMAT_RGB24, 0, nullptr);
  ddjvu_format_set_row_order (format, true);

  auto psize = 3 * ix * iy;
  auto buffer = make_unique<char[]> (psize);

  int retry = 0;
  while (retry <= 20
         && ddjvu_page_render (tpage, DDJVU_RENDER_COLOR, &prect, &rrect,
                               format, 3 * ix, buffer.get ())
                == false)
    {
      this_thread::sleep_for (50ms);
      ++retry;
      qDebug ("fender failed, retry %d", retry);
    }

  auto image = QImage (ix, iy, QImage::Format_RGB888);
  for (auto y = 0; y < iy; ++y)
    {
      for (auto x = 0; x < ix; ++x)
        {
          auto rgb = buffer.get () + 3 * (x + y * ix);
          image.setPixel (x, y, qRgb (rgb[0], rgb[1], rgb[2]));
        }
    }
  *pix = std::move (image);

  return true;
}

}

// Local Variables:
// mode: c++
// End:

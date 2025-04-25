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
/* @CPPFILE ApvlvFileWidget.cc
 *
 *  Author: Alf <naihe2010@126.com>
 */

#include "ApvlvFileWidget.h"
#include "ApvlvParams.h"

namespace apvlv
{

using namespace std;

void
FileWidget::scroll (int times, int w, int h)
{
  // need impl in child
}

double
FileWidget::scrollRate ()
{
  if (mValScrollBar == nullptr)
    return mScrollValue;

  double maxv = mValScrollBar->maximum () - mValScrollBar->minimum ();
  double val = mValScrollBar->value () / maxv;
  if (val > 1.0)
    {
      return 1.00;
    }
  else if (val > 0.0)
    {
      return val;
    }
  else
    {
      return 0.00;
    }
}

void
FileWidget::scrollTo (double s, double y)
{
  if (!mValScrollBar)
    return;

  auto maxv = mValScrollBar->maximum () - mValScrollBar->minimum ();
  auto val = static_cast<int> (y * maxv);
  mValScrollBar->setValue (val);
}

void
FileWidget::scrollUp (int times)
{
  if (mValScrollBar == nullptr)
    return;

  auto const kj_pixels = ApvlvParams::instance ()->getIntOrDefault (
      "kj_pixels", APVLV_KJ_PIXELS_DEFAULT);
  auto rate = kj_pixels * times;
  if (mValScrollBar->value () - rate >= mValScrollBar->minimum ())
    {
      mValScrollBar->setValue (mValScrollBar->value () - rate);
    }
  else if (mValScrollBar->value () > mValScrollBar->minimum ())
    {
      mValScrollBar->setValue (mValScrollBar->minimum ());
    }
  else
    {
      auto params = ApvlvParams::instance ();
      if (params->getBoolOrDefault ("autoscrollpage"))
        {
          if (mPageNumber == 0)
            {
              if (params->getBoolOrDefault ("autoscrolldoc"))
                {
                  showPage (mFile->sum () - 1, 1.0);
                }
            }
          else
            {
              showPage (mPageNumber - 1, 1.0);
            }
        }
    }
}

void
FileWidget::scrollDown (int times)
{
  if (!mValScrollBar)
    return;

  auto const kj_pixels = ApvlvParams::instance ()->getIntOrDefault (
      "kj_pixels", APVLV_KJ_PIXELS_DEFAULT);
  auto rate = kj_pixels * times;
  if (mValScrollBar->value () + rate <= mValScrollBar->maximum ())
    {
      mValScrollBar->setValue (mValScrollBar->value () + rate);
    }
  else if (mValScrollBar->value () < mValScrollBar->maximum ())
    {
      mValScrollBar->setValue (mValScrollBar->maximum ());
    }
  else
    {
      auto params = ApvlvParams::instance ();
      if (params->getBoolOrDefault ("autoscrollpage"))
        {
          if (mPageNumber == mFile->sum () - 1)
            {
              if (params->getBoolOrDefault ("autoscrolldoc"))
                {
                  showPage (0, 0.0);
                }
            }
          else
            {
              showPage (mPageNumber + 1, 0.0);
            }
        }
    }
}

void
FileWidget::scrollLeft (int times)
{
  if (!mHalScrollBar)
    return;

  auto const hl_pixels = ApvlvParams::instance ()->getIntOrDefault (
      "hl_pixels", APVLV_HL_PIXELS_DEFAULT);
  auto val = mHalScrollBar->value () - hl_pixels * times;
  if (val > mHalScrollBar->minimumWidth ())
    {
      mHalScrollBar->setValue (val);
    }
  else
    {
      mHalScrollBar->setValue (mHalScrollBar->minimumWidth ());
    }
}

void
FileWidget::scrollRight (int times)
{
  if (!mHalScrollBar)
    return;

  auto const hl_pixels = ApvlvParams::instance ()->getIntOrDefault (
      "hl_pixels", APVLV_HL_PIXELS_DEFAULT);
  auto val = mHalScrollBar->value () + hl_pixels * times;
  if (val + mHalScrollBar->width () < mHalScrollBar->maximumWidth ())
    {
      mHalScrollBar->setValue (val);
    }
  else
    {
      mHalScrollBar->setValue (mHalScrollBar->maximumWidth ()
                               - mHalScrollBar->width ());
    }
}

}

/* Local Variables: */
/* mode: c++ */
/* End: */

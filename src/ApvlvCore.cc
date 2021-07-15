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
/* @CPPFILE ApvlvCore.cc
 *
 *  Author: Alf <naihe2010@126.com>
 */
/* @date Created: 2009/01/04 09:34:51 Alf*/

#include "ApvlvParams.h"
#include "ApvlvView.h"
#include "ApvlvCore.h"

#include <glib/gstdio.h>

#include <iostream>

namespace apvlv
{
    ApvlvCore::ApvlvCore (ApvlvView *view)
    {
      mInuse = true;

      mView = view;

      mReady = false;

      mProCmd = 0;

      mRotatevalue = 0;

      mSearchResults = nullptr;
      mSearchStr = "";

#if GTK_CHECK_VERSION (3, 0, 0)
      mVbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
#else
      mVbox = gtk_vbox_new (FALSE, 0);
#endif
      g_object_ref (mVbox);

      mScrollwin = gtk_scrolled_window_new (nullptr, nullptr);
      gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (mScrollwin),
                                      GTK_POLICY_AUTOMATIC,
                                      GTK_POLICY_AUTOMATIC);

      mVaj =
          gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW (mScrollwin));
      mHaj =
          gtk_scrolled_window_get_hadjustment (GTK_SCROLLED_WINDOW (mScrollwin));
    }

    ApvlvCore::~ApvlvCore ()
    {
      if (mGMonitor)
        {
          mGMonitor = nullptr;
        }

      g_object_unref (mVbox);
    }

    bool ApvlvCore::reload ()
    {
      return true;
    }

    void ApvlvCore::inuse (bool use)
    {
      mInuse = use;

      if (!mInuse && mView->hasloaded (filename (), type ()) == nullptr)
        {
          debug ("core :%p is not needed, delete it\n", this);
          delete this;
        }
    }

    bool ApvlvCore::inuse ()
    {
      return mInuse;
    }

    int ApvlvCore::type ()
    {
      return mType;
    }

    returnType ApvlvCore::process (int has, int ct, guint key)
    {
      return MATCH;
    }

    void ApvlvCore::setsize (int w, int h)
    {
      gtk_widget_set_size_request (widget (), w, h);
      gtk_widget_set_size_request (mScrollwin, w, h - 16);
      mStatus->setsize (w, 15);
      mWidth = w;
      mHeight = h;
    }

    ApvlvCore *ApvlvCore::copy ()
    {
      return nullptr;
    }

    const char *ApvlvCore::filename ()
    {
      return mFilestr.length () > 0 ? mFilestr.c_str () : nullptr;
    }

    gint ApvlvCore::pagenumber ()
    {
      return mPagenum + 1;
    }

    gdouble ApvlvCore::zoomvalue ()
    {
      return mZoomrate;
    }

    ApvlvFile *ApvlvCore::file ()
    {
      return mFile;
    }

    bool ApvlvCore::writefile (const char *name)
    {
      if (mFile != nullptr)
        {
          debug ("write %p to %s", mFile, name);
          return mFile->writefile (name ? name : filename ());
        }
      return false;
    }

    bool ApvlvCore::loadfile (const char *file, bool check)
    {
      return false;
    }

    GtkWidget *ApvlvCore::widget ()
    {
      return mVbox;
    }

    void ApvlvCore::showpage (gint p, gdouble s)
    {
    }

    void ApvlvCore::refresh ()
    {
    }

    double ApvlvCore::scrollrate ()
    {
      gdouble maxv = gtk_adjustment_get_upper (mVaj) - gtk_adjustment_get_lower (mVaj)
                     - gtk_adjustment_get_page_size (mVaj);
      gdouble val = gtk_adjustment_get_value (mVaj) / maxv;
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

    gboolean ApvlvCore::scrollto (double s)
    {
      if (!mReady)
        return FALSE;

      if (gtk_adjustment_get_upper (mVaj) != gtk_adjustment_get_lower (mVaj))
        {
          gdouble maxv = gtk_adjustment_get_upper (mVaj) - gtk_adjustment_get_lower (mVaj)
                         - gtk_adjustment_get_page_size (mVaj);
          gdouble val = maxv * s;
          gtk_adjustment_set_value (mVaj, val);
          mStatus->show (false);
          return TRUE;
        }
      else
        {
          //debug ("fatal a timer error, try again!");
          return FALSE;
        }
    }

    void ApvlvCore::scrollup (int times)
    {
      if (!mReady)
        return;
      gdouble val = gtk_adjustment_get_value (mVaj);
      gdouble sub = gtk_adjustment_get_upper (mVaj) - gtk_adjustment_get_lower (mVaj);
      gdouble page_size = sub / 3;
      gdouble screen_size = gtk_adjustment_get_page_size (mVaj);
      mVrate = page_size / mLines;
      debug(" val=%lf,sub=%lf", val, sub);
      debug(" pageSize=%lf, screen_size=%lf", page_size, screen_size);

      if (val - mVrate * times > gtk_adjustment_get_lower (mVaj))
        {
          gtk_adjustment_set_value (mVaj, val - mVrate * times);
        }
      else if (val > gtk_adjustment_get_lower (mVaj))
        {
          // set to min scroll value
          gtk_adjustment_set_value (mVaj, gtk_adjustment_get_lower (mVaj));
        }
      else
        {
          if (mAutoScrollPage)
            {
              if (mContinuous)
                {
                  /* set to pos so that one page is hidden above
                   * this is the same pos as in the previous case
                   * So one scroll step does nothing visible.
                   * This is still useful because scrolling overshoots most times because of the
                   * reload lag.
                   * TODO: add more pages and start loading in background somehow prior the transition is reached.
                     full_scroll = sub - screen_size
                     hide one page -> page_size/full_scroll = page_size/(sub-screen_size)
                  */
                  showpage (mPagenum - 1, page_size / (sub - screen_size));
                }
              else
                {
                  showpage (mPagenum + 1, 0.0);
                }
            }
        }

      mStatus->show (false);
    }

    void ApvlvCore::scrolldown (int times)
    {
      if (!mReady)
        return;
      gdouble val = gtk_adjustment_get_value (mVaj);
      gdouble sub = gtk_adjustment_get_upper (mVaj) - gtk_adjustment_get_lower (mVaj);
      gdouble page_size = sub / 3;
      gdouble screen_size = gtk_adjustment_get_page_size (mVaj);
      mVrate = page_size / mLines;
      debug(" val=%lf,sub=%lf", val, sub);
      debug(" pageSize=%lf, screen_size=%lf", page_size, screen_size);

      if (val + mVrate * times + screen_size < sub)
        {
          gtk_adjustment_set_value (mVaj, val + mVrate * times);
        }
      else if (val + screen_size < sub)
        {
          // not enough for one step, set to full scroll val
          // full_scroll = sub - screen_size
          gtk_adjustment_set_value (mVaj, sub - screen_size);
        }
      else
        {
          if (mAutoScrollPage)
            {
              if (mContinuous)
                {
                  /* set to pos so that one page is hidden below
                   * this is the same pos as in the previous case
                   * so one scroll step does nothing visible
                   * this is still useful because scrolling overshoots most times because of the
                   * reload lag.
                   * TODO: add more pages and start loading in background somehow prior the transition is reached.
                     full_scroll = sub - screen_size
                     abs_pos = full_scroll - page_size = sub - screen_size - page_size
                     rel_pos = abs_pos/full_scroll = (sub - screen_size - page_size) / (sub - screen_size)
                                                   =  1 - page_size/(sub - screen_size)

                  */
                  showpage (mPagenum + 1, 1 - page_size / (sub - screen_size));
                }
              else
                {
                  showpage (mPagenum + 1, 0.0);
                }
            }
        }

      mStatus->show (false);
    }

    void ApvlvCore::scrollleft (int times)
    {
      if (!mReady)
        return;

      mHrate = (gtk_adjustment_get_upper (mHaj) - gtk_adjustment_get_lower (mHaj)) / mChars;
      gdouble val = gtk_adjustment_get_value (mHaj) - mHrate * times;
      if (val > gtk_adjustment_get_lower (mVaj))
        {
          gtk_adjustment_set_value (mHaj, val);
        }
      else
        {
          gtk_adjustment_set_value (mHaj, gtk_adjustment_get_lower (mHaj));
        }
    }

    void ApvlvCore::scrollright (int times)
    {
      if (!mReady)
        return;

      mHrate = (gtk_adjustment_get_upper (mHaj) - gtk_adjustment_get_lower (mHaj)) / mChars;
      gdouble val = gtk_adjustment_get_value (mHaj) + mHrate * times;
      if (val + gtk_adjustment_get_page_size (mHaj) < gtk_adjustment_get_upper (mHaj))
        {
          gtk_adjustment_set_value (mHaj, val);
        }
      else
        {
          gtk_adjustment_set_value (mHaj, gtk_adjustment_get_upper (mHaj) - gtk_adjustment_get_page_size (mHaj));
        }
    }

    bool ApvlvCore::usecache ()
    {
      return false;
    }

    void ApvlvCore::usecache (bool use)
    {
    }

    bool ApvlvCore::print (int ct)
    {
      return false;
    }

    bool ApvlvCore::totext (const char *name)
    {
      return false;
    }

    bool ApvlvCore::rotate (int ct)
    {
      return false;
    }

    void ApvlvCore::markposition (const char s)
    {
    }

    void ApvlvCore::setzoom (const char *z)
    {
    }

    void ApvlvCore::jump (const char s)
    {
    }

    void ApvlvCore::nextpage (int times)
    {
    }

    void ApvlvCore::prepage (int times)
    {
    }

    void ApvlvCore::halfnextpage (int times)
    {
    }

    void ApvlvCore::halfprepage (int times)
    {
    }

    bool ApvlvCore::search (const char *str, bool reverse)
    {
      return false;
    }

    void ApvlvCore::gotolink (int ct)
    {
    }

    int ApvlvCore::getskip ()
    {
      return mSkip;
    }

    void ApvlvCore::setskip (int ct)
    {
      mSkip = ct;
    }

    void ApvlvCore::returnlink (int ct)
    {
    }

    void ApvlvCore::setactive (bool act)
    {
      mActive = act;

      if (mActive && filename ())
        {
          gchar *base = g_path_get_basename (filename ());
          mView->settitle (base);
          g_free (base);
        }
    }

    ApvlvCoreStatus::ApvlvCoreStatus ()
    {
#if GTK_CHECK_VERSION (3, 0, 0)
      mHbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
#else
      mHbox = gtk_hbox_new (FALSE, 0);
#endif
    }

    ApvlvCoreStatus::~ApvlvCoreStatus ()
    = default;

    GtkWidget *ApvlvCoreStatus::widget ()
    {
      return mHbox;
    }

    void ApvlvCoreStatus::active (bool act)
    {
    }

    void ApvlvCoreStatus::setsize (int w, int h)
    {
    }

    void ApvlvCoreStatus::show (bool mContinuous)
    {
    }
}

// Local Variables:
// mode: c++
// End:

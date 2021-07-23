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

      mZoomrate = 1.0;

      mZoommode = NORMAL;

      mRotatevalue = 0;

      mSearchResults = nullptr;
      mSearchStr = "";

      mVbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
      g_object_ref (mVbox);

      mPaned = gtk_paned_new (GTK_ORIENTATION_HORIZONTAL);

      auto f_width = gParams->valuei ("fix_width");
      auto f_height = gParams->valuei ("fix_height");

      if (f_width > 0 && f_height > 0)
        {
          gtk_widget_set_size_request (mPaned, f_width, f_height);

          auto hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
          gtk_box_pack_start (GTK_BOX (hbox), mPaned, TRUE, FALSE, 0);
          gtk_box_pack_start (GTK_BOX (mVbox), hbox, TRUE, FALSE, 0);
        }
      else
        {
          gtk_box_pack_start (GTK_BOX (mVbox), mPaned, TRUE, TRUE, 0);
        }

      mContentWidget = gtk_scrolled_window_new (nullptr, nullptr);
      gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (mContentWidget),
                                      GTK_POLICY_AUTOMATIC,
                                      GTK_POLICY_AUTOMATIC);

      mContent = new ApvlvContent ();
      gtk_container_add (GTK_CONTAINER(mContentWidget), mContent->widget ());

      mContentVaj =
          gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW (mContentWidget));
      gtk_scrolled_window_get_hadjustment (GTK_SCROLLED_WINDOW (mContentWidget));

      mMainWidget = gtk_scrolled_window_new (nullptr, nullptr);
      gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (mMainWidget),
                                      GTK_POLICY_AUTOMATIC,
                                      GTK_POLICY_AUTOMATIC);

      mMainVaj =
          gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW (mMainWidget));
      mMainHaj =
          gtk_scrolled_window_get_hadjustment (GTK_SCROLLED_WINDOW (mMainWidget));

      gtk_paned_add1 (GTK_PANED (mPaned), mContentWidget);
      gtk_paned_add2 (GTK_PANED (mPaned), mMainWidget);

      gtk_paned_set_position (GTK_PANED (mPaned), 0);

      mStatus = new ApvlvStatus ();
      gtk_box_pack_end (GTK_BOX (mVbox), mStatus->widget (), FALSE, FALSE, 0);
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

      if (!mInuse && mView->hasloaded (filename ()) == nullptr)
        {
          debug ("core :%p is not needed, delete it\n", this);
          delete this;
        }
    }

    bool ApvlvCore::inuse ()
    {
      return mInuse;
    }

    returnType ApvlvCore::process (int has, int ct, guint key)
    {
      return MATCH;
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

    bool ApvlvCore::loadfile (const char *file, bool check, bool show_content)
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
      gdouble maxv = gtk_adjustment_get_upper (mMainVaj) - gtk_adjustment_get_lower (mMainVaj)
                     - gtk_adjustment_get_page_size (mMainVaj);
      gdouble val = gtk_adjustment_get_value (mMainVaj) / maxv;
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

      GtkAdjustment *vaj = mMainVaj;
      if (isControledContent ())
        {
          vaj = mContentVaj;
        }

      if (gtk_adjustment_get_upper (vaj) != gtk_adjustment_get_lower (vaj))
        {
          gdouble maxv = gtk_adjustment_get_upper (vaj) - gtk_adjustment_get_lower (vaj)
                         - gtk_adjustment_get_page_size (vaj);
          gdouble val = maxv * s;
          gtk_adjustment_set_value (vaj, val);
          if (!isControledContent ())
            {
              show ();
            }
          return TRUE;
        }
      else
        {
          return FALSE;
        }
    }

    void ApvlvCore::scrollup (int times)
    {
      if (!mReady)
        return;
      gdouble val = gtk_adjustment_get_value (mMainVaj);
      gdouble sub = gtk_adjustment_get_upper (mMainVaj) - gtk_adjustment_get_lower (mMainVaj);
      gdouble page_size = sub / 3;
      gdouble screen_size = gtk_adjustment_get_page_size (mMainVaj);
      mVrate = page_size / mLines;
      debug(" val=%lf,sub=%lf", val, sub);
      debug(" pageSize=%lf, screen_size=%lf", page_size, screen_size);

      if (val - mVrate * times > gtk_adjustment_get_lower (mMainVaj))
        {
          gtk_adjustment_set_value (mMainVaj, val - mVrate * times);
        }
      else if (val > gtk_adjustment_get_lower (mMainVaj))
        {
          // set to min scroll value
          gtk_adjustment_set_value (mMainVaj, gtk_adjustment_get_lower (mMainVaj));
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

      show ();
    }

    void ApvlvCore::scrolldown (int times)
    {
      if (!mReady)
        return;
      gdouble val = gtk_adjustment_get_value (mMainVaj);
      gdouble sub = gtk_adjustment_get_upper (mMainVaj) - gtk_adjustment_get_lower (mMainVaj);
      gdouble page_size = sub / 3;
      gdouble screen_size = gtk_adjustment_get_page_size (mMainVaj);
      mVrate = page_size / mLines;
      debug(" val=%lf,sub=%lf", val, sub);
      debug(" pageSize=%lf, screen_size=%lf", page_size, screen_size);

      if (val + mVrate * times + screen_size < sub)
        {
          gtk_adjustment_set_value (mMainVaj, val + mVrate * times);
        }
      else if (val + screen_size < sub)
        {
          // not enough for one step, set to full scroll val
          // full_scroll = sub - screen_size
          gtk_adjustment_set_value (mMainVaj, sub - screen_size);
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

      show ();
    }

    void ApvlvCore::scrollleft (int times)
    {
      if (!mReady)
        return;

      mHrate = (gtk_adjustment_get_upper (mMainHaj) - gtk_adjustment_get_lower (mMainHaj)) / mChars;
      gdouble val = gtk_adjustment_get_value (mMainHaj) - mHrate * times;
      if (val > gtk_adjustment_get_lower (mMainVaj))
        {
          gtk_adjustment_set_value (mMainHaj, val);
        }
      else
        {
          gtk_adjustment_set_value (mMainHaj, gtk_adjustment_get_lower (mMainHaj));
        }
    }

    void ApvlvCore::scrollright (int times)
    {
      if (!mReady)
        return;

      mHrate = (gtk_adjustment_get_upper (mMainHaj) - gtk_adjustment_get_lower (mMainHaj)) / mChars;
      gdouble val = gtk_adjustment_get_value (mMainHaj) + mHrate * times;
      if (val + gtk_adjustment_get_page_size (mMainHaj) < gtk_adjustment_get_upper (mMainHaj))
        {
          gtk_adjustment_set_value (mMainHaj, val);
        }
      else
        {
          gtk_adjustment_set_value (mMainHaj,
                                    gtk_adjustment_get_upper (mMainHaj) - gtk_adjustment_get_page_size (mMainHaj));
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

    void ApvlvCore::toggleContent ()
    {
      auto show = !isShowContent ();
      toggleContent (show);
    }

    void ApvlvCore::toggleContent (bool show)
    {
      if (show)
        {
          ApvlvFileIndex *index = mFile->new_index ();
          if (index)
            {
              mContent->setIndex (index);
              delete mDirIndex;
              mDirIndex = nullptr;
            }
          gtk_paned_set_position (GTK_PANED (mPaned), APVLV_DEFAULT_CONTENT_WIDTH);
        }
      else
        {
          gtk_paned_set_position (GTK_PANED (mPaned), 0);
        }
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
    void ApvlvCore::setDirIndex (const gchar *path)
    {
      ApvlvFileIndex *index = ApvlvFileIndex::newDirIndex (path);
      if (index)
        {
          mContent->setIndex (index);
          delete mDirIndex;
          mDirIndex = index;
        }
    }

    bool ApvlvCore::toggledControlContent (bool is_right)
    {
      if (!isShowContent ())
        {
          return false;
        }

      auto controled = isControledContent ();

      if (!controled && !is_right)
        {
          gtk_widget_set_state_flags (mContentWidget, GTK_STATE_FLAG_FOCUSED, TRUE);
          gtk_widget_set_state_flags (mMainWidget, GTK_STATE_FLAG_INSENSITIVE, TRUE);
          return true;
        }
      else if (controled && is_right)
        {
          gtk_widget_set_state_flags (mContentWidget, GTK_STATE_FLAG_INSENSITIVE, TRUE);
          gtk_widget_set_state_flags (mMainWidget, GTK_STATE_FLAG_FOCUSED, TRUE);
          return true;
        }

      return false;
    }

    void ApvlvCore::show ()
    {
    }

    bool ApvlvCore::isShowContent ()
    {
      auto position = gtk_paned_get_position (GTK_PANED (mPaned));
      return position >= 1;
    }

    bool ApvlvCore::isControledContent ()
    {
      auto focused = gtk_widget_get_state_flags (mContentWidget);
      return focused & GTK_STATE_FLAG_FOCUSED;
    }

    ApvlvStatus::ApvlvStatus ()
    {
      mHbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
      g_object_ref (mHbox);
    }

    ApvlvStatus::~ApvlvStatus ()
    {
      g_object_unref (mHbox);
    }

    GtkWidget *ApvlvStatus::widget ()
    {
      return mHbox;
    }

    void ApvlvStatus::active (bool act)
    {
      auto children = gtk_container_get_children (GTK_CONTAINER(mHbox));
      for (auto child = children; child != nullptr; child = child->next)
        {
          gtk_widget_set_state_flags (GTK_WIDGET (child->data),
                                      (act) ? GTK_STATE_FLAG_ACTIVE :
                                      GTK_STATE_FLAG_INSENSITIVE, TRUE);
        }
    }

    void ApvlvStatus::show (const vector<string> &msgs)
    {
      vector<GtkWidget *> newlabels;
      auto children = gtk_container_get_children (GTK_CONTAINER(mHbox));
      for (const auto &msg: msgs)
        {
          if (children != nullptr)
            {
              gtk_label_set_text (GTK_LABEL (children->data), msg.c_str ());
              children = children->next;
            }
          else
            {
              auto label = gtk_label_new (msg.c_str ());
              newlabels.push_back (label);
            }
        }

      for (auto label: newlabels)
        {
          gtk_box_pack_start (GTK_BOX (mHbox), label, TRUE, TRUE, 0);
        }
    }
}

// Local Variables:
// mode: c++
// End:

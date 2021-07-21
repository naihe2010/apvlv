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
/* @CPPFILE ApvlvDoc.cc
 *
 *  Author: Alf <naihe2010@126.com>
 */
/* @date Created: 2008/09/30 00:00:00 Alf */

#include "ApvlvUtil.h"
#include "ApvlvParams.h"
#include "ApvlvInfo.h"
#include "ApvlvView.h"
#include "ApvlvDoc.h"

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <webkit2/webkit2.h>

#include <cstdlib>
#include <cmath>
#include <iostream>
#include <fstream>
#include <sstream>

namespace apvlv
{
    static void invert_pixbuf (GdkPixbuf *);
    static GtkPrintSettings *settings = nullptr;
    const int APVLV_DOC_CURSOR_WIDTH = 2;

    DISPLAY_TYPE get_display_type_by_filename (const char *name)
    {
      DISPLAY_TYPE type = DISPLAY_TYPE_IMAGE;

      if (g_ascii_strcasecmp (name + strlen (name) - 4, ".htm") == 0
          || g_ascii_strcasecmp (name + strlen (name) - 5, ".html") == 0
          || g_ascii_strcasecmp (name + strlen (name) - 5, ".epub") == 0)
        {
          type = DISPLAY_TYPE_HTML;
        }

      return type;
    }

    ApvlvDoc::ApvlvDoc (ApvlvView *view, DISPLAY_TYPE type, const char *zm, bool cache) : ApvlvCore (view)
    {
      mCurrentCache1 = mCurrentCache2 = mCurrentCache3 = nullptr;

      mReady = false;

      mAdjInchg = false;

      mAutoScrollPage = gParams->valueb ("autoscrollpage");
      mAutoScrollDoc = gParams->valueb ("autoscrolldoc");
      mContinuous = gParams->valueb ("continuous");
      if (type != 0)
        {
          mContinuous = false;
        }

      mZoominit = false;

      mLines = 50;
      mChars = 80;

      mProCmd = 0;

      mInVisual = false;

      mBlankx1 = mBlanky1 = mBlankx2 = mBlanky2 = 0;

      mCurx = mCury = 0;

      mLastpress = 0;

      mRotatevalue = 0;

      mFile = nullptr;

      mSearchResults = nullptr;
      mSearchStr = "";

      GtkWidget *vbox, *ebox;

      if (mContinuous && gParams->valuei ("continuouspad") > 0)
        {
#if GTK_CHECK_VERSION (3, 0, 0)
          vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, gParams->valuei ("continuouspad"));
#else
          vbox = gtk_vbox_new (FALSE, gParams->valuei ("continuouspad"));
#endif
        }
      else
        {
#if GTK_CHECK_VERSION (3, 0, 0)
          vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
#else
          vbox = gtk_vbox_new (FALSE, 0);
#endif
        }

      ebox = gtk_event_box_new ();
      gtk_container_add (GTK_CONTAINER (ebox), vbox);
      g_signal_connect (G_OBJECT (ebox), "button-press-event",
                        G_CALLBACK (apvlv_doc_button_event), this);
      g_signal_connect (G_OBJECT (ebox), "motion-notify-event",
                        G_CALLBACK (apvlv_doc_motion_event), this);
#if GTK_CHECK_VERSION(3, 0, 0)
      gtk_container_add (GTK_CONTAINER (mMainWidget), ebox);
#else
      gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (mMainWidget), ebox);
#endif

      mDisplayType = type;
      if (type == 0)
        {
          mImg1 = gtk_image_new ();
          gtk_box_pack_start (GTK_BOX (vbox), mImg1, TRUE, TRUE, 0);
          if (mAutoScrollPage && mContinuous)
            {
              mImg2 = gtk_image_new ();
              gtk_box_pack_start (GTK_BOX (vbox), mImg2, TRUE, TRUE, 0);
              mImg3 = gtk_image_new ();
              gtk_box_pack_start (GTK_BOX (vbox), mImg3, TRUE, TRUE, 0);
            }
        }
      else if (type == 1)
        {
          mWeb1 = webkit_web_view_new ();
          gtk_box_pack_start (GTK_BOX (vbox), mWeb1, TRUE, TRUE, 0);
          g_signal_connect (mWeb1, "resource-load-started",
                            G_CALLBACK (webview_resource_load_started_cb), this);
          g_signal_connect (mWeb1, "load-changed",
                            G_CALLBACK (webview_load_changed_cb), this);
          g_signal_connect (mWeb1, "context-menu",
                            G_CALLBACK (webview_context_menu_cb), this);
        }

      g_signal_connect (G_OBJECT (mMainVaj), "value-changed",
                        G_CALLBACK (apvlv_doc_on_mouse), this);

      mStatus = new ApvlvDocStatus (this);
      gtk_box_pack_end (GTK_BOX (mVbox), mStatus->widget (), FALSE, FALSE, 0);
    }

    ApvlvDoc::~ApvlvDoc ()
    {
      delete mCurrentCache1;
      delete mCurrentCache2;
      delete mCurrentCache3;

      savelastposition (filename ());
      mPositions.clear ();

      delete mFile;

      delete mStatus;
    }

    void ApvlvDoc::blankarea (int x1, int y1, int x2, int y2, guchar *buffer,
                              int width, int height)
    {
      //    debug ("x1: %d, y1: %d, x2: %d, y2:%d", x1, y1, x2, y2);
      if (x2 > width)
        {
          x2 = width;
        }
      if (y2 > height)
        {
          y2 = height;
        }

      for (gint y = y1; y < y2; y++)
        {
          for (gint x = x1; x < x2; x++)
            {
              gint p = (gint) (y * width * 3 + (x * 3));
              buffer[p + 0] = 0xff - buffer[p + 0];
              buffer[p + 1] = 0xff - buffer[p + 1];
              buffer[p + 2] = 0xff - buffer[p + 2];
            }
        }
    }

    void ApvlvDoc::blankaction (double x, double y)
    {
      ApvlvDocCache *cache;
      ApvlvPos pos = {x, x, y, y};

      if (x < 0)
        {
          x = 0;
        }
      if (y < 0)
        {
          y = 0;
        }

      cache = mCurrentCache1;
      if (x > cache->getwidth ())
        {
          x = cache->getwidth ();
        }

      if (y > cache->getheight ())
        {
          if (mCurrentCache2 != nullptr)
            {
              cache = mCurrentCache2;
            }
          else
            {
              cache = nullptr;
            }
        }

      if (cache == nullptr)
        {
          return;
        }

      if (strcasecmp (gParams->values ("doubleclick"), "word") == 0)
        {
          ApvlvWord *word;
          word =
              cache->getword (int (x),
                              cache ==
                              mCurrentCache2 ? int (y - mCurrentCache1->getheight () - mCurrentCache3->getheight () -
                                                    2 * gParams->valuei ("continuouspad")) : int (y));
          if (word != nullptr)
            {
              pos = word->pos;
              debug ("get word: [%s] x1:%f, x2:%f, y1:%f, y2:%f",
                     word->word.c_str (), pos.x1, pos.x2, pos.y1, pos.y2);
            }
        }
      else if (strcasecmp (gParams->values ("doubleclick"), "line") == 0)
        {
          ApvlvLine *line;
          line =
              cache->getline (x,
                              cache ==
                              mCurrentCache2 ? y - mCurrentCache1->getheight () - mCurrentCache3->getheight () -
                                               2 * gParams->valuei ("continuouspad") : y);
          if (line != nullptr)
            {
              pos = line->pos;
            }
        }
      else if (strcasecmp (gParams->values ("doubleclick"), "page") == 0)
        {
          pos.x1 = 0;
          pos.x2 = cache->getwidth ();
          pos.y1 = 0;
          pos.y2 = cache->getheight ();
        }
      else
        {
          return;
        }

      mInVisual = VISUAL_CTRL_V;
      mBlankx1 = gint (pos.x1);
      mBlanky1 = gint (pos.y1);
      blank (gint (pos.x2), gint (pos.y2));

      //    yank (1);
    }

    void ApvlvDoc::blank (int bx, int by)
    {
      //    debug ("bx: %d, by: %d", bx, by);

      if (mDisplayType != DISPLAY_TYPE_IMAGE)
        {
          return;
        }

      ApvlvDocCache *cache = mCurrentCache1;
      gint rate = gint (cache->getheight () / mLines);

      // get the right cache, 1 or 2
      if (by + rate >= cache->getheight ())
        {
          if (mCurrentCache2 == nullptr)
            {
              by = gint (cache->getheight () - rate);
            }
          else
            {
              cache = mCurrentCache2;
              rate = gint (cache->getheight () / mLines);
            }
        }

      // fix the width and height value
      if (bx + APVLV_DOC_CURSOR_WIDTH >= cache->getwidth ())
        {
          bx = gint (cache->getwidth () - APVLV_DOC_CURSOR_WIDTH);
        }
      if (bx < 0)
        {
          bx = 0;
        }
      if (by < 0)
        {
          by = 0;
        }

      // remember the position
      mCurx = bx;
      mCury = by;

      guchar *buffer = cache->getdata (true);
      GdkPixbuf *pixbuf = cache->getbuf (true);

      if (buffer == nullptr || pixbuf == nullptr)
        {
          debug ("pixbuf data or structure error");
          return;
        }

      mBlankx2 = bx;
      mBlanky2 = by;

      gint y1 = mBlanky1, y2 = mBlanky2;
      if (y1 > y2)
        {
          y1 = mBlanky2;
          y2 = mBlanky1;
        }
      gint x1 = mBlankx1, x2 = mBlankx2;
      if (x1 > x2)
        {
          x1 = mBlankx2;
          x2 = mBlankx1;
        }

      // fix the height value
      // corret to the cache's height
      // if the height > cache1, the minus it
      if (cache == mCurrentCache2)
        {
          by -=
              gint (mCurrentCache1->getheight () - gParams->valuei ("continuouspad"));
          y1 -=
              gint (mCurrentCache1->getheight () - gParams->valuei ("continuouspad"));
          y2 -=
              gint (mCurrentCache1->getheight () - gParams->valuei ("continuouspad"));
        }

      if (by < 0)
        {
          by = 0;
        }
      if (y1 < 0)
        {
          y1 = 0;
        }
      if (y2 < 0)
        {
          y2 = 0;
        }

      if (mInVisual == VISUAL_V)
        {
          //      debug ("");
          if (y1 + rate >= y2)
            {
              blankarea (x1, y1, x2 + APVLV_DOC_CURSOR_WIDTH,
                         y1 == y2 ? y1 + rate : y2, buffer, cache->getwidth (),
                         cache->getheight ());
            }
          else if (y1 + rate * 2 >= y2)
            {
              blankarea (x1, y1, cache->getwidth (), y1 + rate, buffer,
                         cache->getwidth (), cache->getheight ());
              blankarea (0, y1 + rate, x2 + APVLV_DOC_CURSOR_WIDTH, y2,
                         buffer, cache->getwidth (), cache->getheight ());
            }
          else
            {
              blankarea (x1, y1, cache->getwidth (), y1 + rate, buffer,
                         cache->getwidth (), cache->getheight ());
              blankarea (0, y1 + rate, cache->getwidth (), y2 - rate, buffer,
                         cache->getwidth (), cache->getheight ());
              blankarea (0, y2 - rate, x2 + APVLV_DOC_CURSOR_WIDTH, y2,
                         buffer, cache->getwidth (), cache->getheight ());
            }
        }
      else if (mInVisual == VISUAL_CTRL_V)
        {
          //      debug ("");
          blankarea (x1, y1,
                     x1 + APVLV_DOC_CURSOR_WIDTH >
                     x2 ? x1 + APVLV_DOC_CURSOR_WIDTH : x2,
                     y1 + rate > y2 ? y1 + rate : y2, buffer,
                     cache->getwidth (), cache->getheight ());
        }
      else
        {
          //      debug ("");
          blankarea (bx, by, bx + APVLV_DOC_CURSOR_WIDTH, by + rate, buffer,
                     cache->getwidth (), cache->getheight ());
        }

      // reset the pixbuf to image container
      if (cache == mCurrentCache1)
        {
          if (mCurrentCache2 != nullptr)
            {
              mCurrentCache2->getdata (true);
              GdkPixbuf *p = mCurrentCache2->getbuf (true);
              gtk_image_set_from_pixbuf (GTK_IMAGE (mImg2), p);
            }

          gtk_image_set_from_pixbuf (GTK_IMAGE (mImg1), pixbuf);
        }
      else
        {
          mCurrentCache1->getdata (true);
          GdkPixbuf *p = mCurrentCache1->getbuf (true);
          gtk_image_set_from_pixbuf (GTK_IMAGE (mImg1), p);

          gtk_image_set_from_pixbuf (GTK_IMAGE (mImg2), pixbuf);
        }
    }

    void ApvlvDoc::togglevisual (int key)
    {
      if (!gParams->valueb ("visualmode"))
        {
          return;
        }

      if (mInVisual == VISUAL_NONE)
        {
          mBlankx1 = mCurx;
          mBlanky1 = mCury;
        }

      int type = key == 'v' ? VISUAL_V : VISUAL_CTRL_V;
      if (mInVisual == type)
        {
          mInVisual = VISUAL_NONE;
        }
      else
        {
          mInVisual = type;
        }
      blank (mCurx, mCury);
    }

    int ApvlvDoc::yank (int times)
    {
      char *txt1 = nullptr, *txt2 = nullptr, *txt3 = nullptr;

      gint y1 = mBlanky1, y2 = mBlanky2;
      if (y1 > y2)
        {
          y1 = mBlanky2;
          y2 = mBlanky1;
        }
      gint x1 = mBlankx1, x2 = mBlankx2;
      if (x1 > x2)
        {
          x1 = mBlankx2;
          x2 = mBlankx1;
        }
      if (y1 == y2 || mInVisual == VISUAL_CTRL_V)
        {
          mFile->pagetext (mPagenum, x1, y1, x2 + APVLV_DOC_CURSOR_WIDTH,
                           gint (y2 + mVrate), &txt1);
        }
      else
        {
          mFile->pagetext (mPagenum, x1, y1, mCurrentCache1->getwidth (),
                           gint (y1 + mVrate), &txt1);
          mFile->pagetext (mPagenum, 0, gint (y1 + mVrate),
                           mCurrentCache1->getwidth (), y2, &txt2);
          mFile->pagetext (mPagenum, 0, y2, x2 + APVLV_DOC_CURSOR_WIDTH,
                           gint (y2 + mVrate), &txt3);
        }

      GtkClipboard *cb = gtk_clipboard_get (nullptr);
      string text;
      if (txt1 != nullptr)
        {
          text += txt1;
          g_free (txt1);
        }
      if (txt2 != nullptr)
        {
          text += txt2;
          g_free (txt2);
        }
      if (txt3 != nullptr)
        {
          text += txt3;
          g_free (txt3);
        }
      gtk_clipboard_set_text (cb, text.c_str (), gint (text.length ()));

      return 0;
    }

    returnType ApvlvDoc::subprocess (int ct, guint key)
    {
      guint procmd = mProCmd;
      mProCmd = 0;
      switch (procmd)
        {
          case 'm':
            markposition (char (key));
          break;

          case '\'':
            jump (char (key));
          break;

          case 'z':
            if (key == 'i')
              {
                char temp[0x10];
                g_snprintf (temp, sizeof temp, "%f", mZoomrate * 1.1);
                setzoom (temp);
              }
            else if (key == 'o')
              {
                char temp[0x10];
                g_snprintf (temp, sizeof temp, "%f", mZoomrate / 1.1);
                setzoom (temp);
              }
            else if (key == 'h')
              {
                setzoom ("fitheight");
              }
            else if (key == 'w')
              {
                setzoom ("fitwidth");
              }
          break;

          default:
            return NO_MATCH;
          break;
        }

      return MATCH;
    }

    returnType ApvlvDoc::process (int has, int ct, guint key)
    {
      if (mProCmd != 0)
        {
          return subprocess (ct, key);
        }

      if (!has)
        {
          ct = 1;
        }

      switch (key)
        {
          case GDK_KEY_Page_Down:
          case CTRL ('f'):
            nextpage (ct);
          break;
          case GDK_KEY_Page_Up:
          case CTRL ('b'):
            prepage (ct);
          break;
          case CTRL ('d'):
            halfnextpage (ct);
          break;
          case CTRL ('u'):
            halfprepage (ct);
          break;
          case ':':
          case '/':
          case '?':
            mView->promptcommand (char (key));
          return NEED_MORE;
          case 'H':
            if (mDisplayType == DISPLAY_TYPE_IMAGE)
              {
                scrollto (0.0);
                blank (0, 0);
              }
            else
              scrollwebto (0.0, 0.0);
          break;
          case 'M':
            if (mDisplayType == DISPLAY_TYPE_IMAGE)
              {
                scrollto (0.5);
                blank (0, gint (gtk_adjustment_get_upper (mMainVaj) / 2));
              }
            else
              scrollwebto (0.0, 0.5);
          break;
          case 'L':
            if (mDisplayType == DISPLAY_TYPE_IMAGE)
              {
                scrollto (1.0);
                blank (0, gint (gtk_adjustment_get_upper (mMainVaj)));
              }
            else
              scrollwebto (0.0, 1.0);
          break;
          case '0':
            if (mDisplayType == DISPLAY_TYPE_IMAGE)
              scrollleft (mChars);
            else
              scrollwebto (0.0, 0.0);
          break;
          case '$':
            if (mDisplayType == DISPLAY_TYPE_IMAGE)
              scrollright (mChars);
            else
              scrollwebto (1.0, 0.0);
          break;
          case CTRL ('p'):
          case GDK_KEY_Up:
          case 'k':
            if (mControlContent)
              {
                mContent->scrollup (ct);
                contentShowPage (mContent->currentIndex (), false);
              }
            else
              {
                if (mDisplayType == DISPLAY_TYPE_IMAGE)
                  scrollup (ct);
                else
                  scrollupweb (ct);
              }
          break;
          case CTRL ('n'):
          case CTRL ('j'):
          case GDK_KEY_Down:
          case 'j':
            if (mControlContent)
              {
                mContent->scrolldown (ct);
                contentShowPage (mContent->currentIndex (), false);
              }
            else
              {
                if (mDisplayType == DISPLAY_TYPE_IMAGE)
                  scrolldown (ct);
                else
                  scrolldownweb (ct);
              }
          break;
          case GDK_KEY_BackSpace:
          case GDK_KEY_Left:
          case CTRL ('h'):
          case 'h':
            if (mControlContent)
              {
                mContent->scrollleft (ct);
                contentShowPage (mContent->currentIndex (), false);
              }
            else
              {
                if (mDisplayType == DISPLAY_TYPE_IMAGE)
                  scrollleft (ct);
                else
                  scrollleftweb (ct);
              }
          break;
          case GDK_KEY_space:
          case GDK_KEY_Right:
          case CTRL ('l'):
          case 'l':
            if (mControlContent)
              {
                mContent->scrollright (ct);
                contentShowPage (mContent->currentIndex (), false);
              }
            else
              {
                if (mDisplayType == DISPLAY_TYPE_IMAGE)
                  scrollright (ct);
                else
                  scrollrightweb (ct);
              }
          break;
          case GDK_KEY_Return:
            contentShowPage (mContent->currentIndex (), true);
          break;
          case 'R':
            reload ();
          break;
          case CTRL (']'):
            gotolink (ct);
          break;
          case CTRL ('t'):
            returnlink (ct);
          break;
          case 't':
            mView->newtab (helppdf.c_str ());
          mView->open ();
          break;
          case 'T':
            mView->newtab (helppdf.c_str ());
          mView->opendir ();
          break;
          case 'o':
            mView->open ();
          break;
          case 'O':
            mView->opendir ();
          break;
          case 'r':
            rotate (ct);
          break;
          case 'G':
            markposition ('\'');
          if (!has)
            {
              showpage (mFile->pagesum () - 1, 0.0);
            }
          else
            {
              showpage (ct - 1, 0.0);
            }
          break;
          case 'm':
          case '\'':
          case 'z':
            mProCmd = key;
          return NEED_MORE;
          break;
          case 'n':
            if (mSearchCmd == SEARCH)
              {
                markposition ('\'');
                search ("", false);
              }
            else if (mSearchCmd == BACKSEARCH)
              {
                markposition ('\'');
                search ("", true);
              }
            else
              {
              }
          break;
          case 'N':
            if (mSearchCmd == SEARCH)
              {
                markposition ('\'');
                search ("", true);
              }
            else if (mSearchCmd == BACKSEARCH)
              {
                markposition ('\'');
                search ("", false);
              }
            else
              {
              }
          break;
          case CTRL ('v'):
          case 'v':
            togglevisual (char (key));
          break;
          case ('y'):
            yank (ct);
          mInVisual = VISUAL_NONE;
          blank (mCurx, mCury);
          break;
          case ('s'):
            setskip (ct);
          break;
          case ('c'):
            toggleContent ();
          break;
          default:
            return NO_MATCH;
          break;
        }

      return MATCH;
    }

    ApvlvDoc *ApvlvDoc::copy ()
    {
      char rate[16];
      g_snprintf (rate, sizeof rate, "%f", mZoomrate);
      auto *ndoc = new ApvlvDoc (mView, mDisplayType, rate, usecache ());
      ndoc->loadfile (mFilestr, false, false);
      ndoc->showpage (mPagenum, scrollrate ());
      return ndoc;
    }

    void ApvlvDoc::setzoom (const char *z)
    {
      if (z != nullptr)
        {
          if (strcasecmp (z, "normal") == 0)
            {
              mZoommode = NORMAL;
              mZoomrate = 1.2;
            }
          else if (strcasecmp (z, "fitwidth") == 0)
            {
              mZoommode = FITWIDTH;
            }
          else if (strcasecmp (z, "fitheight") == 0)
            {
              mZoommode = FITHEIGHT;
            }
          else
            {
              double d = strtod (z, nullptr);
              if (d > 0)
                {
                  mZoommode = CUSTOM;
                  mZoomrate = d;
                }
            }
        }

      if (mFile != nullptr)
        {
          gint pn = max (0, pagenumber () - 1);
          mFile->pagesize (pn, mRotatevalue, &mPagex, &mPagey);

          if (mZoommode == FITWIDTH)
            {
              // TODO: need impl by content width
              // mZoomrate = ((double) (mWidth - 26)) / mPagex;
            }
          else if (mZoommode == FITHEIGHT)
            {
              // TODO: need impl by content height
              // mZoomrate = ((double) (mHeight - 26)) / mPagey;
            }

          refresh ();
        }
    }

    bool ApvlvDoc::savelastposition (const char *filename)
    {
      if (filename == nullptr || helppdf == filename || gParams->valueb ("noinfo"))
        {
          return false;
        }

      bool ret = gInfo->file (mPagenum, scrollrate (), filename, mSkip);

      return ret;
    }

    bool ApvlvDoc::loadlastposition (const char *filename)
    {
      if (filename == nullptr || helppdf == filename || gParams->valueb ("noinfo"))
        {
          showpage (0, 0.0);
          return false;
        }

      bool ret = false;

      infofile *fp = gInfo->file (filename);

      // correctly check
      mScrollvalue = fp->rate;
      showpage (fp->page, 0.0);
      setskip (fp->skip);

      return ret;
    }

    bool ApvlvDoc::reload ()
    {
      savelastposition (filename ());
      return loadfile (mFilestr, false, mShowContent);
    }

    bool ApvlvDoc::loadfile (string &filename, bool check, bool show_content)
    {
      return loadfile (filename.c_str (), check, show_content);
    }

    gint ApvlvDoc::pagenumber ()
    {
      if (mContinuous && mCurrentCache2 != nullptr)
        {
          if (scrollrate () > 0.5)
            {
              return gint (mCurrentCache2->getpagenum () + 1);
            }
          else
            {
              return gint (mCurrentCache1->getpagenum () + 1);
            }
        }
      else
        {
          return mPagenum + 1;
        }
    }

    bool ApvlvDoc::usecache ()
    {
      return false;
    }

    void ApvlvDoc::usecache (bool use)
    {
      if (use)
        {
          mView->errormessage ("No pthread, can't support cache!!!");
          mView->infomessage
              ("If you really have pthread, please recomplie the apvlv and try again.");
        }
    }

    bool ApvlvDoc::loadfile (const char *filename, bool check, bool show_content)
    {
      if (check)
        {
          if (strcmp (filename, mFilestr.c_str ()) == 0)
            {
              return false;
            }
        }

      if (mFile)
        {
          delete mFile;
          mFile = nullptr;
        }
      mReady = false;

      mFile = ApvlvFile::newFile (filename, false);

      //debug ("mFile = %p", mFile);
      if (mFile != nullptr)
        {
          mFilestr = filename;

          if (mFile->pagesum () <= 1)
            {
              debug ("pagesum () = %d", mFile->pagesum ());
              mContinuous = false;
              mAutoScrollDoc = false;
              mAutoScrollPage = false;
            }

          //debug ("pagesum () = %d", mFile->pagesum ());

          if (mCurrentCache1 != nullptr)
            {
              delete mCurrentCache1;
              mCurrentCache1 = nullptr;
            }
          mCurrentCache1 = new ApvlvDocCache (mFile);

          if (mCurrentCache2 != nullptr)
            {
              delete mCurrentCache2;
              mCurrentCache2 = nullptr;
            }
          if (mCurrentCache3 != nullptr)
            {
              delete mCurrentCache3;
              mCurrentCache3 = nullptr;
            }

          if (mContinuous)
            {
              mCurrentCache2 = new ApvlvDocCache (mFile);
              mCurrentCache3 = new ApvlvDocCache (mFile);
            }

          loadlastposition (filename);

          mStatus->show (mContinuous);

          setactive (true);

          mReady = true;

          mSearchStr = "";
          if (mSearchResults != nullptr)
            {
              delete mSearchResults;
              mSearchResults = nullptr;
            }

          mInVisual = VISUAL_NONE;

          if (gParams->valuei ("autoreload") > 0)
            {
              if (g_file_test (filename, G_FILE_TEST_IS_SYMLINK))
                {
                  gchar *realname = g_file_read_link (filename, nullptr);
                  if (realname)
                    {
                      mGFile = g_file_new_for_path (realname);
                      g_free (realname);
                    }
                  else
                    {
                      mGFile = nullptr;
                    }
                }
              else
                {
                  mGFile = g_file_new_for_path (filename);
                }

              if (mGFile)
                {
                  GError *error = nullptr;
                  mGMonitor = g_file_monitor_file (mGFile, G_FILE_MONITOR_NONE, nullptr, &error);
                  if (error != nullptr)
                    {
                      debug ("Create file monitor failed: %s\n", error->message);
                      g_error_free (error);
                    }
                }
              else
                {
                  mGMonitor = nullptr;
                }

              if (mGMonitor)
                {
                  g_file_monitor_set_rate_limit (mGMonitor, gParams->valuei ("autoreload") * 1000);
                  g_signal_connect (G_OBJECT (mGMonitor), "changed", G_CALLBACK (apvlv_doc_monitor_callback), this);
                  debug ("connect changed callback to : %p\n", mGMonitor);
                }
            }
        }

      if (show_content && mFile != nullptr)
        {
          toggleContent (true);
        }

      return mFile != nullptr;
    }

    int ApvlvDoc::convertindex (int p)
    {
      if (mFile != nullptr)
        {
          int c = mFile->pagesum ();

          if (p >= 0 && p < c)
            {
              return p;
            }
          else if (p >= c && mAutoScrollDoc)
            {
              return p % c;
            }
          else if (p < 0 && mAutoScrollDoc)
            {
              return c + p;
            }
          else
            {
              return -1;
            }
        }
      return -1;
    }

    void ApvlvDoc::markposition (const char s)
    {
      ApvlvDocPosition adp = {mPagenum, scrollrate ()};
      mPositions[s] = adp;
    }

    void ApvlvDoc::jump (const char s)
    {
      auto it = mPositions.find (s);
      if (it != mPositions.end ())
        {
          ApvlvDocPosition adp = it->second;
          markposition ('\'');
          showpage (adp.pagenum, adp.scrollrate);
        }
    }

    void ApvlvDoc::showpage (int p, double s)
    {
      int rp = convertindex (p);
      if (rp < 0)
        return;

      //debug ("show page: %d | %lf", rp,s);
      mAdjInchg = true;

      if (mAutoScrollPage && mContinuous && !mAutoScrollDoc)
        {
          int rp2 = convertindex (p + 1);
          if (rp2 < 0)
            {
              if (rp == mPagenum + 1)
                {
                  return;
                }
              else
                {
                  rp--;
                }
            }
        }

      mPagenum = rp;

      if (!mZoominit)
        {
          mZoominit = true;
          setzoom (nullptr);
          debug ("zoom rate: %f", mZoomrate);
        }

      refresh ();

      scrollto (s);
    }

    void ApvlvDoc::nextpage (int times)
    {
      showpage (mPagenum + times, 0.0);
    }

    void ApvlvDoc::prepage (int times)
    {
      showpage (mPagenum - times, 0.0);
    }

    void ApvlvDoc::refresh ()
    {
      if (mFile == nullptr)
        return;

      if (mDisplayType == DISPLAY_TYPE_IMAGE)
        {
          mCurrentCache1->set (mPagenum, mZoomrate, mRotatevalue, false);
          GdkPixbuf *buf = mCurrentCache1->getbuf (true);
          gtk_image_set_from_pixbuf (GTK_IMAGE (mImg1), buf);

          if (mAutoScrollPage && mContinuous)
            {
              mCurrentCache2->set (convertindex (mPagenum + 1), mZoomrate,
                                   mRotatevalue, false);
              buf = mCurrentCache2->getbuf (true);
              gtk_image_set_from_pixbuf (GTK_IMAGE (mImg2), buf);

              mCurrentCache3->set (convertindex (mPagenum + 2), mZoomrate,
                                   mRotatevalue, false);
              buf = mCurrentCache3->getbuf (true);
              gtk_image_set_from_pixbuf (GTK_IMAGE (mImg3), buf);
            }
        }
      else if (mDisplayType == DISPLAY_TYPE_HTML)
        {
          mFile->renderweb (mPagenum, 0, 0, mZoomrate, mRotatevalue, mWeb1);
        }
      mStatus->show (mContinuous);
    }

    void ApvlvDoc::srtranslate (int &rtimes, double &sr, bool single2continuous)
    {
      gdouble winv = gtk_adjustment_get_upper (mMainVaj) - gtk_adjustment_get_lower (mMainVaj),
          pagewidth = gtk_adjustment_get_page_size (mMainVaj),
          maxv = winv - pagewidth,
          maxv2 = (winv - 2 * pagewidth) / 2,
          value;

      if (single2continuous)
        {
          value = 0.5 * pagewidth + sr * maxv2;
          sr = (value - 0.5 * pagewidth) / maxv;
        }
      else
        {
          value = 0.5 * pagewidth + sr * maxv;
          if (value > 0.5 * winv)
            {
              rtimes += 1;
              value -= 0.5 * winv;
            }

          if (value > 0.5 * (winv - pagewidth)) sr = 1;
          else if (value > 0.5 * pagewidth) sr = (value - 0.5 * pagewidth) / maxv2;
          else sr = 0;
        }
    }

    void ApvlvDoc::halfnextpage (int times)
    {
      double sr = scrollrate ();
      int rtimes = times / 2;

      if (mAutoScrollPage && mContinuous) srtranslate (rtimes, sr, false);

      if (times % 2 != 0)
        {
          if (sr > 0.5)
            {
              sr = 0;
              rtimes += 1;
            }
          else
            {
              sr = 1;
            }
        }

      if (mAutoScrollPage && mContinuous) srtranslate (rtimes, sr, true);

      showpage (mPagenum + rtimes, sr);
    }

    void ApvlvDoc::halfprepage (int times)
    {
      double sr = scrollrate ();
      int rtimes = times / 2;

      if (mAutoScrollPage && mContinuous) srtranslate (rtimes, sr, false);

      if (times % 2 != 0)
        {
          if (sr < 0.5)
            {
              sr = 1;
              rtimes += 1;
            }
          else
            {
              sr = 0;
            }
        }

      if (mAutoScrollPage && mContinuous) srtranslate (rtimes, sr, true);

      showpage (mPagenum - rtimes, sr);
    }

    void ApvlvDoc::markselection ()
    {
      debug ("mSelect: %d.", mSearchSelect);
      debug ("zoomrate: %f", mZoomrate);

      ApvlvPos rect = (*mSearchResults)[mSearchSelect];

      // Caculate the correct position
      //debug ("pagex: %f, pagey: %f, x1: %f, y1: %f, x2: %f, y2: %f", mPagex, mPagey, rect->x1, rect->y1, rect->x2, rect->y2);
      gint x1 = (gint) ((rect.x1) * mZoomrate);
      gint x2 = (gint) ((rect.x2) * mZoomrate);
      gint y1 = (gint) ((mPagey - rect.y2) * mZoomrate);
      gint y2 = (gint) ((mPagey - rect.y1) * mZoomrate);
      debug ("x1: %d, y1: %d, x2: %d, y2: %d", x1, y1, x2, y2);

      // make the selection at the page center
      gdouble val = ((y1 + y2) - gtk_adjustment_get_page_size (mMainVaj)) / 2;
      debug ("upper: %f, lower: %f, page_size: %f, val: %f",
             gtk_adjustment_get_upper (mMainVaj), gtk_adjustment_get_lower (mMainVaj),
             gtk_adjustment_get_page_size (mMainVaj), val);
      if (val + gtk_adjustment_get_page_size (mMainVaj)
          > gtk_adjustment_get_upper (mMainVaj) - gtk_adjustment_get_lower (mMainVaj) - 5)
        {
          debug ("set value: %f",
                 gtk_adjustment_get_upper (mMainVaj) - gtk_adjustment_get_lower (mMainVaj)
                 - gtk_adjustment_get_page_size (mMainVaj)
                 - 5);
          gtk_adjustment_set_value (mMainVaj, gtk_adjustment_get_upper (mMainVaj) - gtk_adjustment_get_lower (mMainVaj)
                                              - gtk_adjustment_get_page_size (mMainVaj)
                                              - 5);    /* just for avoid the auto scroll page */
        }
      else if (val > 5)
        {
          debug ("set value: %f", val);
          gtk_adjustment_set_value (mMainVaj, val);
        }
      else
        {
          debug ("set value: %f", gtk_adjustment_get_lower (mMainVaj) + 5);
          gtk_adjustment_set_value (mMainVaj, gtk_adjustment_get_lower (mMainVaj) + 5);    /* avoid auto scroll page */
        }

      val = ((x1 + x2) - gtk_adjustment_get_page_size (mMainHaj)) / 2;
      if (val + gtk_adjustment_get_page_size (mMainHaj) > gtk_adjustment_get_upper (mMainHaj))
        {
          gtk_adjustment_set_value (mMainHaj, gtk_adjustment_get_upper (mMainHaj));
        }
      else if (val > 0)
        {
          gtk_adjustment_set_value (mMainHaj, val);
        }
      else
        {
          gtk_adjustment_set_value (mMainHaj, gtk_adjustment_get_lower (mMainHaj));
        }

      mCurrentCache1->set (mPagenum, mZoomrate, mRotatevalue);
      guchar *pagedata = mCurrentCache1->getdata (true);
      GdkPixbuf *pixbuf = mCurrentCache1->getbuf (true);

      mFile->pageselectsearch (mPagenum, mCurrentCache1->getwidth (),
                               mCurrentCache1->getheight (), mZoomrate,
                               mRotatevalue, pixbuf, (char *) pagedata,
                               gint (mSearchSelect), mSearchResults);
      gtk_image_set_from_pixbuf (GTK_IMAGE (mImg1), pixbuf);
      debug ("helight num: %d", mPagenum);
    }

    void ApvlvDoc::scrollup (int times)
    {
      if (!mReady)
        { return; }

      if (!gParams->valueb ("visualmode"))
        {
          ApvlvCore::scrollup (times);
          return;
        }

      gdouble sub = gtk_adjustment_get_upper (mMainVaj) - gtk_adjustment_get_lower (mMainVaj);
      mVrate = sub / mLines;
      gint opage = mPagenum, npage = mPagenum;

      gint ny1 = gint (mCury - mVrate * times);
      if (ny1 < gtk_adjustment_get_value (mMainVaj))
        {
          ApvlvCore::scrollup (times);
          npage = mPagenum;
        }

      if (npage == opage)
        {
          blank (mCurx, ny1);
        }
      else
        {
          if (!gParams->valueb ("continuous"))
            {
              blank (mCurx, gint (mScrollvalue * mCurrentCache1->getheight ()));
            }
          else
            {
              blank (mCurx, gint (gtk_adjustment_get_upper (mMainVaj) / 2));
            }
        }
    }

    void ApvlvDoc::scrolldown (int times)
    {
      if (!mReady)
        { return; }

      if (!gParams->valueb ("visualmode"))
        {
          ApvlvCore::scrolldown (times);
          return;
        }

      gdouble sub = gtk_adjustment_get_upper (mMainVaj) - gtk_adjustment_get_lower (mMainVaj);
      mVrate = sub / mLines;
      gint opage = mPagenum, npage = mPagenum;

      gint ny1 = gint (mCury + mVrate * times);
      if (ny1 - gtk_adjustment_get_value (mMainVaj) >= gtk_adjustment_get_page_size (mMainVaj))
        {
          ApvlvCore::scrolldown (times);
          npage = mPagenum;
        }

      if (npage == opage)
        {
          blank (mCurx, ny1);
        }
      else
        {
          if (!gParams->valueb ("continuous"))
            {
              blank (mCurx, gint (mScrollvalue * mCurrentCache1->getheight ()));
            }
          else
            {
              blank (mCurx, gint (gtk_adjustment_get_upper (mMainVaj) / 2));
            }
        }
    }

    void ApvlvDoc::scrollleft (int times)
    {
      if (!mReady)
        return;

      if (!gParams->valueb ("visualmode"))
        {
          ApvlvCore::scrollleft (times);
          return;
        }

      gdouble sub = gtk_adjustment_get_upper (mMainHaj) - gtk_adjustment_get_lower (mMainHaj);
      mHrate = sub / mChars;

      gint nx1 = gint (mCurx - mHrate * times);
      if (nx1 < gtk_adjustment_get_upper (mMainHaj) - gtk_adjustment_get_page_size (mMainHaj))
        {
          ApvlvCore::scrollleft (times);
        }
      blank (nx1, mCury);
    }

    void ApvlvDoc::scrollright (int times)
    {
      if (!mReady)
        return;

      if (!gParams->valueb ("visualmode"))
        {
          ApvlvCore::scrollright (times);
          return;
        }

      gdouble sub = gtk_adjustment_get_upper (mMainHaj) - gtk_adjustment_get_lower (mMainHaj);
      mHrate = sub / mChars;

      gint nx1 = gint (mCurx + mHrate * times);
      if (nx1 > gtk_adjustment_get_page_size (mMainHaj))
        {
          ApvlvCore::scrollright (times);
        }
      blank (nx1, mCury);
    }

    void ApvlvDoc::scrollweb (int times, int h, int v)
    {
      if (!mReady)
        return;

      gchar *javasrc = g_strdup_printf ("window.scrollBy(%d, %d);", times * h, times * v);
      webkit_web_view_run_javascript (WEBKIT_WEB_VIEW (mWeb1),
                                      javasrc,
                                      nullptr, nullptr, this);
      g_free (javasrc);
    }

    void ApvlvDoc::scrollwebto (double xrate, double yrate)
    {
      if (!mReady)
        return;

      gchar *javasrc = g_strdup_printf ("window.scroll(window.screenX * %f, window.screenY * %f);", xrate, yrate);
      webkit_web_view_run_javascript (WEBKIT_WEB_VIEW (mWeb1),
                                      javasrc,
                                      nullptr, nullptr, this);
      g_free (javasrc);
    }

    void ApvlvDoc::scrollupweb (int times)
    {
      scrollweb (times, 0, -50);
    }

    void ApvlvDoc::scrolldownweb (int times)
    {
      scrollweb (times, 0, 50);
    }

    void ApvlvDoc::scrollleftweb (int times)
    {
      scrollweb (times, -50, 0);
    }

    void ApvlvDoc::scrollrightweb (int times)
    {
      scrollweb (times, 50, 0);
    }

    bool ApvlvDoc::needsearch (const char *str, bool reverse)
    {
      if (mFile == nullptr)
        return false;

      // search a different string
      if (strlen (str) > 0 && strcmp (str, mSearchStr.c_str ()) != 0)
        {
          debug ("different string.");
          mSearchSelect = 0;
          mSearchStr = str;
          return true;
        }

      else if (mSearchResults == nullptr)
        {
          debug ("no result.");
          mSearchSelect = 0;
          return true;
        }

        // same string, but need to search next page
      else if ((mSearchPagenum != mPagenum)
               || ((mSearchReverse == reverse)
                   && mSearchSelect == mSearchResults->size () - 1)
               || ((mSearchReverse != reverse) && mSearchSelect == 0))
        {
          debug
          ("same, but need next string: S: %d, s: %d, sel: %d, max: %d.",
           mSearchReverse, reverse, mSearchSelect, mSearchResults->size ());
          mSearchSelect = 0;
          return true;
        }

        // same string, not need search, but has zoomed
      else
        {
          debug
          ("same, not need next string. sel: %d, max: %u",
           mSearchSelect, mSearchResults->size ());
          if (mSearchReverse == reverse)
            {
              mSearchSelect++;
            }
          else
            {
              mSearchSelect--;
            }

          markselection ();
          return false;
        }
    }

    bool ApvlvDoc::search (const char *str, bool reverse)
    {
      if (*str == '\0' && mSearchStr.empty ())
        {
          return false;
        }

      if (*str != '\0')
        {
          mSearchCmd = reverse ? BACKSEARCH : SEARCH;
        }

      if (!needsearch (str, reverse))
        {
          return true;
        }

      if (mSearchResults != nullptr)
        {
          delete mSearchResults;
          mSearchResults = nullptr;
        }

      bool wrap = gParams->valueb ("wrapscan");

      int i = mPagenum;
      int sum = mFile->pagesum (), from = i;
      bool search = false;
      while (true)
        {
          if (*str != 0 || search)
            {
              mSearchResults =
                  mFile->pagesearch ((i + sum) % sum, mSearchStr.c_str (),
                                     reverse);
              mSearchReverse = reverse;
              if (mSearchResults != nullptr)
                {
                  showpage ((i + sum) % sum, 0.5);
                  mSearchPagenum = mPagenum;
                  markselection ();
                  return true;
                }
            }

          search = true;

          if (!reverse && i < (wrap ? (from + sum) : (sum - 1)))
            {
              //          debug ("wrap: %d, i++:", wrap, i, i + 1);
              i++;
            }
          else if (reverse && i > (wrap ? (from - sum) : 0))
            {
              debug ("wrap: %d, i--:", wrap, i, i - 1);
              i--;
            }
          else
            {
              mView->errormessage ("can't find word: '%s'",
                                   mSearchStr.c_str ());
              return false;
            }
        }
    }

    bool ApvlvDoc::totext (const char *file)
    {
      if (mFile == nullptr)
        return false;

      char *txt;
      bool ret = mFile->pagetext (mPagenum, 0, 0, mCurrentCache1->getwidth (),
                                  mCurrentCache1->getheight (), &txt);
      if (ret)
        {
          g_file_set_contents (file, txt, -1, nullptr);
          return true;
        }
      return false;
    }

    void ApvlvDoc::setactive (bool act)
    {
      mStatus->active (act);
      mActive = act;
    }

    bool ApvlvDoc::rotate (int ct)
    {
      // just hack
      if (ct == 1)
        ct = 90;

      if (ct % 90 != 0)
        {
          mView->errormessage ("Not a 90 times value: %d", ct);
          return false;
        }

      mRotatevalue += ct;
      while (mRotatevalue < 0)
        {
          mRotatevalue += 360;
        }
      while (mRotatevalue > 360)
        {
          mRotatevalue -= 360;
        }
      refresh ();
      return true;
    }

    void ApvlvDoc::gotolink (int ct)
    {
      ApvlvLinks *links1 = mCurrentCache1->getlinks ();
      ApvlvLinks *links2 = mCurrentCache2 ? mCurrentCache2->getlinks () : nullptr;

      int siz = links1 ? int (links1->size ()) : 0;
      siz += links2 ? int (links2->size ()) : 0;

      ct--;

      if (ct >= 0 && ct < siz)
        {
          markposition ('\'');

          ApvlvDocPosition p = {mPagenum, scrollrate ()};
          mLinkPositions.push_back (p);

          if (links1 == nullptr)
            {
              showpage ((*links2)[ct].mPage, 0.0);
            }
          else
            {
              if (ct < (int) links1->size ())
                {
                  showpage ((*links1)[ct].mPage, 0.0);
                }
              else
                {
                  showpage ((*links2)[ct - links1->size ()].mPage, 0.0);
                }
            }
        }
    }

    void ApvlvDoc::returnlink (int ct)
    {
      debug ("Ctrl-t %d", ct);
      if (ct <= (int) mLinkPositions.size () && ct > 0)
        {
          markposition ('\'');
          ApvlvDocPosition p = {0, 0};
          while (ct-- > 0)
            {
              p = mLinkPositions[mLinkPositions.size () - 1];
              mLinkPositions.pop_back ();
            }
          showpage (p.pagenum, p.scrollrate);
        }
    }

    bool ApvlvDoc::print (int ct)
    {
#ifdef WIN32
      return false;
#else
      bool ret = false;
      GtkPrintOperation *print = gtk_print_operation_new ();

      gtk_print_operation_set_allow_async (print, TRUE);
      gtk_print_operation_set_show_progress (print, TRUE);

      auto *data = new PrintData;
      data->file = mFile;
      data->frmpn = mPagenum;
      data->endpn = mPagenum + (ct > 0 ? ct : 1) - 1;
      if ((int) data->endpn >= mFile->pagesum ())
        {
          data->endpn = mFile->pagesum () - 1;
        }
      //If nothing is specified, print all pages
      if (ct == -1)
        {
          data->frmpn = 0;
          data->endpn = mFile->pagesum () - 1;
          //revert to +ve value, since I don't know if ct is assumed to be +ve anywhere
          ct = mFile->pagesum ();
        }

      g_signal_connect (G_OBJECT (print), "begin-print",
                        G_CALLBACK (begin_print), data);
      g_signal_connect (G_OBJECT (print), "draw-page", G_CALLBACK (draw_page),
                        data);
      g_signal_connect (G_OBJECT (print), "end-print", G_CALLBACK (end_print),
                        data);
      if (settings != nullptr)
        {
          gtk_print_operation_set_print_settings (print, settings);
        }
      int r =
          gtk_print_operation_run (print, GTK_PRINT_OPERATION_ACTION_PRINT_DIALOG,
                                   GTK_WINDOW (mView->widget ()), nullptr);
      if (r == GTK_PRINT_OPERATION_RESULT_APPLY)
        {
          if (settings != nullptr)
            {
              g_object_unref (settings);
            }
          settings = gtk_print_operation_get_print_settings (print);
          ret = true;
        }
      g_object_unref (print);
      return ret;
#endif
    }

    void ApvlvDoc::apvlv_doc_on_mouse (GtkAdjustment *adj, ApvlvDoc *doc)
    {
      if (doc->mAdjInchg)
        {
          doc->mAdjInchg = false;
          return;
        }

      if (gtk_adjustment_get_upper (adj) - gtk_adjustment_get_lower (adj)
          == gtk_adjustment_get_page_size (adj) + gtk_adjustment_get_value (adj))
        {
          doc->scrolldown (1);
        }
      else if (gtk_adjustment_get_value (adj) == 0)
        {
          doc->scrollup (1);
        }
    }

#ifndef WIN32
    void
    ApvlvDoc::begin_print (GtkPrintOperation *operation,
                           GtkPrintContext *context, PrintData *data)
    {
      gtk_print_operation_set_n_pages (operation,
                                       gint (data->endpn - data->frmpn + 1));
    }

    void
    ApvlvDoc::draw_page (GtkPrintOperation *operation,
                         GtkPrintContext *context,
                         gint page_nr, PrintData *data)
    {
      cairo_t *cr = gtk_print_context_get_cairo_context (context);
      data->file->pageprint (gint (data->frmpn + page_nr), cr);
      PangoLayout *layout = gtk_print_context_create_pango_layout (context);
      pango_cairo_show_layout (cr, layout);
      g_object_unref (layout);
    }

    void
    ApvlvDoc::end_print (GtkPrintOperation *operation,
                         GtkPrintContext *context, PrintData *data)
    {
      delete data;
    }
#endif

    void
    ApvlvDoc::apvlv_doc_monitor_callback (GFileMonitor *fm, GFile *gf1, GFile *gf2, GFileMonitorEvent ev, ApvlvDoc *doc)
    {
      if (!doc->mInuse)
        {
          return;
        }

      if (ev == G_FILE_MONITOR_EVENT_CHANGED)
        {
          doc->mView->errormessage ("Contents is modified, apvlv reload it automatically");
          doc->reload ();
        }
    }

    void
    ApvlvDoc::apvlv_doc_button_event (GtkEventBox *box,
                                      GdkEventButton *button, ApvlvDoc *doc)
    {
      if (button->button == 1)
        {
          if (button->type == GDK_BUTTON_PRESS)
            {
              // this is a manual method to test double click
              // I think, a normal user double click will in 500 millseconds
              if (button->time - doc->mLastpress < 500)
                {
                  doc->mInVisual = VISUAL_NONE;
                  double rx, ry;
                  doc->eventpos (button->x, button->y, &rx, &ry);
                  doc->blankaction (rx, ry);
                }
              else
                {
                  doc->mBlankx1 = gint (button->x);
                  doc->mBlanky1 = gint (button->y);
                  doc->mInVisual = VISUAL_NONE;
                  double rx, ry;
                  doc->eventpos (button->x, button->y, &rx, &ry);
                  doc->blank (gint (rx), gint (ry));
                }

              doc->mLastpress = gint (button->time);
            }
        }
      else if (button->button == 3)
        {
          if (doc->mInVisual == VISUAL_NONE)
            {
              return;
            }

          doc->mInVisual = VISUAL_NONE;

          GtkWidget *menu, *item;

          menu = gtk_menu_new ();
          gtk_menu_attach_to_widget (GTK_MENU (menu), GTK_WIDGET (box), nullptr);

          item = gtk_menu_item_new_with_label ("Copy to Clipboard");
          gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
          gtk_widget_show (item);

          g_signal_connect (item, "activate",
                            G_CALLBACK (apvlv_doc_copytoclipboard_cb), doc);

#if GTK_CHECK_VERSION(3, 22, 0)
          gtk_menu_popup_at_pointer (GTK_MENU (menu), nullptr);
#else
          gtk_menu_popup (GTK_MENU (menu), nullptr, nullptr, nullptr, nullptr, 0, 0);
#endif
        }
    }

    void
    ApvlvDoc::apvlv_doc_copytoclipboard_cb (GtkMenuItem *item,
                                            ApvlvDoc *doc)
    {
      doc->yank (1);
      doc->mInVisual = VISUAL_NONE;
      doc->blank (doc->mCurx, doc->mCury);
    }

    void
    ApvlvDoc::apvlv_doc_motion_event (GtkWidget *box,
                                      GdkEventMotion *motion, ApvlvDoc *doc)
    {
      doc->mInVisual = VISUAL_V;
      double rx, ry;
      doc->eventpos (motion->x, motion->y, &rx, &ry);
      doc->blank (gint (rx), gint (ry));
    }

    void
    ApvlvDoc::webview_resource_load_started_cb (WebKitWebView *web_view,
                                                WebKitWebResource *resource,
                                                WebKitURIRequest *request,
                                                ApvlvDoc *doc)
    {
      debug("resource: %s, request: %s", webkit_web_resource_get_uri (resource), webkit_uri_request_get_uri (request));
    }

    void
    ApvlvDoc::webview_load_changed_cb (WebKitWebView *web_view,
                                       WebKitLoadEvent event,
                                       ApvlvDoc *doc)
    {
      if (event == WEBKIT_LOAD_FINISHED)
        {
          string anchor = doc->mFile->get_anchor ();
          if (!anchor.empty ())
            {
              gchar *javasrc = g_strdup_printf ("document.getElementById('%s').scrollIntoView();",
                                                anchor.c_str ());
              webkit_web_view_run_javascript (web_view,
                                              javasrc,
                                              nullptr, nullptr, doc);
              g_free (javasrc);
            }
        }
    }

    gboolean
    ApvlvDoc::webview_context_menu_cb (WebKitWebView *web_view,
                                       WebKitContextMenu *context_menu,
                                       GdkEvent *event,
                                       WebKitHitTestResult *hit_test_result,
                                       ApvlvDoc *doc)
    {
      return TRUE;
    }

    void
    ApvlvDoc::eventpos (double x, double y, double *rx, double *ry)
    {
      int dw, dh;
      if (rx != nullptr)
        {
          GtkAllocation allocation;
          gtk_widget_get_allocation (mMainWidget, &allocation);
          dw = gint (mPagex * mZoomrate) - allocation.width;
          dw = dw >> 1;
          if (dw >= 0)
            {
              dw = 0;
            }
          *rx = x + dw;
        }

      if (ry != nullptr)
        {
          GtkAllocation allocation;
          gtk_widget_get_allocation (mMainWidget, &allocation);
          dh = gint (mPagey * mZoomrate) - allocation.height;
          dh = dh >> 1;
          if (dh >= 0)
            {
              dh = 0;
            }
          *ry = y + dh;
        }
    }

    void ApvlvDoc::contentShowPage (ApvlvFileIndex *index, bool force)
    {
      if (index == nullptr)
        return;

      if (index->type == ApvlvFileIndexType::DIR)
        return;

      auto follow_mode = "always";
      if (!force)
        {
          follow_mode = gParams->values ("content_follow_mode");
        }

      if (g_ascii_strcasecmp (follow_mode, "none") == 0)
        {
          return;
        }

      else if (g_ascii_strcasecmp (follow_mode, "page") == 0)
        {
          if (index->type == ApvlvFileIndexType::PAGE)
            {
              if (index->page != mPagenum)
                showpage (index->page, 0.0);
            }
          return;
        }

      else if (g_ascii_strcasecmp (follow_mode, "always") == 0)
        {
          if (index->type == ApvlvFileIndexType::PAGE)
            {
              if (index->page != mPagenum)
                showpage (index->page, 0.0);
            }
          else
            {
              if (index->path != filename ())
                {
                  loadfile (index->path, true, false);
                }
            }
        }
    }

    ApvlvDocCache::ApvlvDocCache (ApvlvFile *file)
    {
      mFile = file;
      mPagenum = -1;
      mLines = nullptr;
      mData = nullptr;
      mSize = 0;
      mBuf = nullptr;
      mLinks = nullptr;
      mInverted = false;
      mZoom = 1.0;
      mRotate = 0;
      mWidth = 0;
      mHeight = 0;
    }

    void ApvlvDocCache::set (guint p, double zm, guint rot, bool delay)
    {
      mPagenum = gint (p);
      mZoom = zm;
      mRotate = rot;

      if (mLines != nullptr)
        {
          delete mLines;
          mLines = nullptr;
        }

      if (mData != nullptr)
        {
          delete[]mData;
          mData = nullptr;
        }
      if (mBuf != nullptr)
        {
          g_object_unref (mBuf);
          mBuf = nullptr;
        }
      if (mLinks != nullptr)
        {
          delete mLinks;
          mLinks = nullptr;
        }

      mInverted = gParams->valueb ("inverted");
      load (this);
    }

    void ApvlvDocCache::load (ApvlvDocCache *ac)
    {
      int c = ac->mFile->pagesum ();

      if (ac->mPagenum < 0 || ac->mPagenum >= c)
        {
          debug ("no this page: %d", ac->mPagenum);
          return;
        }

      double tpagex, tpagey;
      ac->mFile->pagesize (ac->mPagenum, gint (ac->mRotate), &tpagex, &tpagey);

      ac->mWidth = MAX ((tpagex * ac->mZoom + 0.5), 1);
      ac->mHeight = MAX ((tpagey * ac->mZoom + 0.5), 1);

      // this is very import to get the double times size data
      // the 2ed chunk data will be for output
      ac->mSize = ac->mWidth * ac->mHeight * 3;
      auto *dat = new guchar[2 * ac->mSize];

      GdkPixbuf *bu = gdk_pixbuf_new_from_data (dat, GDK_COLORSPACE_RGB,
                                                FALSE,
                                                8,
                                                ac->mWidth, ac->mHeight,
                                                3 * ac->mWidth,
                                                nullptr, nullptr);
      //debug ("ac->mFile: %p", ac->mFile);
      ac->mFile->render (ac->mPagenum, ac->mWidth, ac->mHeight, ac->mZoom,
                         gint (ac->mRotate), bu, (char *) dat);
      if (ac->mInverted)
        {
          invert_pixbuf (bu);
        }
      // backup the pixbuf data
      memcpy (dat + ac->mSize, dat, ac->mSize);

      delete ac->mLinks;

      ac->mLinks = ac->mFile->getlinks (ac->mPagenum);
      //debug ("has mLinkMappings: %p", ac->mLinks);

      ac->mData = dat;
      ac->mBuf = bu;

      ac->preparelines (0, 0, gint (tpagex), gint (tpagey));
    }

    ApvlvDocCache::~ApvlvDocCache ()
    {
      delete mLinks;

      delete mLines;

      delete[]mData;

      if (mBuf != nullptr)
        {
          g_object_unref (mBuf);
        }
    }

    guint ApvlvDocCache::getpagenum () const
    {
      return mPagenum;
    }

    /*
     * get the cache data
     * @param: wait, if not wait, not wait the buffer be prepared
     * @return: the buffer
     * */
    guchar *ApvlvDocCache::getdata (bool wait)
    {
      memcpy (mData, mData + mSize, mSize);
      return mData;
    }

    /*
     * get the cache GdkPixbuf
     * @param: wait, if not wait, not wait the pixbuf be prepared
     * @return: the buffer
     * */
    GdkPixbuf *ApvlvDocCache::getbuf (bool wait)
    {
      return mBuf;
    }

    gint ApvlvDocCache::getwidth () const
    {
      return mWidth;
    }

    gint ApvlvDocCache::getheight () const
    {
      return mHeight;
    }

    ApvlvLinks *ApvlvDocCache::getlinks ()
    {
      return mLinks;
    }

    ApvlvWord *ApvlvDocCache::getword (int x, int y)
    {
      ApvlvLine *line;

      line = getline (x, y);
      if (line != nullptr)
        {
          for (auto &mWord : line->mWords)
            {
              debug ("itr: %f,%f", itr->pos.y1, itr->pos.y2);
              if (x >= mWord.pos.x1 && x <= mWord.pos.x2)
                {
                  return &mWord;
                }
            }
        }
      return nullptr;
    }

    ApvlvLine *ApvlvDocCache::getline (double x, double y)
    {
      debug ("getline: %f", y);

      if (mLines == nullptr)
        {
          return nullptr;
        }

      for (auto &mLine : *mLines)
        {
          debug ("itr: %f,%f", itr->pos.y1, itr->pos.y2);
          if (y >= mLine.pos.y2 && y <= mLine.pos.y1)
            {
              return &mLine;
            }
        }
      return nullptr;
    }

    void ApvlvDocCache::preparelines (gint x1, gint y1, gint x2,
                                      gint y2)
    {
      if (strcmp (gParams->values ("doubleclick"), "page") == 0
          || strcmp (gParams->values ("doubleclick"), "none") == 0)
        {
          return;
        }

      gchar *content = nullptr;
      mFile->pagetext (mPagenum, x1, y1, x2, y2, &content);
      if (content != nullptr)
        {
          ApvlvPoses *results;
          string word;

          mLines = new vector<ApvlvLine>;

          ApvlvPos lastpos = {0, 0, 0, 0};

          if (strcmp (gParams->values ("doubleclick"), "word") == 0)
            {
              stringstream ss (content);
              while (!ss.eof ())
                {
                  ss >> word;
                  results = mFile->pagesearch (mPagenum, word.c_str (), false);
                  if (results != nullptr)
                    {
                      lastpos = prepare_add (lastpos, results, word.c_str ());
                      delete results;
                    }
                }
            }
          else
            {
              gchar **v, *p;
              int i;

              v = g_strsplit (content, "\n", -1);
              if (v != nullptr)
                {
                  for (i = 0; v[i] != nullptr; ++i)
                    {
                      p = v[i];
                      while (*p != '\0' && isspace (*p))
                        {
                          p++;
                        }
                      if (*p == '\0')
                        {
                          continue;
                        }
                      debug ("search [%s]", p);
                      results = mFile->pagesearch (mPagenum, p, false);
                      if (results != nullptr)
                        {
                          lastpos = prepare_add (lastpos, results, p);
                          delete results;
                        }
                    }
                  g_strfreev (v);
                }
            }

          for (auto &mLine : *mLines)
            {
              //              debug ("line: %f, %f, %f, %f", it->pos.x1, it->pos.y1,
              //                     it->pos.x2, it->pos.y2);
              for (auto wit = mLine.mWords.begin (); wit != mLine.mWords.end (); wit++)
                {
                  //                 debug ("word: %f, %f, %f, %f: %s", wit->pos.x1, wit->pos.y1,
                  //                         wit->pos.x2, wit->pos.y2, wit->word.c_str ());
                }
            }
          g_free (content);
        }
    }

    ApvlvPos ApvlvDocCache::prepare_add (ApvlvPos &lastpos,
                                         ApvlvPoses *results, const char *word)
    {
      ApvlvPoses::iterator itr;
      for (itr = results->begin (); itr != results->end (); itr++)
        {
          itr->x1 *= mZoom;
          itr->x2 *= mZoom;
          itr->y1 = mHeight - itr->y1 * mZoom;
          itr->y2 = mHeight - itr->y2 * mZoom;

          if ((lastpos.y2 < itr->y2)
              || (lastpos.y2 - itr->y2 < 0.0001 && lastpos.x2 < itr->x2))
            {
              debug ("[%s] x1:%f, x2:%f, y1:%f, y2:%f", word, itr->x1,
                     itr->x2, itr->y1, itr->y2);
              break;
            }
        }

      if (itr == results->end ())
        {
          itr = results->begin ();
        }

      //      debug ("itr: %f, %f, %f, %f", itr->x1, itr->y1, itr->x2, itr->y2);
      vector<ApvlvLine>::iterator litr;
      for (litr = mLines->begin (); litr != mLines->end (); litr++)
        {
          //          debug ("litr: %f, %f", litr->pos.y1, litr->pos.y2);
          if (fabs (itr->y1 - litr->pos.y1) < 0.0001
              && fabs (itr->y2 - litr->pos.y2) < 0.0001)
            {
              break;
            }
        }

      ApvlvWord aword = {*itr};
      if (litr != mLines->end ())
        {
          litr->mWords.push_back (aword);
          if (itr->x1 < litr->pos.x1)
            {
              litr->pos.x1 = itr->x1;
            }
          if (itr->x2 > litr->pos.x2)
            {
              litr->pos.x2 = itr->x2;
            }
        }
      else
        {
          ApvlvLine line;
          line.pos = *itr;
          line.mWords.push_back (aword);
          mLines->push_back (line);
        }

      return *itr;
    }

    ApvlvDocStatus::ApvlvDocStatus (ApvlvDoc *doc)
    {
      mDoc = doc;
      for (auto &i : mStlab)
        {
          i = gtk_label_new ("");
          gtk_box_pack_start (GTK_BOX (mHbox), i, TRUE, TRUE, 0);
        }
    }

    ApvlvDocStatus::~ApvlvDocStatus ()
    = default;

    void ApvlvDocStatus::active (bool act)
    {
      for (auto &i : mStlab)
        {
#if GTK_CHECK_VERSION(3, 0, 0)
          gtk_widget_set_state_flags (i,
                                      (act) ? GTK_STATE_FLAG_ACTIVE :
                                      GTK_STATE_FLAG_INSENSITIVE, TRUE);
#else
          gtk_widget_modify_fg (mStlab[i],
                                (act) ? GTK_STATE_ACTIVE:
                                GTK_STATE_INSENSITIVE, nullptr);
#endif
        }
    }

    void ApvlvDocStatus::show (bool mContinuous)
    {
      if (mDoc->filename ())
        {
          gint pn = mDoc->pagenumber (),
              totpn = mDoc->file ()->pagesum ();
          gdouble sr = mDoc->scrollrate ();
          int tmprtimes = 0;
          mDoc->srtranslate (tmprtimes, sr, false);

          char temp[AD_STATUS_SIZE][256];
          gchar *bn;
          bn = g_path_get_basename (mDoc->filename ());
          g_snprintf (temp[0], sizeof temp[0], "%s", bn);
          g_snprintf (temp[1], sizeof temp[1], "%d/%d", pn, totpn);
          g_snprintf (temp[2], sizeof temp[2], "%d%%",
                      (int) (mDoc->zoomvalue () * 100));
          g_snprintf (temp[3], sizeof temp[3], "%d%%",
                      (int) ((sr + pn - 1.0) / totpn * 100));
          for (unsigned int i = 0; i < AD_STATUS_SIZE; ++i)
            {
              gtk_label_set_text (GTK_LABEL (mStlab[i]), temp[i]);
            }
          g_free (bn);
        }
    }

    static void invert_pixbuf (GdkPixbuf *pixbuf)
    {
      guchar *data, *p;
      guint width, height, x, y, rowstride, n_channels;

      n_channels = gdk_pixbuf_get_n_channels (pixbuf);
      g_assert (gdk_pixbuf_get_colorspace (pixbuf) == GDK_COLORSPACE_RGB);
      g_assert (gdk_pixbuf_get_bits_per_sample (pixbuf) == 8);

      /* First grab a pointer to the raw pixel data. */
      data = gdk_pixbuf_get_pixels (pixbuf);

      /* Find the number of bytes per row (could be padded). */
      rowstride = gdk_pixbuf_get_rowstride (pixbuf);

      width = gdk_pixbuf_get_width (pixbuf);
      height = gdk_pixbuf_get_height (pixbuf);
      for (x = 0; x < width; x++)
        {
          for (y = 0; y < height; y++)
            {
              /* Calculate pixel's offset into the data array. */
              p = data + x * n_channels + y * rowstride;
              /* Change the RGB values */
              p[0] = 255 - p[0];
              p[1] = 255 - p[1];
              p[2] = 255 - p[2];
            }
        }
    }
}

// Local Variables:
// mode: c++
// End:

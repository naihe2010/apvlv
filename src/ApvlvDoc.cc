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

#include "ApvlvDoc.h"
#include "ApvlvInfo.h"
#include "ApvlvParams.h"
#include "ApvlvUtil.h"
#include "ApvlvView.h"

#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>
#include <webkit2/webkit2.h>

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <set>
#include <sstream>

namespace apvlv
{
static void invert_pixbuf (GdkPixbuf *);
static GtkPrintSettings *settings = nullptr;
const int APVLV_CURSOR_WIDTH_DEFAULT = 2;

DISPLAY_TYPE
get_display_type_by_filename (const char *name)
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

ApvlvDoc::ApvlvDoc (ApvlvView *view, const char *zm, bool cache)
    : ApvlvCore (view)
{
  mReady = false;

  mAdjInchg = false;

  mAutoScrollPage = gParams->valueb ("autoscrollpage");
  mAutoScrollDoc = gParams->valueb ("autoscrolldoc");
  mContinuous = gParams->valueb ("continuous");

  mZoominit = false;

  mProCmd = 0;

  mInVisual = false;

  mLastPoint = { 0, 0 };
  mCurPoint = { 0, 0 };

  mLastpress = 0;

  mRotatevalue = 0;

  mFile = nullptr;

  mSearchResults = nullptr;
  mSearchStr = "";

  mCurrentImage = nullptr;

  mCurrentCache[0] = mCurrentCache[1] = mCurrentCache[2] = nullptr;

  if (mContinuous && gParams->valuei ("continuouspad") > 0)
    {
      mVbox = gtk_box_new (GTK_ORIENTATION_VERTICAL,
                           gParams->valuei ("continuouspad"));
    }
  else
    {
      mVbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
    }

  gtk_container_add (GTK_CONTAINER (mMainWidget), mVbox);

  mDisplayType = DISPLAY_TYPE_IMAGE;

  mImg[0] = new ApvlvImage (this, 0);
  gtk_box_pack_start (GTK_BOX (mVbox), mImg[0]->widget (), TRUE, TRUE, 0);
  if (mAutoScrollPage && mContinuous)
    {
      mImg[1] = new ApvlvImage (this, 1);
      gtk_box_pack_start (GTK_BOX (mVbox), mImg[1]->widget (), TRUE, TRUE, 0);
      mImg[2] = new ApvlvImage (this, 2);
      gtk_box_pack_start (GTK_BOX (mVbox), mImg[2]->widget (), TRUE, TRUE, 0);
    }
  else
    {
      mImg[1] = nullptr;
      mImg[2] = nullptr;
    }

  mWeb[0] = webkit_web_view_new ();
  g_object_ref (mWeb[0]);
  g_signal_connect (mWeb[0], "resource-load-started",
                    G_CALLBACK (webview_resource_load_started_cb), this);
  g_signal_connect (mWeb[0], "load-changed",
                    G_CALLBACK (webview_load_changed_cb), this);
  g_signal_connect (mWeb[0], "context-menu",
                    G_CALLBACK (webview_context_menu_cb), this);

  g_signal_connect (G_OBJECT (mMainVaj), "value-changed",
                    G_CALLBACK (apvlv_doc_on_mouse), this);
}

ApvlvDoc::~ApvlvDoc ()
{
  for (auto cache : mCurrentCache)
    {
      delete cache;
    }

  for (auto img : mImg)
    {
      delete img;
    }

  g_object_unref (mWeb[0]);

  savelastposition (filename ());
  mPositions.clear ();

  delete mFile;

  delete mStatus;
}

void
ApvlvDoc::blankarea (ApvlvImage *image, ApvlvPos pos, guchar *buffer,
                     int width, int height)
{
  int x1 = int (pos.x1), x2 = int (pos.x2);
  int y1 = int (pos.y1), y2 = int (pos.y2);
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
          gint p = (gint)(y * width * 3 + (x * 3));
          buffer[p + 0] = 0xff - buffer[p + 0];
          buffer[p + 1] = 0xff - buffer[p + 1];
          buffer[p + 2] = 0xff - buffer[p + 2];
        }
    }
}

void
ApvlvDoc::doubleClickBlank (ApvlvImage *image, double x, double y)
{
  auto cache = mCurrentCache[image->mId];
  ApvlvPos pos = { x, x, y, y };

  if (strcasecmp (gParams->values ("doubleclick"), "word") == 0)
    {
      ApvlvWord *word;
      word = cache->getword (x, y);
      if (word != nullptr)
        {
          debug ("find word: %s, [%.0f, %.0f, %.0f, %.0f]\n",
                 word->word.c_str (), word->pos.x1, word->pos.y1, word->pos.x2,
                 word->pos.y2);
          pos = word->pos;
        }
    }
  else if (strcasecmp (gParams->values ("doubleclick"), "line") == 0)
    {
      ApvlvLine *line;
      line = cache->getline (y);
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
  updateLastPoint (pos.x1, pos.y1);
  updateCurPoint (pos.x2, pos.y2, FALSE);
  blank (image);
}

void
ApvlvDoc::blank (ApvlvImage *image)
{
  g_return_if_fail (image != nullptr);

  auto cache = mCurrentCache[image->mId];
  auto poses = cache->getSelected (mLastPoint, mCurPoint, mInVisual);
  auto buffer = cache->getdata (true);

  for (auto pos : poses)
    {
      blankarea (image, pos, buffer, cache->getwidth (), cache->getheight ());
    }

  GdkPixbuf *p = cache->getbuf (true);
  image->setFromPixbuf (p);
}

void
ApvlvDoc::togglevisual (int key)
{
  if (!gParams->valueb ("visualmode"))
    {
      return;
    }

  if (mInVisual == VISUAL_NONE)
    {
      updateLastPoint (mCurPoint.x, mCurPoint.y);
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

  blank (mCurrentImage);
}

void
ApvlvDoc::yank (ApvlvImage *image, int times)
{
  g_return_if_fail (image);
  auto cache = mCurrentCache[image->mId];
  auto poses = cache->getSelected (mLastPoint, mCurPoint, mInVisual);
  string content;
  gchar *text;
  for (auto pos : poses)
    {
      if (mFile->pagetext (cache->getpagenum (), pos.x1, pos.y1, pos.x2,
                           pos.y2, &text))
        {
          content.append (text);
          g_free (text);
        }
    }

  debug ("selected \n[%s]\n", content.c_str ());

  GtkClipboard *cb = gtk_clipboard_get (nullptr);
  gtk_clipboard_set_text (cb, content.c_str (), gint (content.length ()));
}

void
ApvlvDoc::annotUnderline (ApvlvImage *image)
{
  auto cache = mCurrentCache[image->mId];
  auto poses = cache->getSelected (mLastPoint, mCurPoint, mInVisual);
  for (auto pos : poses)
    {
      mFile->annot_underline (cache->getpagenum (), pos.x1, pos.y2, pos.x2,
                              pos.y2);
    }

  refresh ();
}
void
ApvlvDoc::annotText (ApvlvImage *image)
{
  auto cache = mCurrentCache[image->mId];
  auto poses = cache->getSelected (mLastPoint, mCurPoint, mInVisual);
  if (poses.empty ())
    return;

  auto pos = poses[poses.size () - 1];
  auto text = ApvlvView::input ("Comment: ", mZoomrate * (pos.x2 - pos.x1),
                                mZoomrate * (pos.y2 - pos.y1));
  if (text)
    {
      mFile->annot_text (cache->getpagenum (), pos.x1,
                         cache->getheight () - pos.y1, pos.x2,
                         cache->getheight () - pos.y2, text);
      g_free (text);
    }

  refresh ();
}

returnType
ApvlvDoc::subprocess (int ct, guint key)
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

returnType
ApvlvDoc::process (int has, int ct, guint key)
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
    case 'F':
      mView->promptcommand (char (key));
      return NEED_MORE;
    case 'H':
      if (mDisplayType == DISPLAY_TYPE_IMAGE)
        {
          scrollto (0.0);
          updateCurPoint (0, 0, TRUE);
          blank (mCurrentImage);
        }
      else
        scrollwebto (0.0, 0.0);
      break;
    case 'M':
      if (mDisplayType == DISPLAY_TYPE_IMAGE)
        {
          scrollto (0.5);
          auto x = 0;
          auto y
              = double (mCurrentCache[mCurrentImage->mId]->getheight ()) / 2;
          updateCurPoint (x, y, TRUE);
          blank (mCurrentImage);
        }
      else
        scrollwebto (0.0, 0.5);
      break;
    case 'L':
      if (mDisplayType == DISPLAY_TYPE_IMAGE)
        {
          scrollto (1.0);
          auto x = 0;
          auto y = double (mCurrentCache[mCurrentImage->mId]->getheight ());
          updateCurPoint (x, y, TRUE);
          blank (mCurrentImage);
        }
      else
        scrollwebto (0.0, 1.0);
      break;
    case '0':
      if (mDisplayType == DISPLAY_TYPE_IMAGE)
        scrollleft (INT_MAX);
      else
        scrollwebto (0.0, 0.0);
      break;
    case '$':
      if (mDisplayType == DISPLAY_TYPE_IMAGE)
        scrollright (INT_MAX);
      else
        scrollwebto (1.0, 0.0);
      break;
    case CTRL ('p'):
    case GDK_KEY_Up:
    case 'k':
      if (isControledContent ())
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
      if (isControledContent ())
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
      if (isControledContent ())
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
      if (isControledContent ())
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
      yank (mCurrentImage, ct);
      mInVisual = VISUAL_NONE;
      blank (mCurrentImage);
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

ApvlvDoc *
ApvlvDoc::copy ()
{
  char rate[16];
  g_snprintf (rate, sizeof rate, "%f", mZoomrate);
  auto *ndoc = new ApvlvDoc (mView, rate, usecache ());
  ndoc->loadfile (mFilestr.c_str (), false, false);
  ndoc->showpage (mPagenum, scrollrate ());
  return ndoc;
}

void
ApvlvDoc::setzoom (const char *z)
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
          auto x_root = gtk_widget_get_allocated_width (mMainWidget);
          mZoomrate = x_root / mPagex;
        }
      else if (mZoommode == FITHEIGHT)
        {
          auto y_root = gtk_widget_get_allocated_height (mMainWidget);
          mZoomrate = y_root / mPagey;
        }

      refresh ();
    }
}

bool
ApvlvDoc::savelastposition (const char *filename)
{
  if (filename == nullptr || helppdf == filename || gParams->valueb ("noinfo"))
    {
      return false;
    }

  bool ret = gInfo->file (mPagenum, scrollrate (), filename, mSkip);

  return ret;
}

bool
ApvlvDoc::loadlastposition (const char *filename)
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

bool
ApvlvDoc::reload ()
{
  savelastposition (filename ());
  return loadfile (mFilestr.c_str (), false, isShowContent ());
}

gint
ApvlvDoc::pagenumber ()
{
  if (mContinuous && mCurrentCache[1] != nullptr)
    {
      if (scrollrate () > 0.5)
        {
          return gint (mCurrentCache[1]->getpagenum () + 1);
        }
      else
        {
          return gint (mCurrentCache[1]->getpagenum () + 1);
        }
    }
  else
    {
      return mPagenum + 1;
    }
}

bool
ApvlvDoc::usecache ()
{
  return false;
}

void
ApvlvDoc::usecache (bool use)
{
  if (use)
    {
      mView->errormessage ("No pthread, can't support cache!!!");
      mView->infomessage ("If you really have pthread, please recomplie the "
                          "apvlv and try again.");
    }
}

bool
ApvlvDoc::loadfile (const char *filename, bool check, bool show_content)
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

  // debug ("mFile = %p", mFile);
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

      // debug ("pagesum () = %d", mFile->pagesum ());

      if (mCurrentCache[0] != nullptr)
        {
          delete mCurrentCache[0];
          mCurrentCache[0] = nullptr;
        }
      mCurrentCache[0] = new ApvlvDocCache (mFile);

      if (mCurrentCache[1] != nullptr)
        {
          delete mCurrentCache[1];
          mCurrentCache[1] = nullptr;
        }
      if (mCurrentCache[2] != nullptr)
        {
          delete mCurrentCache[2];
          mCurrentCache[2] = nullptr;
        }

      if (mContinuous)
        {
          mCurrentCache[1] = new ApvlvDocCache (mFile);
          mCurrentCache[2] = new ApvlvDocCache (mFile);
        }

      loadlastposition (filename);

      show ();

      setactive (true);

      setDisplayType (get_display_type_by_filename (filename));

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

          if (mGMonitor)
            {
              g_file_monitor_cancel (mGMonitor);
              mGMonitor = nullptr;
            }

          if (mGFile)
            {
              GError *error = nullptr;
              mGMonitor = g_file_monitor_file (mGFile, G_FILE_MONITOR_NONE,
                                               nullptr, &error);
              if (error != nullptr)
                {
                  debug ("Create file monitor failed: %s\n", error->message);
                  g_error_free (error);
                }
            }

          if (mGMonitor)
            {
              g_file_monitor_set_rate_limit (
                  mGMonitor, gParams->valuei ("autoreload") * 1000);
              g_signal_connect (G_OBJECT (mGMonitor), "changed",
                                G_CALLBACK (apvlv_doc_monitor_callback), this);
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

int
ApvlvDoc::convertindex (int p)
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

void
ApvlvDoc::markposition (const char s)
{
  ApvlvDocPosition adp = { mPagenum, scrollrate () };
  mPositions[s] = adp;
}

void
ApvlvDoc::jump (const char s)
{
  auto it = mPositions.find (s);
  if (it != mPositions.end ())
    {
      ApvlvDocPosition adp = it->second;
      markposition ('\'');
      showpage (adp.pagenum, adp.scrollrate);
    }
}

void
ApvlvDoc::showpage (int p, double s)
{
  int rp = convertindex (p);
  if (rp < 0)
    return;

  // debug ("show page: %d | %lf", rp,s);
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

void
ApvlvDoc::nextpage (int times)
{
  showpage (mPagenum + times, 0.0);
}

void
ApvlvDoc::prepage (int times)
{
  showpage (mPagenum - times, 0.0);
}

void
ApvlvDoc::refresh ()
{
  if (mFile == nullptr)
    return;

  if (mDisplayType == DISPLAY_TYPE_IMAGE)
    {
      mCurrentCache[0]->set (mPagenum, mZoomrate, mRotatevalue, false);
      GdkPixbuf *buf = mCurrentCache[0]->getbuf (true);
      mImg[0]->setFromPixbuf (buf);

      if (mAutoScrollPage && mContinuous)
        {
          mCurrentCache[1]->set (convertindex (mPagenum + 1), mZoomrate,
                                 mRotatevalue, false);
          buf = mCurrentCache[1]->getbuf (true);
          mImg[1]->setFromPixbuf (buf);

          mCurrentCache[2]->set (convertindex (mPagenum + 2), mZoomrate,
                                 mRotatevalue, false);
          buf = mCurrentCache[2]->getbuf (true);
          mImg[2]->setFromPixbuf (buf);
        }
    }
  else if (mDisplayType == DISPLAY_TYPE_HTML)
    {
      mFile->renderweb (mPagenum, 0, 0, mZoomrate, mRotatevalue, mWeb[0]);
    }
  show ();
}

void
ApvlvDoc::srtranslate (int &rtimes, double &sr, bool single2continuous)
{
  gdouble winv = gtk_adjustment_get_upper (mMainVaj)
                 - gtk_adjustment_get_lower (mMainVaj),
          pagewidth = gtk_adjustment_get_page_size (mMainVaj),
          maxv = winv - pagewidth, maxv2 = (winv - 2 * pagewidth) / 2, value;

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

      if (value > 0.5 * (winv - pagewidth))
        sr = 1;
      else if (value > 0.5 * pagewidth)
        sr = (value - 0.5 * pagewidth) / maxv2;
      else
        sr = 0;
    }
}

void
ApvlvDoc::halfnextpage (int times)
{
  double sr = scrollrate ();
  int rtimes = times / 2;

  if (mAutoScrollPage && mContinuous)
    srtranslate (rtimes, sr, false);

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

  if (mAutoScrollPage && mContinuous)
    srtranslate (rtimes, sr, true);

  showpage (mPagenum + rtimes, sr);
}

void
ApvlvDoc::halfprepage (int times)
{
  double sr = scrollrate ();
  int rtimes = times / 2;

  if (mAutoScrollPage && mContinuous)
    srtranslate (rtimes, sr, false);

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

  if (mAutoScrollPage && mContinuous)
    srtranslate (rtimes, sr, true);

  showpage (mPagenum - rtimes, sr);
}

void
ApvlvDoc::markselection ()
{
  debug ("mSelect: %d.", mSearchSelect);
  debug ("zoomrate: %f", mZoomrate);

  ApvlvPos rect = (*mSearchResults)[mSearchSelect];

  // Caculate the correct position
  // debug ("pagex: %f, pagey: %f, x1: %f, y1: %f, x2: %f, y2: %f", mPagex,
  // mPagey, rect->x1, rect->y1, rect->x2, rect->y2);
  gint x1 = (gint)((rect.x1) * mZoomrate);
  gint x2 = (gint)((rect.x2) * mZoomrate);
  gint y1 = (gint)((mPagey - rect.y2) * mZoomrate);
  gint y2 = (gint)((mPagey - rect.y1) * mZoomrate);
  debug ("x1: %d, y1: %d, x2: %d, y2: %d", x1, y1, x2, y2);

  // make the selection at the page center
  gdouble val = ((y1 + y2) - gtk_adjustment_get_page_size (mMainVaj)) / 2;
  debug ("upper: %f, lower: %f, page_size: %f, val: %f",
         gtk_adjustment_get_upper (mMainVaj),
         gtk_adjustment_get_lower (mMainVaj),
         gtk_adjustment_get_page_size (mMainVaj), val);
  if (val + gtk_adjustment_get_page_size (mMainVaj)
      > gtk_adjustment_get_upper (mMainVaj)
            - gtk_adjustment_get_lower (mMainVaj) - 5)
    {
      debug ("set value: %f", gtk_adjustment_get_upper (mMainVaj)
                                  - gtk_adjustment_get_lower (mMainVaj)
                                  - gtk_adjustment_get_page_size (mMainVaj)
                                  - 5);
      gtk_adjustment_set_value (
          mMainVaj, gtk_adjustment_get_upper (mMainVaj)
                        - gtk_adjustment_get_lower (mMainVaj)
                        - gtk_adjustment_get_page_size (mMainVaj)
                        - 5); /* just for avoid the auto scroll page */
    }
  else if (val > 5)
    {
      debug ("set value: %f", val);
      gtk_adjustment_set_value (mMainVaj, val);
    }
  else
    {
      debug ("set value: %f", gtk_adjustment_get_lower (mMainVaj) + 5);
      gtk_adjustment_set_value (mMainVaj,
                                gtk_adjustment_get_lower (mMainVaj)
                                    + 5); /* avoid auto scroll page */
    }

  val = ((x1 + x2) - gtk_adjustment_get_page_size (mMainHaj)) / 2;
  if (val + gtk_adjustment_get_page_size (mMainHaj)
      > gtk_adjustment_get_upper (mMainHaj))
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

  mCurrentCache[0]->set (mPagenum, mZoomrate, mRotatevalue);
  guchar *pagedata = mCurrentCache[0]->getdata (true);
  GdkPixbuf *pixbuf = mCurrentCache[0]->getbuf (true);

  mFile->pageselectsearch (mPagenum, mCurrentCache[0]->getwidth (),
                           mCurrentCache[0]->getheight (), mZoomrate,
                           mRotatevalue, pixbuf, (char *)pagedata,
                           gint (mSearchSelect), mSearchResults);
  mImg[0]->setFromPixbuf (pixbuf);
  debug ("helight num: %d", mPagenum);
}

void
ApvlvDoc::scrollup (int times)
{
  if (!mReady)
    {
      return;
    }

  if (!gParams->valueb ("visualmode"))
    {
      ApvlvCore::scrollup (times);
      return;
    }

  auto cache = mCurrentCache[mCurrentImage->mId];
  auto rate = cache->getHeightOfLine (mCurPoint.y);

  gint ny1 = gint (mCurPoint.y - rate * times);
  if (ny1 < gtk_adjustment_get_value (mMainVaj))
    {
      ApvlvCore::scrollup (times);
    }

  if (ny1 <= 0)
    ny1 = cache->getheight ();

  updateCurPoint (mCurPoint.x, ny1, mInVisual == VISUAL_NONE);
  blank (mCurrentImage);
}

void
ApvlvDoc::scrolldown (int times)
{
  if (!mReady)
    {
      return;
    }

  if (!gParams->valueb ("visualmode"))
    {
      ApvlvCore::scrolldown (times);
      return;
    }

  auto cache = mCurrentCache[mCurrentImage->mId];
  auto rate = cache->getHeightOfLine (mCurPoint.y);

  gint ny1 = gint (mCurPoint.y + rate * times);
  gint height = ny1;
  if (mCurrentImage->mId > 1)
    {
      height += mCurrentCache[0]->getheight ();
    }
  if (mCurrentImage->mId > 1)
    {
      height += mCurrentCache[1]->getheight ();
    }

  if (height - gtk_adjustment_get_value (mMainVaj)
      >= gtk_adjustment_get_page_size (mMainVaj))
    {
      ApvlvCore::scrolldown (times);
    }

  if (ny1 >= cache->getheight ())
    {
      ny1 = 0;
      if (mCurrentImage->mId == 0)
        {
          mCurrentImage = mImg[1];
        }
      else if (mCurrentImage->mId == 1)
        {
          mCurrentImage = mImg[2];
        }
    }

  updateCurPoint (mCurPoint.x, ny1, mInVisual == VISUAL_NONE);
  blank (mCurrentImage);
}

void
ApvlvDoc::scrollleft (int times)
{
  if (!mReady)
    return;

  if (!gParams->valueb ("visualmode"))
    {
      ApvlvCore::scrollleft (times);
      return;
    }

  auto cache = mCurrentCache[mCurrentImage->mId];
  auto rate = cache->getWidthOfWord (mCurPoint.x, mCurPoint.y);

  gint nx1 = gint (mCurPoint.x - rate * times);
  if (nx1 < 0)
    nx1 = 0;

  if (nx1 < gtk_adjustment_get_upper (mMainHaj)
                - gtk_adjustment_get_page_size (mMainHaj))
    {
      ApvlvCore::scrollleft (times);
    }

  updateCurPoint (nx1, mCurPoint.y, mInVisual == VISUAL_NONE);
  blank (mCurrentImage);
}

void
ApvlvDoc::scrollright (int times)
{
  if (!mReady)
    return;

  if (!gParams->valueb ("visualmode"))
    {
      ApvlvCore::scrollright (times);
      return;
    }

  auto cache = mCurrentCache[mCurrentImage->mId];
  auto rate = cache->getWidthOfWord (mCurPoint.x, mCurPoint.y);

  gint nx1 = gint (mCurPoint.x + rate * times);
  if (nx1 > mCurrentCache[0]->getwidth ())
    nx1 = mCurrentCache[0]->getwidth () - 1;

  if (nx1 > gtk_adjustment_get_page_size (mMainHaj))
    {
      ApvlvCore::scrollright (times);
    }

  updateCurPoint (nx1, mCurPoint.y, mInVisual == VISUAL_NONE);
  blank (mCurrentImage);
}

void
ApvlvDoc::scrollweb (int times, int h, int v)
{
  if (!mReady)
    return;

  gchar *javasrc
      = g_strdup_printf ("window.scrollBy(%d, %d);", times * h, times * v);
  webkit_web_view_run_javascript (WEBKIT_WEB_VIEW (mWeb[0]), javasrc, nullptr,
                                  nullptr, this);
  g_free (javasrc);
}

void
ApvlvDoc::scrollwebto (double xrate, double yrate)
{
  if (!mReady)
    return;

  gchar *javasrc = g_strdup_printf (
      "window.scroll(window.screenX * %f, window.screenY * %f);", xrate,
      yrate);
  webkit_web_view_run_javascript (WEBKIT_WEB_VIEW (mWeb[0]), javasrc, nullptr,
                                  nullptr, this);
  g_free (javasrc);
}

void
ApvlvDoc::scrollupweb (int times)
{
  scrollweb (times, 0, -50);
}

void
ApvlvDoc::scrolldownweb (int times)
{
  scrollweb (times, 0, 50);
}

void
ApvlvDoc::scrollleftweb (int times)
{
  scrollweb (times, -50, 0);
}

void
ApvlvDoc::scrollrightweb (int times)
{
  scrollweb (times, 50, 0);
}

bool
ApvlvDoc::needsearch (const char *str, bool reverse)
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
      debug ("same, but need next string: S: %d, s: %d, sel: %d, max: %d.",
             mSearchReverse, reverse, mSearchSelect, mSearchResults->size ());
      mSearchSelect = 0;
      return true;
    }

  // same string, not need search, but has zoomed
  else
    {
      debug ("same, not need next string. sel: %d, max: %u", mSearchSelect,
             mSearchResults->size ());
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

bool
ApvlvDoc::search (const char *str, bool reverse)
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
          mSearchResults = mFile->pagesearch ((i + sum) % sum,
                                              mSearchStr.c_str (), reverse);
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
          mView->errormessage ("can't find word: '%s'", mSearchStr.c_str ());
          return false;
        }
    }
}

bool
ApvlvDoc::totext (const char *file)
{
  if (mFile == nullptr)
    return false;

  char *txt;
  bool ret = mFile->pagetext (mPagenum, 0, 0, mCurrentCache[0]->getwidth (),
                              mCurrentCache[0]->getheight (), &txt);
  if (ret)
    {
      g_file_set_contents (file, txt, -1, nullptr);
      return true;
    }
  return false;
}

void
ApvlvDoc::setactive (bool act)
{
  mStatus->active (act);
  mActive = act;
}

bool
ApvlvDoc::rotate (int ct)
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

void
ApvlvDoc::gotolink (int ct)
{
  ApvlvLinks *links1 = mCurrentCache[0]->getlinks ();
  ApvlvLinks *links2
      = mCurrentCache[1] ? mCurrentCache[1]->getlinks () : nullptr;

  int siz = links1 ? int (links1->size ()) : 0;
  siz += links2 ? int (links2->size ()) : 0;

  ct--;

  if (ct >= 0 && ct < siz)
    {
      markposition ('\'');

      ApvlvDocPosition p = { mPagenum, scrollrate () };
      mLinkPositions.push_back (p);

      if (links1 == nullptr)
        {
          showpage ((*links2)[ct].mPage, 0.0);
        }
      else
        {
          if (ct < (int)links1->size ())
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

void
ApvlvDoc::returnlink (int ct)
{
  debug ("Ctrl-t %d", ct);
  if (ct <= (int)mLinkPositions.size () && ct > 0)
    {
      markposition ('\'');
      ApvlvDocPosition p = { 0, 0 };
      while (ct-- > 0)
        {
          p = mLinkPositions[mLinkPositions.size () - 1];
          mLinkPositions.pop_back ();
        }
      showpage (p.pagenum, p.scrollrate);
    }
}

bool
ApvlvDoc::print (int ct)
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
  if ((int)data->endpn >= mFile->pagesum ())
    {
      data->endpn = mFile->pagesum () - 1;
    }
  // If nothing is specified, print all pages
  if (ct == -1)
    {
      data->frmpn = 0;
      data->endpn = mFile->pagesum () - 1;
      // revert to +ve value, since I don't know if ct is assumed to be +ve
      // anywhere
      ct = mFile->pagesum ();
    }

  g_signal_connect (G_OBJECT (print), "begin-print", G_CALLBACK (begin_print),
                    data);
  g_signal_connect (G_OBJECT (print), "draw-page", G_CALLBACK (draw_page),
                    data);
  g_signal_connect (G_OBJECT (print), "end-print", G_CALLBACK (end_print),
                    data);
  if (settings != nullptr)
    {
      gtk_print_operation_set_print_settings (print, settings);
    }
  int r = gtk_print_operation_run (print,
                                   GTK_PRINT_OPERATION_ACTION_PRINT_DIALOG,
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

void
ApvlvDoc::apvlv_doc_on_mouse (GtkAdjustment *adj, ApvlvDoc *doc)
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
ApvlvDoc::begin_print (GtkPrintOperation *operation, GtkPrintContext *context,
                       PrintData *data)
{
  gtk_print_operation_set_n_pages (operation,
                                   gint (data->endpn - data->frmpn + 1));
}

void
ApvlvDoc::draw_page (GtkPrintOperation *operation, GtkPrintContext *context,
                     gint page_nr, PrintData *data)
{
  cairo_t *cr = gtk_print_context_get_cairo_context (context);
  data->file->pageprint (gint (data->frmpn + page_nr), cr);
  PangoLayout *layout = gtk_print_context_create_pango_layout (context);
  pango_cairo_show_layout (cr, layout);
  g_object_unref (layout);
}

void
ApvlvDoc::end_print (GtkPrintOperation *operation, GtkPrintContext *context,
                     PrintData *data)
{
  delete data;
}
#endif

void
ApvlvDoc::apvlv_doc_monitor_callback (GFileMonitor *fm, GFile *gf1, GFile *gf2,
                                      GFileMonitorEvent ev, ApvlvDoc *doc)
{
  if (!doc->mInuse)
    {
      return;
    }

  if (ev == G_FILE_MONITOR_EVENT_CHANGED)
    {
      debug ("Contents is modified, apvlv reload it automatically");
      doc->reload ();
    }
}

void
ApvlvDoc::apvlv_doc_button_press_cb (GtkEventBox *box, GdkEventButton *button,
                                     ApvlvDoc *doc)
{
  auto image = doc->getApvlvImageByEventBox (box);
  auto cache = doc->mCurrentCache[image->mId];
  gdouble x, y;
  image->toCacheSize (button->x, button->y, cache, &x, &y);
  if (button->button == 1)
    {
      if (button->type == GDK_BUTTON_PRESS)
        {
          // this is a manual method to test double click
          // I think, a normal user double click will in 500 millseconds
          if (button->time - doc->mLastpress < 500)
            {
              doc->mInVisual = ApvlvDoc::VISUAL_NONE;
              doc->doubleClickBlank (image, x, y);
            }
          else
            {
              doc->mInVisual = ApvlvDoc::VISUAL_NONE;
              doc->updateLastPoint (x, y);
              doc->updateCurPoint (x, y, FALSE);
              doc->blank (image);
            }

          doc->mLastpress = button->time;
        }
    }
  else if (button->button == 3)
    {
      if (doc->mInVisual == ApvlvDoc::VISUAL_NONE)
        {
          return;
        }

      GtkWidget *menu, *item;

      menu = gtk_menu_new ();
      gtk_menu_attach_to_widget (GTK_MENU (menu), GTK_WIDGET (box), nullptr);

      item = gtk_menu_item_new_with_label ("Copy to Clipboard");
      gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
      gtk_widget_show (item);
      g_signal_connect (
          item, "activate",
          G_CALLBACK (ApvlvImage::apvlv_image_copytoclipboard_cb), image);

      item = gtk_menu_item_new_with_label ("Under line");
      gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
      gtk_widget_show (item);
      g_signal_connect (item, "activate",
                        G_CALLBACK (ApvlvImage::apvlv_image_underline_cb),
                        image);

      item = gtk_menu_item_new_with_label ("Annotate");
      gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
      gtk_widget_show (item);
      g_signal_connect (item, "activate",
                        G_CALLBACK (ApvlvImage::apvlv_image_annotate_cb),
                        image);

#if GTK_CHECK_VERSION(3, 22, 0)
      gtk_menu_popup_at_pointer (GTK_MENU (menu), nullptr);
#else
      gtk_menu_popup (GTK_MENU (menu), nullptr, nullptr, nullptr, nullptr, 0,
                      0);
#endif
    }
}

gboolean
ApvlvDoc::apvlv_doc_motion_notify_cb (GtkEventBox *box, GdkEventMotion *motion,
                                      ApvlvDoc *doc)
{
  auto image = doc->getApvlvImageByEventBox (box);
  gdouble x, y;
  image->toCacheSize (motion->x, motion->y, doc->mCurrentCache[image->mId], &x,
                      &y);
  if (!(motion->state & GDK_BUTTON1_MASK))
    {
      doc->annotShow (image, x, y);
      return FALSE;
    }

  doc->mInVisual = ApvlvDoc::VISUAL_V;
  if (motion->state & GDK_CONTROL_MASK)
    doc->mInVisual = ApvlvDoc::VISUAL_CTRL_V;
  doc->updateCurPoint (x, y, FALSE);
  doc->blank (image);
  return TRUE;
}

gboolean
ApvlvDoc::apvlv_doc_tooltip_cb (GtkEventBox *box, int x, int y,
                                gboolean keyboard_mode, GtkTooltip *tooltip,
                                ApvlvDoc *doc)
{
  auto image = doc->getApvlvImageByEventBox (box);
  auto cache = doc->mCurrentCache[image->mId];
  gdouble rx, ry;
  image->toCacheSize (double (x), double (y), cache, &rx, &ry);
  auto annot = cache->getAnnotText (rx, ry);
  if (annot == nullptr)
    {
      return FALSE;
    }

  gtk_widget_set_tooltip_text (GTK_WIDGET (box), annot->text.c_str ());
  return TRUE;
}

void
ApvlvDoc::webview_resource_load_started_cb (WebKitWebView *web_view,
                                            WebKitWebResource *resource,
                                            WebKitURIRequest *request,
                                            ApvlvDoc *doc)
{
  debug ("resource: %s, request: %s", webkit_web_resource_get_uri (resource),
         webkit_uri_request_get_uri (request));
}

void
ApvlvDoc::webview_load_changed_cb (WebKitWebView *web_view,
                                   WebKitLoadEvent event, ApvlvDoc *doc)
{
  if (event == WEBKIT_LOAD_FINISHED)
    {
      string anchor = doc->mFile->get_anchor ();
      if (!anchor.empty ())
        {
          gchar *javasrc = g_strdup_printf (
              "document.getElementById('%s').scrollIntoView();",
              anchor.c_str ());
          webkit_web_view_run_javascript (web_view, javasrc, nullptr, nullptr,
                                          doc);
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
ApvlvDoc::contentShowPage (ApvlvFileIndex *index, bool force)
{
  if (index == nullptr)
    return;

  if (index->type == ApvlvFileIndexType::FILE_INDEX_DIR)
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
      if (index->type == ApvlvFileIndexType::FILE_INDEX_PAGE)
        {
          if (index->page != mPagenum)
            showpage (index->page, 0.0);
        }
      return;
    }

  else if (g_ascii_strcasecmp (follow_mode, "always") == 0)
    {
      if (index->type == ApvlvFileIndexType::FILE_INDEX_PAGE)
        {
          if (index->page != mPagenum)
            showpage (index->page, 0.0);
        }
      else
        {
          if (index->path != filename ())
            {
              loadfile (index->path.c_str (), true, false);
            }
        }
    }
}

void
ApvlvDoc::setDisplayType (DISPLAY_TYPE type)
{
  if (type == DISPLAY_TYPE_IMAGE)
    {
      if (gtk_widget_get_parent (mWeb[0]) != nullptr)
        {
          gtk_container_remove (GTK_CONTAINER (mVbox), mWeb[0]);
        }

      if (gtk_widget_get_parent (mImg[0]->widget ()) == nullptr)
        {
          gtk_box_pack_start (GTK_BOX (mVbox), mImg[0]->widget (), TRUE, TRUE,
                              0);
        }
      if (mImg[1])
        {
          if (gtk_widget_get_parent (mImg[1]->widget ()) == nullptr)
            {
              gtk_box_pack_start (GTK_BOX (mVbox), mImg[1]->widget (), TRUE,
                                  TRUE, 0);
            }
        }
      if (mImg[2])
        {
          if (gtk_widget_get_parent (mImg[2]->widget ()) == nullptr)
            {
              gtk_box_pack_start (GTK_BOX (mVbox), mImg[2]->widget (), TRUE,
                                  TRUE, 0);
            }
        }
    }
  else
    {
      if (gtk_widget_get_parent (mImg[0]->widget ()) != nullptr)
        {
          gtk_container_remove (GTK_CONTAINER (mVbox), mImg[0]->widget ());
        }
      if (mImg[1])
        {
          if (gtk_widget_get_parent (mImg[1]->widget ()) != nullptr)
            {
              gtk_container_remove (GTK_CONTAINER (mVbox), mImg[1]->widget ());
            }
        }
      if (mImg[2])
        {
          if (gtk_widget_get_parent (mImg[2]->widget ()) != nullptr)
            {
              gtk_container_remove (GTK_CONTAINER (mVbox), mImg[2]->widget ());
            }
        }

      if (gtk_widget_get_parent (mWeb[0]) == nullptr)
        {
          gtk_box_pack_start (GTK_BOX (mVbox), mWeb[0], TRUE, TRUE, 0);
        }
    }

  gtk_widget_show_all (mVbox);

  mDisplayType = type;
}

void
ApvlvDoc::updateLastPoint (gdouble x, gdouble y)
{
  mLastPoint = { x, y };
}

void
ApvlvDoc::updateCurPoint (gdouble x, gdouble y, gboolean updateLast)
{
  if (updateLast)
    {
      updateLastPoint (mCurPoint.x, mCurPoint.y);
    }
  mCurPoint = { x, y };
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
  mAnnotTexts = nullptr;
}

void
ApvlvDocCache::set (guint p, double zm, guint rot, bool delay)
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
      delete[] mData;
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

void
ApvlvDocCache::load (ApvlvDocCache *ac)
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

  GdkPixbuf *bu = gdk_pixbuf_new_from_data (dat, GDK_COLORSPACE_RGB, FALSE, 8,
                                            ac->mWidth, ac->mHeight,
                                            3 * ac->mWidth, nullptr, nullptr);
  // debug ("ac->mFile: %p", ac->mFile);
  ac->mFile->render (ac->mPagenum, ac->mWidth, ac->mHeight, ac->mZoom,
                     gint (ac->mRotate), bu, (char *)dat);
  if (ac->mInverted)
    {
      invert_pixbuf (bu);
    }
  // backup the pixbuf data
  memcpy (dat + ac->mSize, dat, ac->mSize);

  delete ac->mLinks;

  ac->mLinks = ac->mFile->getlinks (ac->mPagenum);
  // debug ("has mLinkMappings: %p", ac->mLinks);

  ac->mData = dat;
  ac->mBuf = bu;

  ac->preGetLines (0, 0, gint (tpagex), gint (tpagey));
  ac->sortLines ();

  delete ac->mAnnotTexts;
  ac->mAnnotTexts = ac->mFile->getAnnotTexts (ac->mPagenum);
}

ApvlvDocCache::~ApvlvDocCache ()
{
  delete mLinks;

  delete mLines;

  delete[] mData;

  if (mBuf != nullptr)
    {
      g_object_unref (mBuf);
    }

  delete mAnnotTexts;
}

gint
ApvlvDocCache::getpagenum () const
{
  return mPagenum;
}

/*
 * get the cache data
 * @param: wait, if not wait, not wait the buffer be prepared
 * @return: the buffer
 * */
guchar *
ApvlvDocCache::getdata (bool wait)
{
  memcpy (mData, mData + mSize, mSize);
  return mData;
}

/*
 * get the cache GdkPixbuf
 * @param: wait, if not wait, not wait the pixbuf be prepared
 * @return: the buffer
 * */
GdkPixbuf *
ApvlvDocCache::getbuf (bool wait)
{
  return mBuf;
}

gint
ApvlvDocCache::getwidth () const
{
  return mWidth;
}

gint
ApvlvDocCache::getheight () const
{
  return mHeight;
}

ApvlvLinks *
ApvlvDocCache::getlinks ()
{
  return mLinks;
}

ApvlvWord *
ApvlvDocCache::getword (gdouble x, gdouble y)
{
  auto line = getline (y);
  if (line == nullptr)
    return nullptr;

  for (auto &mWord : line->mWords)
    {
      if (x >= mWord.pos.x1 && x <= mWord.pos.x2)
        {
          return &mWord;
        }
    }

  return nullptr;
}

ApvlvLine *
ApvlvDocCache::getline (gdouble y)
{
  g_return_val_if_fail (mLines != nullptr, nullptr);

  for (auto &mLine : *mLines)
    {
      if (y >= mLine.pos.y1 && y <= mLine.pos.y2)
        {
          return &mLine;
        }
    }

  return nullptr;
}

void
ApvlvDocCache::preGetLines (gint x1, gint y1, gint x2, gint y2)
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

      std::set<string> processed;

      if (strcmp (gParams->values ("doubleclick"), "word") == 0)
        {
          stringstream ss (content);
          while (!ss.eof ())
            {
              ss >> word;

              if (processed.count (word) > 0)
                continue;

              processed.insert (word);

              results = mFile->pagesearch (mPagenum, word.c_str (), false);
              if (results != nullptr)
                {
                  prepare_add (word.c_str (), results);
                  delete results;
                }
            }
        }
      else if (strcmp (gParams->values ("doubleclick"), "line") == 0)
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

                  if (processed.count (word) > 0)
                    continue;

                  processed.insert (word);

                  debug ("search [%s]", p);
                  results = mFile->pagesearch (mPagenum, p, false);
                  if (results != nullptr)
                    {
                      prepare_add (p, results);
                      delete results;
                    }
                }
              g_strfreev (v);
            }
        }

      g_free (content);
    }
}

void
ApvlvDocCache::sortLines ()
{
  for (auto line : *mLines)
    {
      sort (line.mWords.begin (), line.mWords.end (),
            [] (const ApvlvWord &w1, const ApvlvWord &w2) {
              return w1.pos.x1 < w2.pos.x1;
            });
    }

  sort (mLines->begin (), mLines->end (),
        [] (const ApvlvLine &line1, const ApvlvLine &line2) {
          return line1.pos.y1 < line2.pos.y1;
        });
}

void
ApvlvDocCache::prepare_add (const char *word, ApvlvPoses *results)
{
  for (auto itr : *results)
    {
      itr.x1 = itr.x1 * mZoom;
      itr.x2 = itr.x2 * mZoom;
      itr.y1 = mHeight - itr.y1 * mZoom;
      itr.y2 = mHeight - itr.y2 * mZoom;

      vector<ApvlvLine>::iterator litr;
      for (litr = mLines->begin (); litr != mLines->end (); ++litr)
        {
          if (fabs (itr.y1 - litr->pos.y1) < 0.0001
              && fabs (itr.y2 - litr->pos.y2) < 0.0001)
            {
              break;
            }
        }

      if (litr != mLines->end ())
        {
          bool need = true;
          for (auto &mWord : litr->mWords)
            {
              auto w = &mWord;
              if (itr.x1 >= w->pos.x1 && itr.x2 <= w->pos.x2)
                {
                  need = false;
                  break;
                }
              else if (itr.x1 <= w->pos.x1 && itr.x2 >= w->pos.x2)
                {
                  w->pos.x1 = itr.x1;
                  w->pos.x2 = itr.x2;
                  w->word = word;
                  need = false;
                  break;
                }
            }

          if (need)
            {
              ApvlvWord w = { itr, word };
              litr->mWords.push_back (w);
              if (itr.x1 < litr->pos.x1)
                {
                  litr->pos.x1 = itr.x1;
                }
              if (itr.x2 > litr->pos.x2)
                {
                  litr->pos.x2 = itr.x2;
                }
            }
        }
      else
        {
          ApvlvLine line;
          line.pos = itr;
          ApvlvWord w = { itr, word };
          line.mWords.push_back (w);
          mLines->push_back (line);
        }
    }
}

gdouble
ApvlvDocCache::getHeightOfLine (gdouble y)
{
  for (const auto &line : *mLines)
    {
      if (y > line.pos.y2 && y < line.pos.y1)
        {
          return line.pos.y1 - line.pos.y2;
        }
    }

  return APVLV_LINE_HEIGHT_DEFAULT;
}

gdouble
ApvlvDocCache::getWidthOfWord (gdouble x, gdouble y)
{
  for (const auto &line : *mLines)
    {
      if (y > line.pos.y2 && y < line.pos.y1)
        {
          for (const auto &word : line.mWords)
            {
              if (x > word.pos.x1 && x < word.pos.x2)
                {
                  return word.pos.x2 - word.pos.x1;
                }
            }
        }
    }

  return APVLV_WORD_WIDTH_DEFAULT;
}

vector<ApvlvPos>
ApvlvDocCache::getSelected (ApvlvPoint last, ApvlvPoint cur, int visual)
{
  vector<ApvlvPos> poses;
  auto y1 = last.y, y2 = cur.y;
  auto x1 = last.x, x2 = cur.x;

  g_return_val_if_fail (y1 <= y2, poses);

  auto lines = getlines (y1, y2);

  if (visual == ApvlvDoc::VISUAL_V)
    {
      if (lines.empty ())
        {
          ApvlvPos pos = { x1, x2, y1, y2 };
          poses.push_back (pos);
        }
      else if (lines.size () == 1)
        {
          ApvlvPos pos = { x1, x2, lines[0]->pos.y1, lines[0]->pos.y2 };
          poses.push_back (pos);
        }
      else if (lines.size () == 2)
        {
          ApvlvPos pos1
              = { x1, lines[0]->pos.x2, lines[0]->pos.y1, lines[0]->pos.y2 };
          poses.push_back (pos1);
          ApvlvPos pos2
              = { lines[1]->pos.x1, x2, lines[1]->pos.y1, lines[1]->pos.y2 };
          poses.push_back (pos2);
        }
      else
        {
          ApvlvPos pos1
              = { x1, lines[0]->pos.x2, lines[0]->pos.y1, lines[0]->pos.y2 };
          poses.push_back (pos1);
          for (size_t lid = 1; lid < lines.size () - 1; ++lid)
            {
              poses.push_back (lines[lid]->pos);
            }
          auto lastid = lines.size () - 1;
          ApvlvPos pos2 = { lines[lastid]->pos.x1, x2, lines[lastid]->pos.y1,
                            lines[lastid]->pos.y2 };
          poses.push_back (pos2);
        }
    }
  else if (visual == ApvlvDoc::VISUAL_CTRL_V)
    {
      ApvlvPos pos = { x1, x2, y1, y2 };
      poses.push_back (pos);
    }
  else
    {
      ApvlvPos pos = { cur.x, cur.x + int (APVLV_CURSOR_WIDTH_DEFAULT), cur.y,
                       cur.y + APVLV_LINE_HEIGHT_DEFAULT };
      if (!lines.empty ())
        {
          pos.y1 = lines[lines.size () - 1]->pos.y1;
          pos.y2 = lines[lines.size () - 1]->pos.y2;
        }
      poses.push_back (pos);
    }

  return poses;
}

vector<ApvlvLine *>
ApvlvDocCache::getlines (gdouble y1, gdouble y2)
{
  vector<ApvlvLine *> lines;

  for (auto &mLine : *mLines)
    {
      auto line = &mLine;
      if (line->pos.y2 >= y1 && line->pos.y1 <= y2)
        lines.push_back (line);
    }

  return lines;
}

ApvlvAnnotText *
ApvlvDocCache::getAnnotText (gdouble x, gdouble y)
{
  for (auto &annottext : *mAnnotTexts)
    {
      if (x >= annottext.pos.x1 && x <= annottext.pos.x2
          && y >= annottext.pos.y1 && y <= annottext.pos.y2)
        {
          debug ("find annotation: %s, %0.f,%0.f,%0.f,%0.f",
                 annottext.text.c_str (), annottext.pos.x1, annottext.pos.y1,
                 annottext.pos.x2, annottext.pos.y2);
          return &annottext;
        }
    }

  return nullptr;
}

void
ApvlvDoc::show ()
{
  if (filename ())
    {
      vector<string> labels;

      gint pn = pagenumber ();
      gint totpn = file ()->pagesum ();
      gdouble sr = scrollrate ();
      int tmprtimes = 0;
      srtranslate (tmprtimes, sr, false);

      char temp[256];
      gchar *bn;
      bn = g_path_get_basename (filename ());
      g_snprintf (temp, sizeof temp, "%s", bn);
      g_free (bn);
      labels.emplace_back (temp);
      g_snprintf (temp, sizeof temp, "%d/%d", pn, totpn);
      labels.emplace_back (temp);
      g_snprintf (temp, sizeof temp, "%d%%", (int)(zoomvalue () * 100));
      labels.emplace_back (temp);
      g_snprintf (temp, sizeof temp, "%d%%",
                  (int)((sr + pn - 1.0) / totpn * 100));
      labels.emplace_back (temp);

      mStatus->show (labels);
    }
}

ApvlvImage *
ApvlvDoc::getApvlvImageByEventBox (GtkEventBox *box)
{
  for (auto img : mImg)
    {
      if (img->widget () == GTK_WIDGET (box))
        return img;
    }

  return nullptr;
}

void
ApvlvDoc::apvlv_doc_enter_notify_cb (GtkEventBox *box, GdkEvent *event,
                                     ApvlvDoc *doc)
{
  if (doc->mCurrentImage == nullptr)
    {
      doc->mCurrentImage = doc->getApvlvImageByEventBox (box);
      return;
    }

  auto lastId = doc->mCurrentImage->mId;
  doc->mCurrentImage = doc->getApvlvImageByEventBox (box);
  if (doc->mCurrentImage->mId > lastId)
    {
      doc->mCurPoint.y = 0;
    }
  if (doc->mCurrentImage->mId < lastId)
    {
      auto cache = doc->mCurrentCache[lastId - 1];
      doc->mCurPoint.y = cache->getheight ();
    }
}

bool
ApvlvDoc::find (const char *str)
{
  g_return_val_if_fail (mReady, false);
  g_return_val_if_fail (gParams->valueb ("visualmode"), false);
  g_return_val_if_fail (*str != '\0', true);

  auto cache = mCurrentCache[mCurrentImage->mId];
  auto results = mFile->pagesearch (cache->getpagenum (), str, false);
  g_return_val_if_fail (results != nullptr, false);

  for (auto pos : *results)
    {
      if (pos.y1 > mCurPoint.y
          || (pos.y1 == mCurPoint.y && pos.x1 > mCurPoint.x))
        {
          auto buffer = cache->getdata (true);
          ApvlvPos pos1 = { pos.x1, pos.x2, cache->getheight () - pos.y1,
                            cache->getheight () - pos.y2 };
          blankarea (mCurrentImage, pos1, buffer, cache->getwidth (),
                     cache->getheight ());
          GdkPixbuf *p = cache->getbuf (true);
          mCurrentImage->setFromPixbuf (p);
          mLastPoint = { pos1.x1, pos1.y1 };
          mCurPoint = { pos1.x2, pos1.y2 };
          break;
        }
    }

  delete results;
  return true;
}

void
ApvlvDoc::annotShow (ApvlvImage *image, gdouble x, gdouble y)
{
  auto cache = mCurrentCache[image->mId];
  auto annot = cache->getAnnotText (x, y);
  if (annot == nullptr)
    {
      return;
    }

  auto win = gtk_window_new (GTK_WINDOW_POPUP);
  auto label = gtk_label_new_with_mnemonic (annot->text.c_str ());
  g_timeout_add (1000, G_SOURCE_FUNC (gtk_widget_destroy), win);
  gtk_container_add (GTK_CONTAINER (win), label);
  gtk_widget_show_all (win);
}

static void
invert_pixbuf (GdkPixbuf *pixbuf)
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

ApvlvImage::ApvlvImage (ApvlvDoc *doc, int id)
{
  mDoc = doc;
  mId = id;
  mEventBox = gtk_event_box_new ();
  g_object_ref (mEventBox);
  mImage = gtk_image_new ();
  gtk_container_add (GTK_CONTAINER (mEventBox), mImage);
  gtk_widget_add_events (mEventBox, GDK_POINTER_MOTION_MASK);
  g_signal_connect (G_OBJECT (mEventBox), "enter-notify-event",
                    G_CALLBACK (ApvlvDoc::apvlv_doc_enter_notify_cb), mDoc);
  g_signal_connect (G_OBJECT (mEventBox), "button-press-event",
                    G_CALLBACK (ApvlvDoc::apvlv_doc_button_press_cb), mDoc);
  g_signal_connect (G_OBJECT (mEventBox), "motion-notify-event",
                    G_CALLBACK (ApvlvDoc::apvlv_doc_motion_notify_cb), mDoc);
  // g_object_set (G_OBJECT (mEventBox), "has-tooltip", TRUE, nullptr);
  // g_signal_connect (G_OBJECT (mEventBox), "query-tooltip",
  //                   G_CALLBACK (ApvlvDoc::apvlv_doc_tooltip_cb), mDoc);
}

ApvlvImage::~ApvlvImage () { g_object_unref (mEventBox); }

GtkWidget *
ApvlvImage::widget ()
{
  return mEventBox;
}

void
ApvlvImage::setFromPixbuf (GdkPixbuf *buf)
{
  gtk_image_set_from_pixbuf (GTK_IMAGE (mImage), buf);
}

void
ApvlvImage::toCacheSize (gdouble x, gdouble y, ApvlvDocCache *cache,
                         gdouble *rx, gdouble *ry)
{
  auto x_root = gtk_widget_get_allocated_width (mEventBox);
  auto y_root = gtk_widget_get_allocated_height (mEventBox);
  if (rx)
    *rx = x - (x_root - gdouble (cache->getwidth ())) / 2;
  if (ry)
    *ry = y - (y_root - gdouble (cache->getheight ())) / 2;
}

void
ApvlvImage::apvlv_image_copytoclipboard_cb (GtkMenuItem *item,
                                            ApvlvImage *image)
{
  image->mDoc->yank (image, 1);
  image->mDoc->mInVisual = ApvlvDoc::VISUAL_NONE;
  image->mDoc->updateCurPoint (image->mDoc->mCurPoint.x,
                               image->mDoc->mCurPoint.y, TRUE);
}

void
ApvlvImage::apvlv_image_underline_cb (GtkMenuItem *item, ApvlvImage *image)
{
  image->mDoc->annotUnderline (image);
  image->mDoc->mInVisual = ApvlvDoc::VISUAL_NONE;
  image->mDoc->updateCurPoint (image->mDoc->mCurPoint.x,
                               image->mDoc->mCurPoint.y, TRUE);
}

void
ApvlvImage::apvlv_image_annotate_cb (GtkMenuItem *item, ApvlvImage *image)
{
  image->mDoc->annotText (image);
  image->mDoc->mInVisual = ApvlvDoc::VISUAL_NONE;
  image->mDoc->updateCurPoint (image->mDoc->mCurPoint.x,
                               image->mDoc->mCurPoint.y, TRUE);
}
}

// Local Variables:
// mode: c++
// End:

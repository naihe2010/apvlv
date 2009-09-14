/*
 * This file is part of the apvlv package
 *
 * Copyright (C) 2008 Alf.
 *
 * Contact: Alf <naihe2010@gmail.com>
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
/* @CFILE ApvlvDir.hpp
 *
 *  Author: Alf <naihe2010@gmail.com>
 */
/* @date Created: 2009/01/03 23:27:52 Alf*/

#ifndef _APVLV_DIR_
#define _APVLV_DIR_

#ifdef HAVE_CONFIG_H
# include "config.hpp"
#endif

#include "ApvlvCore.hpp"
#include "ApvlvWindow.hpp"

namespace apvlv
{
  class ApvlvDir;
  class ApvlvDirNode
  {
  public:
    ApvlvDirNode (gint);
    ApvlvDirNode (bool isdir, const char *, const char *);
    ~ApvlvDirNode ();

    //
    // Get the destination
    bool dest (const char **rpath, int *pagenum);

  private:
      gint mPagenum;		/* -1 means dir, 0 means file, > 0 means page num */
    char filename[0x100];
    char *realname;
  };

  class ApvlvDir;
  class ApvlvDirStatus:public ApvlvCoreStatus
  {
  public:
    ApvlvDirStatus (ApvlvDir *);

    ~ApvlvDirStatus ();

    void active (bool act);

    void setsize (int, int);

    void show ();

  private:
      ApvlvDir * mDoc;
#define AD_STATUS_SIZE   4
    GtkWidget *mStlab[AD_STATUS_SIZE];
  };

  class ApvlvDir:public ApvlvCore
  {
  public:
    ApvlvDir (int w, int h);

     ~ApvlvDir ();

    bool loadfile (const char *file, bool check = true);

    void setactive (bool act);

    returnType process (int times, guint keyval);

  private:

      returnType subprocess (int ct, guint key);

    bool reload ();

    bool enter (guint key);

    void scrollup (int times);

    void scrolldown (int times);

    void scrollleft (int times);

    void scrollright (int times);

    static void apvlv_dir_on_changed (GtkTreeSelection *, ApvlvDir *);

    bool walk_poppler_iter_index (GtkTreeIter * titr,
				  PopplerIndexIter * iter);

    bool walk_dir_path_index (GtkTreeIter * titr, const char *path);

    static gboolean apvlv_dir_first_select_cb (ApvlvDir *);

    gint mFirstSelTimer;

    PopplerDocument *mDoc;
    PopplerIndexIter *mIndex;

    GSList *mDirNodes;

    GtkWidget *mDirView;
    GtkTreeStore *mStore;
    GtkTreeIter mPrevIter, mCurrentIter;
    GtkTreeSelection *mSelection;
  };

}

#endif

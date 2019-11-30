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
/* @CPPFILE ApvlvDir.h
 *
 *  Author: Alf <naihe2010@126.com>
 */
/* @date Created: 2009/01/03 23:27:52 Alf*/

#ifndef _APVLV_DIR_
#define _APVLV_DIR_

#include "ApvlvCore.h"
#include "ApvlvWindow.h"

namespace apvlv
{
  class ApvlvDir;
  class ApvlvDirNode
  {
  public:
    ApvlvDirNode (ApvlvDir *, GtkTreeIter *, gint, const char *);
    ApvlvDirNode (ApvlvDir *, GtkTreeIter *, bool isdir, const char *, const char *);
    ~ApvlvDirNode ();

    //
    // Get the destination
    bool dest (const char **rpath, int *pagenum);

    //
    // Get the string
    const char *phrase ();

    //
    // Get the gtk tree iter
    GtkTreeIter *iter ();

  private:
    static void apvlv_dirnode_monitor_callback (GFileMonitor *, GFile *, GFile *, GFileMonitorEvent, ApvlvDirNode *);

    GtkTreeIter itr[1];

    GFile * mGFile;
    GFileMonitor * mGMonitor;

    gint mPagenum;		/* -1 means dir, 0 means file, > 0 means page num */
    char filename[0x100];
    char *realname;

    ApvlvDir *mDir;
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
    ApvlvDir (ApvlvView *, int w, int h);

    ~ApvlvDir ();

    bool loadfile (const char *file, bool check = true);

    void setactive (bool act);

    returnType process (int hastimes, int times, guint keyval);

  private:

    returnType subprocess (int ct, guint key);

    bool reload ();

    bool enter (guint key);

    void scrollup (int times);

    void scrolldown (int times);

    void scrollleft (int times);

    void scrollright (int times);

    bool search (const char *str, bool reverse = false);

    bool walk_file_index (GtkTreeIter * titr, ApvlvFileIndexIter iter);

    bool walk_dir_path_index (GtkTreeIter * titr, const char *path);

    void apvlv_dir_change_node (ApvlvDirNode *, GFile *, GFileMonitorEvent);

    static void apvlv_dir_on_changed (GtkTreeSelection *, ApvlvDir *);

    static gboolean apvlv_dir_first_select_cb (ApvlvDir *);

    static void apvlv_dir_monitor_callback (GFileMonitor *, GFile *, GFile *, GFileMonitorEvent, ApvlvDir *);

    gint mFirstSelTimer;

    ApvlvFileIndex *mIndex;

    GList *mDirNodes;

    GtkWidget *mDirView;
    GtkTreeStore *mStore;
    GtkTreeIter mPrevIter, mCurrentIter;
    GtkTreeSelection *mSelection;

    friend class ApvlvDirNode;
  };

}

#endif

/* Local Variables: */
/* mode: c++ */
/* End: */

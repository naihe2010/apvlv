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
  class ApvlvDirNode
    {
  public:
    ApvlvDirNode (gint p);
    ApvlvDirNode (const char *s);

    virtual ~ApvlvDirNode ();

    virtual void show (ApvlvWindow *);

  protected:
    gint mPagenum;
    char *mNamed;
    };

  class ApvlvDirNodeDir: public ApvlvDirNode
    {
  public:
    ApvlvDirNodeDir (const char *filename);

    virtual ~ApvlvDirNodeDir ();

    virtual void show (ApvlvWindow *);

  protected:
    string mPath;
    };

  class ApvlvDirNodeFile: public ApvlvDirNodeDir
    {
  public:
    ApvlvDirNodeFile (const char *path, const char *file);

    virtual ~ApvlvDirNodeFile ();

    virtual void show (ApvlvWindow *);

  protected:
    string mFile;
    };

  class ApvlvDir;
  class ApvlvDirStatus: public ApvlvCoreStatus
    {
  public:
    ApvlvDirStatus (ApvlvDir *);

    ~ApvlvDirStatus ();

    void active (bool act);

    void setsize (int, int);

    void show ();

  private:
    ApvlvDir *mDoc;
#define AD_STATUS_SIZE   4
    GtkWidget *mStlab[AD_STATUS_SIZE];
    };

  class ApvlvDir: public ApvlvCore
    {
  public:
    ApvlvDir (const char *zm, const char *path);

    ApvlvDir (const char *zm, ApvlvDoc *doc);

    ~ApvlvDir ();

    void setactive (bool act);

    returnType process (int times, guint keyval);

  private:
    returnType subprocess (int ct, guint key);

    bool reload ();

    void scrollup (int times);

    void scrolldown (int times);

    void scrollleft (int times);

    void scrollright (int times);

    static void apvlv_dir_on_changed (GtkTreeSelection *, ApvlvDir *);

    void walk_poppler_iter_index (GtkTreeIter *titr, PopplerIndexIter *iter);

    void walk_path_file_index (GtkTreeIter *titr, const char *path);

    static gboolean apvlv_dir_first_select_cb (ApvlvDir *);

    gint mFirstSelTimer;

    string mPath;
    ApvlvDoc *mDoc;

    ApvlvWindow *mWindow;

    // directory view
    GtkWidget *mDirView;
    GtkTreeView *mView;
    GtkTreeStore *mStore;
    GtkTreeIter mPrevIter, mCurrentIter;
    GtkTreeSelection *mSelection;
    };

}

#endif

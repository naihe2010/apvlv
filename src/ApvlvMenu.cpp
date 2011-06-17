/*
* This file is part of the apvlv package
* Copyright (C) <2008>  <Alf>
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
/* @CFILE ApvlvMenu.cpp
*
*  Author: Alf <naihe2010@126.com>
*/
/* @date Created: 2010/01/21 15:09:58 Alf*/

#include "ApvlvView.hpp"
#include "ApvlvMenu.hpp"

namespace apvlv
{
static void apvlv_menu_on_file_open ();
static void apvlv_menu_on_file_opentab ();
static void apvlv_menu_on_file_saveas ();
static void apvlv_menu_on_file_print ();
static void apvlv_menu_on_file_quit ();
static void apvlv_menu_on_page_prev ();
static void apvlv_menu_on_page_next ();
static void apvlv_menu_on_page_scrollup ();
static void apvlv_menu_on_page_scrolldown ();
static void apvlv_menu_on_tools_jumpto ();
static void apvlv_menu_on_tools_jumpback ();
static void apvlv_menu_on_help_about ();

ApvlvMenu::ApvlvMenu ()
{
  const GtkActionEntry action_entries[] =
  {
    {"File", NULL, "File", NULL, NULL, NULL},
    {"Open", NULL, "Open", NULL, NULL, apvlv_menu_on_file_open},
    {
      "OpenTab", ":tabnew", "Open Tab...", NULL, NULL,
      apvlv_menu_on_file_opentab
    },
    {
      "SaveAs", ":w", "Save As...", NULL, NULL,
      apvlv_menu_on_file_saveas
    },
    {
      "Print", ":p[rint]", "Print", NULL, NULL,
      apvlv_menu_on_file_print
    },
    {"Quit", ":q[uit]", "Quit", NULL, NULL, apvlv_menu_on_file_quit},
    {"Page", NULL, "Page", NULL, NULL, NULL},
    {
      "Previous", "<PageUp>", "Previous", NULL, NULL,
      apvlv_menu_on_page_prev
    },
    {
      "Next", "<PageDown>", "Next", NULL, NULL,
      apvlv_menu_on_page_next
    },
    {
      "ScrollUp", "<Up>", "ScrollUp", NULL, NULL,
      apvlv_menu_on_page_scrollup
    },
    {
      "ScrollDown", "<Down>", "ScrollDown", NULL, NULL,
      apvlv_menu_on_page_scrolldown
    },
    {"Tools", NULL, "Tools", NULL, NULL, NULL},
    {
      "JumpTo", "<Ctrl>]", "JumpTo", NULL, NULL,
      apvlv_menu_on_tools_jumpto
    },
    {
      "JumpBack", "<Ctrl>t", "JumpBack", NULL, NULL,
      apvlv_menu_on_tools_jumpback
    },
    {"Help", NULL, "Help", NULL, NULL, NULL},
    {
      "About", NULL, "About", NULL, NULL,
      apvlv_menu_on_help_about
    },
  };

  const char *menu_string =
    "<ui>"
    "<menubar>"
    "<menu name=\"File\" action=\"File\">"
    "<menuitem name=\"Open\" action=\"Open\" />"
    "<menuitem name=\"OpenTab\" action=\"OpenTab\" />"
    "<menuitem name=\"SaveAs\" action=\"SaveAs\" />"
    "<menuitem name=\"Print\" action=\"Print\" />"
    "<menuitem name=\"Quit\" action=\"Quit\" />"
    "</menu>"
    "<menu name=\"Page\" action=\"Page\">"
    "<menuitem name=\"Previous\" action=\"Previous\" />"
    "<menuitem name=\"Next\" action=\"Next\" />"
    "<menuitem name=\"ScrollUp\" action=\"ScrollUp\" />"
    "<menuitem name=\"ScrollDown\" action=\"ScrollDown\" />"
    "</menu>"
    "<menu name=\"Tools\" action=\"Tools\">"
    "<menuitem name=\"JumpTo\" action=\"JumpTo\" />"
    "<menuitem name=\"JumpBack\" action=\"JumpBack\" />"
    "</menu>"
    "<menu name=\"Help\" action=\"Help\">"
    "<menuitem name=\"About\" action=\"About\" />"
    "</menu>" "</menubar>" "</ui>";

  GtkUIManager *manager = gtk_ui_manager_new ();

  GtkActionGroup *group = gtk_action_group_new ("action");
  gtk_action_group_add_actions (group, action_entries, sizeof
                                (action_entries) / sizeof
                                (action_entries[0]), NULL);
  gtk_ui_manager_insert_action_group (manager, group, 0);

  gtk_ui_manager_add_ui_from_string (manager, menu_string, -1, NULL);

  mMenu = gtk_ui_manager_get_widget (manager, "/ui/menubar");
}

ApvlvMenu::~ApvlvMenu ()
{
}

GtkWidget *ApvlvMenu::widget ()
{
  return mMenu;
}

void ApvlvMenu::setsize (gint w, gint h)
{
  gtk_widget_set_size_request (mMenu, w, h);
}

static void apvlv_menu_on_file_open ()
{
  gView->open ();
}

static void apvlv_menu_on_file_opentab ()
{
  gView->newtab (helppdf.c_str ());
}

static void apvlv_menu_on_file_saveas ()
{
//        gView->save ();
}

static void apvlv_menu_on_file_print ()
{
  //       gView->print ();
}

static void apvlv_menu_on_file_quit ()
{
  gView->quit ();
}

static void apvlv_menu_on_page_prev ()
{
  gView->crtadoc ()->prepage (1);
}

static void apvlv_menu_on_page_next ()
{
  gView->crtadoc ()->nextpage (1);
}

static void apvlv_menu_on_page_scrollup ()
{
  gView->crtadoc ()->scrollup (1);
}

static void apvlv_menu_on_page_scrolldown ()
{
  gView->crtadoc ()->scrolldown (1);
}

static void apvlv_menu_on_tools_jumpto ()
{
  gView->crtadoc ()->gotolink (1);
}

static void apvlv_menu_on_tools_jumpback ()
{
  gView->crtadoc ()->returnlink (1);
}

static void apvlv_menu_on_help_about ()
{
  GtkWidget *dia = NULL;
  static const char *author[] =
  {
    "Alf",
    NULL
  };

  dia = gtk_about_dialog_new ();
  gtk_about_dialog_set_name (GTK_ABOUT_DIALOG (dia), PACKAGE_NAME);
  gtk_about_dialog_set_version (GTK_ABOUT_DIALOG (dia), PACKAGE_VERSION);
  gtk_about_dialog_set_authors (GTK_ABOUT_DIALOG (dia), author);
  gtk_about_dialog_set_artists (GTK_ABOUT_DIALOG (dia), author);
  gtk_about_dialog_set_website (GTK_ABOUT_DIALOG (dia),
                                "http://naihe2010.cublog.cn");
  gtk_about_dialog_set_copyright (GTK_ABOUT_DIALOG (dia), "GNU");
  gtk_dialog_run (GTK_DIALOG (dia));
  gtk_widget_destroy (dia);
}
};

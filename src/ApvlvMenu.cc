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
/* @CPPFILE ApvlvMenu.cc
 *
 *  Author: Alf <naihe2010@126.com>
 */
/* @date Created: 2010/01/21 15:09:58 Alf*/

#include <gtk/gtk.h>

#include "ApvlvUtil.h"
#include "ApvlvMenu.h"

namespace apvlv
{
    extern "C" void apvlv_menu_on_file_open ();
    extern "C" void apvlv_menu_on_file_opentab ();
    extern "C" void apvlv_menu_on_file_saveas ();
    extern "C" void apvlv_menu_on_file_print ();
    extern "C" void apvlv_menu_on_file_quit ();
    extern "C" void apvlv_menu_on_page_previous ();
    extern "C" void apvlv_menu_on_page_next ();
    extern "C" void apvlv_menu_on_page_scrollup ();
    extern "C" void apvlv_menu_on_page_scrolldown ();
    extern "C" void apvlv_menu_on_tools_jumpto ();
    extern "C" void apvlv_menu_on_tools_jumpback ();
    extern "C" void apvlv_menu_on_help_about ();

    ApvlvMenu::ApvlvMenu ()
    {
      GtkBuilder *builder = gtk_builder_new ();
      gtk_builder_add_from_file (builder, mainmenubar_glade.c_str (), nullptr);
      gtk_builder_connect_signals (builder, nullptr);
      mMenu = GTK_WIDGET (gtk_builder_get_object (builder, "main_menubar"));
    }

    ApvlvMenu::~ApvlvMenu ()
    = default;

    GtkWidget *ApvlvMenu::widget ()
    {
      return mMenu;
    }

    void ApvlvMenu::setsize (gint w, gint h)
    {
      gtk_widget_set_size_request (mMenu, w, h);
    }

    extern "C" void apvlv_menu_on_file_open ()
    {
      //mView->open ();
    }

    extern "C" void apvlv_menu_on_file_opentab ()
    {
      //gView->newtab (helppdf.c_str ());
    }

    extern "C" void apvlv_menu_on_file_saveas ()
    {
      //        gView->save ();
    }

    extern "C" void apvlv_menu_on_file_print ()
    {
      //       gView->print ();
    }

    extern "C" void apvlv_menu_on_file_quit ()
    {
      //gView->quit ();
    }

    extern "C" void apvlv_menu_on_page_previous ()
    {
      //gView->crtadoc ()->prepage (1);
    }

    extern "C" void apvlv_menu_on_page_next ()
    {
      //gView->crtadoc ()->nextpage (1);
    }

    extern "C" void apvlv_menu_on_page_scrollup ()
    {
      //gView->crtadoc ()->scrollup (1);
    }

    extern "C" void apvlv_menu_on_page_scrolldown ()
    {
      //gView->crtadoc ()->scrolldown (1);
    }

    extern "C" void apvlv_menu_on_tools_jumpto ()
    {
      //gView->crtadoc ()->gotolink (1);
    }

    extern "C" void apvlv_menu_on_tools_jumpback ()
    {
      //gView->crtadoc ()->returnlink (1);
    }

    extern "C" void apvlv_menu_on_help_about ()
    {
      static const char *author[] =
          {
              "Alf",
              nullptr
          };

      GtkWidget *dia = gtk_about_dialog_new ();
      gtk_about_dialog_set_program_name (GTK_ABOUT_DIALOG (dia), PACKAGE_NAME);
      gtk_about_dialog_set_version (GTK_ABOUT_DIALOG (dia), PACKAGE_VERSION);
      gtk_about_dialog_set_authors (GTK_ABOUT_DIALOG (dia), author);
      gtk_about_dialog_set_artists (GTK_ABOUT_DIALOG (dia), author);
      gtk_about_dialog_set_website (GTK_ABOUT_DIALOG (dia),
                                    "https://naihe2010.github.io/apvlv");
      gtk_about_dialog_set_copyright (GTK_ABOUT_DIALOG (dia), "GNU");
      gtk_dialog_run (GTK_DIALOG (dia));
      gtk_widget_destroy (dia);
    }
};

// Local Variables:
// mode: c++
// End:

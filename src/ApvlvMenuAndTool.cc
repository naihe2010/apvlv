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
/* @CPPFILE ApvlvMenuAndTool.cc
 *
 *  Author: Alf <naihe2010@126.com>
 */
/* @date Created: 2010/01/21 15:09:58 Alf*/

#include <gtk/gtk.h>

#include "ApvlvMenuAndTool.h"
#include "ApvlvUtil.h"
#include "ApvlvView.h"

extern "C"
{
  void apvlv_on_file_open (GtkWidget *wid, apvlv::ApvlvView *view);
  void apvlv_on_file_opentab (GtkWidget *wid, apvlv::ApvlvView *view);
  void apvlv_on_file_saveas (GtkWidget *wid, apvlv::ApvlvView *view);
  void apvlv_on_file_print (GtkWidget *wid, apvlv::ApvlvView *view);
  void apvlv_on_file_quit (GtkWidget *wid, apvlv::ApvlvView *view);
  void apvlv_on_page_previous (GtkWidget *wid, apvlv::ApvlvView *view);
  void apvlv_on_page_next (GtkWidget *wid, apvlv::ApvlvView *view);
  void apvlv_on_page_scrollup (GtkWidget *wid, apvlv::ApvlvView *view);
  void apvlv_on_page_scrolldown (GtkWidget *wid, apvlv::ApvlvView *view);
  void apvlv_on_navigate_jumpto (GtkWidget *wid, apvlv::ApvlvView *view);
  void apvlv_on_navigate_jumpback (GtkWidget *wid, apvlv::ApvlvView *view);
  void apvlv_on_help_about (GtkWidget *wid, apvlv::ApvlvView *view);
}

namespace apvlv
{
ApvlvMenuAndTool::ApvlvMenuAndTool (ApvlvView *view)
{
  mBuilder = gtk_builder_new ();
  gtk_builder_add_from_file (mBuilder, mainmenubar_glade.c_str (), nullptr);
  gtk_builder_connect_signals (mBuilder, view);
}

ApvlvMenuAndTool::~ApvlvMenuAndTool () { g_object_unref (mBuilder); };

GtkWidget *
ApvlvMenuAndTool::menubar ()
{
  auto wid = GTK_WIDGET (gtk_builder_get_object (mBuilder, "main_menubar"));
  return wid;
}

GtkWidget *
ApvlvMenuAndTool::toolbar ()
{
  auto wid = GTK_WIDGET (gtk_builder_get_object (mBuilder, "main_toolbar"));
  return wid;
}

extern "C"
{
  void
  apvlv_on_file_open (GtkWidget *wid, ApvlvView *view)
  {
    view->open ();
  }

  void
  apvlv_on_file_opentab (GtkWidget *wid, apvlv::ApvlvView *view)
  {
    view->newtab (view->crtadoc ());
  }

  void
  apvlv_on_file_saveas (GtkWidget *wid, apvlv::ApvlvView *view)
  {
    // view->crtadoc()->save ();
  }

  void
  apvlv_on_file_print (GtkWidget *wid, apvlv::ApvlvView *view)
  {
    // view->crtadoc()->print ();
  }

  void
  apvlv_on_file_quit (GtkWidget *wid, apvlv::ApvlvView *view)
  {
    gtk_main_quit ();
  }

  void
  apvlv_on_page_previous (GtkWidget *wid, apvlv::ApvlvView *view)
  {
    view->crtadoc ()->prepage (1);
  }

  void
  apvlv_on_page_next (GtkWidget *wid, apvlv::ApvlvView *view)
  {
    view->crtadoc ()->nextpage (1);
  }

  void
  apvlv_on_page_scrollup (GtkWidget *wid, apvlv::ApvlvView *view)
  {
    view->crtadoc ()->scrollup (1);
  }

  void
  apvlv_on_page_scrolldown (GtkWidget *wid, apvlv::ApvlvView *view)
  {
    view->crtadoc ()->scrolldown (1);
  }

  void
  apvlv_on_navigate_jumpto (GtkWidget *wid, apvlv::ApvlvView *view)
  {
    GtkDialogFlags flags = GTK_DIALOG_DESTROY_WITH_PARENT;
    auto dialog = gtk_message_dialog_new (
        reinterpret_cast<GtkWindow *> (view->widget ()), flags,
        GTK_MESSAGE_QUESTION, GTK_BUTTONS_OK_CANCEL, ("Input page number: "));
    auto entry = gtk_entry_new ();
    gtk_container_add (
        GTK_CONTAINER (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
        entry);
    gtk_widget_show_all (dialog);
    gtk_dialog_run (GTK_DIALOG (dialog));
    auto text = gtk_entry_get_text (GTK_ENTRY (entry));
    if (text != nullptr && isdigit (*text))
      {
        auto page = strtol (text, nullptr, 10);
        view->crtadoc ()->showpage (page, 0.f);
      }
    gtk_widget_destroy (dialog);
  }

  void
  apvlv_on_navigate_jumpback (GtkWidget *wid, apvlv::ApvlvView *view)
  {
    view->crtadoc ()->jump ('\'');
  }

  void
  apvlv_on_help_about (GtkWidget *wid, apvlv::ApvlvView *view)
  {
    static const char *author[] = { "Alf", nullptr };

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
}
};

// Local Variables:
// mode: c++
// End:

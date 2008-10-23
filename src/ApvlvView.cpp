/****************************************************************************
 * Copyright (c) 1998-2005,2006 Free Software Foundation, Inc.              
 *                                                                          
 * Permission is hereby granted, free of charge, to any person obtaining a  
 * copy of this software and associated documentation files (the            
 * "Software"), to deal in the Software without restriction, including      
 * without limitation the rights to use, copy, modify, merge, publish,      
 * distribute, distribute with modifications, sublicense, and/or sell       
 * copies of the Software, and to permit persons to whom the Software is    
 * furnished to do so, subject to the following conditions:                 
 *                                                                          
 * The above copyright notice and this permission notice shall be included  
 * in all copies or substantial portions of the Software.                   
 *                                                                          
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS  
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF               
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.   
 * IN NO EVENT SHALL THE ABOVE COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,   
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR    
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR    
 * THE USE OR OTHER DEALINGS IN THE SOFTWARE.                               
 *                                                                          
 * Except as contained in this notice, the name(s) of the above copyright   
 * holders shall not be used in advertising or otherwise to promote the     
 * sale, use or other dealings in this Software without prior written       
 * authorization.                                                           
 ****************************************************************************/

/****************************************************************************
 *  Author:    YuPengda
 *  AuthorRef: Alf <naihe2010@gmail.com>
 *  Blog:      http://naihe2010.cublog.cn
 ****************************************************************************/
#include "ApvlvDoc.hpp"
#include "ApvlvParams.hpp"
#include "ApvlvCmds.hpp"
#include "ApvlvUtil.hpp"
#include "ApvlvView.hpp"

#include <stdlib.h>
#include <string.h>

#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <gdk/gdkkeysyms.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include <iostream>
#include <sstream>

namespace apvlv
{
  ApvlvView::ApvlvView (ApvlvParams *pa): ApvlvCmds (pa)
    {
      mainwindow = gtk_window_new (GTK_WINDOW_TOPLEVEL);
      g_signal_connect (G_OBJECT (mainwindow), "size-allocate",
                        G_CALLBACK (apvlv_view_resized_cb), this);

      width = atoi (param->settingvalue ("width"));
      height = atoi (param->settingvalue ("height"));

      full_has = FALSE;
      gtk_widget_set_size_request (mainwindow, width, height);

      g_object_set_data (G_OBJECT (mainwindow), "view", this);
      g_signal_connect (G_OBJECT (mainwindow), "event",
                        G_CALLBACK (apvlv_view_keypress_cb), this);

      GtkWidget *vbox = gtk_vbox_new (FALSE, 0);
      gtk_container_add (GTK_CONTAINER (mainwindow), vbox);

      adoc = new ApvlvDoc (param->settingvalue ("zoom"));
      gtk_box_pack_start (GTK_BOX (vbox), adoc->widget (), FALSE, FALSE, 0);
      crtadoc = adoc;

      statusbar = gtk_entry_new ();
      gtk_box_pack_start (GTK_BOX (vbox), statusbar, FALSE, FALSE, 0);
      g_object_set_data (G_OBJECT (statusbar), "view", this);
      g_signal_connect (G_OBJECT (statusbar), "event", G_CALLBACK (apvlv_view_statusbar_cb), this);

      g_signal_connect (G_OBJECT (mainwindow), "delete-event",
                        G_CALLBACK (apvlv_view_delete_cb), this);
      g_signal_connect (G_OBJECT (mainwindow), "destroy-event",
                        G_CALLBACK (apvlv_view_delete_cb), this);

      gtk_widget_show_all (mainwindow);

      cmd_has = FALSE;

      const char *fs = param->settingvalue ("fullscreen");
      if (strcmp (fs, "yes") == 0)
        {
          fullscreen ();
        }
      else
        {
          gtk_widget_set_usize (mainwindow, width, height);
        }
    }

  ApvlvView::~ApvlvView ()
    {
    }

  void 
    ApvlvView::open ()
      {
        GtkWidget *dia = gtk_file_chooser_dialog_new ("",
                                                      GTK_WINDOW (mainwindow),
                                                      GTK_FILE_CHOOSER_ACTION_SAVE,
                                                      GTK_STOCK_CANCEL,
                                                      GTK_RESPONSE_CANCEL,
                                                      GTK_STOCK_OK,
                                                      GTK_RESPONSE_ACCEPT,
                                                      NULL);
        gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (dia), param->settingvalue ("defaultdir"));

        GtkFileFilter *filter = gtk_file_filter_new ();
        gtk_file_filter_add_mime_type (filter, "PDF File");
        gtk_file_filter_add_pattern (filter, "*.pdf");
        gtk_file_filter_add_pattern (filter, "*.PDF");
        gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (dia), filter);

        gint ret = gtk_dialog_run (GTK_DIALOG (dia));
        if (ret == GTK_RESPONSE_ACCEPT)
          {
            gchar *filename =
              gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dia));
            loadfile (filename);
            g_free (filename);
          }
        gtk_widget_destroy (dia);
      }

  void 
    ApvlvView::promptsearch ()
      {
        gtk_entry_set_text (GTK_ENTRY (statusbar), "/");
        cmd_mode = SEARCH;
        cmd_show ();
      }

  void 
    ApvlvView::promptbacksearch ()
      {
        gtk_entry_set_text (GTK_ENTRY (statusbar), "?");
        cmd_mode = BACKSEARCH;
        cmd_show ();
      }

  void 
    ApvlvView::promptcommand ()
      {
        gtk_entry_set_text (GTK_ENTRY (statusbar), ":");
        cmd_mode = COMMANDMODE;
        cmd_show ();
      }

  void 
    ApvlvView::cmd_show ()
      {
        if (mainwindow == NULL)
          return;

        gtk_widget_grab_focus (statusbar);
        gtk_entry_set_position (GTK_ENTRY (statusbar), 1);
        cmd_has = TRUE;
      }

  void 
    ApvlvView::status_show ()
      {
        if (mainwindow == NULL)
          return;

        char temp[256] = { 0 };

        if (crtadoc != NULL && crtadoc->filename ())
          {
            snprintf (temp, sizeof temp, "\"%s\"\t%d/%d\t\t%d\%\t\t\t\t%d\%",
                      crtadoc->filename (),
                      crtadoc->pagenumber (),
                      crtadoc->pagesum (),
                      (int) (crtadoc->zoomvalue () * 100),
                      (int) (crtadoc->scrollrate () * 100)
            );
          }

        gtk_entry_set_text (GTK_ENTRY (statusbar), temp);

        gtk_widget_grab_focus (crtadoc->widget ());
        cmd_has = FALSE;
      }

  void 
    ApvlvView::fullscreen () 
      { 
        if (full_has == false)
          {
            gtk_window_maximize (GTK_WINDOW (mainwindow)); 
            full_has = true; 
          }
        else
          {
            gtk_window_unmaximize (GTK_WINDOW (mainwindow)); 
            full_has = false;
          }
      }

  void
    ApvlvView::dowindow (const char *s)
      {
        ApvlvDoc *ndoc = NULL;
        if (strcmp (s, "C-w") == 0)
          {
            ndoc = crtadoc->getneighbor ("j");
            if (ndoc == NULL)
              {
                ndoc = crtadoc->getneighbor ("l");
              }
          }

        else if (strcmp (s, "-") == 0)
          {
            crtadoc->sizesmaller (1);
          }
        else if (strcmp (s, "+") == 0)
          {
            crtadoc->sizebigger (1);
          }

        else
          {
            ndoc = crtadoc->getneighbor (s);
          }

        if (ndoc != NULL)
          {
            crtadoc = ndoc;
          }

        status_show ();
      }

  void
    ApvlvView::parse_cmd (GdkEventKey * gek)
      {
        if (gek->keyval == GDK_Page_Up)
          {
            prepage ();
          }
        else if (gek->keyval == GDK_Page_Down)
          {
            nextpage ();
          }
        else if (gek->state == GDK_CONTROL_MASK)
          {
            push ("C-");
            push (gek->keyval);
          }
        else
          {
            push (gek->string);
          }
      }

  void
    ApvlvView::run (const char *str)
      {
        switch (cmd_mode)
          {
          case SEARCH:
            crtadoc->search (str);
            break;

          case BACKSEARCH:
            crtadoc->backsearch (str);
            break;

          case COMMANDMODE:
            runcmd (str);
            break;

          default:
            break;
          }

        status_show ();
      }

  void 
    ApvlvView::runcmd (const char *str)
      {
        if (*str == '!')
          {
            system (str + 1);
          }
        else
          {
            stringstream ss (str);
            string cmd, subcmd, argu;
            ss >> cmd >> subcmd >> argu;

            if (cmd == "set")
              {
              }
            else if (cmd == "map")
              {
              }
            else if (cmd == "vsp")
              {
                vseparate ();
              }
            else if (cmd == "hsp")
              {
                hseparate ();
              }
            else if (cmd == "zoom" || cmd == "z")
              {
                setzoom (subcmd.c_str ());
              }
            else if (cmd == "forwardpage" || cmd == "fp")
              {
                nextpage (atoi (subcmd.c_str ()));
              }
            else if (cmd == "prewardpage" || cmd == "pp")
              {
                prepage (atoi (subcmd.c_str ()));
              }
            else if (cmd == "goto" || cmd == "g")
              {
                showpage (atoi (subcmd.c_str ()));
              }
            else if ((cmd == "help" || cmd == "h")
                     && subcmd == "info")
              {
                loadfile (helppdf);
                showpage (2);
              }
            else if ((cmd == "help" || cmd == "h")
                     && subcmd == "command")
              {
                loadfile (helppdf);
                showpage (4);
              }
            else if ((cmd == "help" || cmd == "h")
                     && subcmd == "setting")
              {
                loadfile (helppdf);
                showpage (6);
              }
            else if (cmd == "help" || cmd == "h")
              {
                loadfile (helppdf);
              }
            else if (cmd == "q" || cmd == "quit")
              {
                // return, avoid to return to status mode
                ApvlvDoc *ndoc = crtadoc->getneighbor ("n");
                if (ndoc == crtadoc)
                  {
                    quit ();
                  }
                else
                  {
                    delete crtadoc;
                    crtadoc = ndoc;
                  }
              }

            // After processed the command, return to status mode
            status_show ();
          }
      }

  void
    ApvlvView::apvlv_view_resized_cb (GtkWidget * wid, GtkAllocation * al,
                                      ApvlvView * view)
      {
        gtk_window_get_size (GTK_WINDOW (wid), &view->width, &view->height);
        gtk_widget_set_usize (view->crtadoc->widget (), view->width, view->height - 20);
        gtk_widget_set_usize (view->statusbar, view->width, 20);
        view->crtadoc->setsize (view->width, view->height - 20);
      }

  gint 
    ApvlvView::apvlv_view_keypress_cb (GtkWidget * wid, GdkEvent * ev)
      {
        ApvlvView *view =
          (ApvlvView *) g_object_get_data (G_OBJECT (wid), "view");

        if (view->cmd_has == FALSE && ev->type == GDK_KEY_PRESS)
          {
            view->parse_cmd ((GdkEventKey *) ev);
            if (view->cmd_has == false)
              {
                view->status_show ();
              }
            return TRUE;
          }

        return FALSE;
      }

  gint 
    ApvlvView::apvlv_view_statusbar_cb (GtkWidget * wid, GdkEvent * ev)
      {
        ApvlvView *view = (ApvlvView *) g_object_get_data (G_OBJECT (wid), "view");

        if (view->cmd_has == TRUE && ev->type == GDK_KEY_PRESS)
          {
            GdkEventKey *gek = (GdkEventKey *) ev;
            if (gek->keyval == GDK_Return)
              {
                gchar *str =
                  (gchar *) gtk_entry_get_text (GTK_ENTRY (view->statusbar));
                if (str)
                  {
                    view->run (str + 1);
                  }
                return TRUE;
              }
            else if (gek->keyval == GDK_Escape)
              {
                view->status_show ();
                return TRUE;
              }

            return FALSE;
          }

        return FALSE;
      }

  void
    ApvlvView::apvlv_view_delete_cb (GtkWidget * wid, GtkAllocation * al,
                                     ApvlvView * view)
      {
        view->mainwindow = NULL;
        gtk_main_quit ();
      }
}

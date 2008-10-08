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
#include "ApvlvView.hpp"

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

      width = atoi (param->value ("width"));
      height = atoi (param->value ("height"));

      full_has = FALSE;
      gtk_widget_set_size_request (mainwindow, width, height);

      g_object_set_data (G_OBJECT (mainwindow), "view", this);
      g_signal_connect (G_OBJECT (mainwindow), "event",
                        G_CALLBACK (apvlv_view_keypress_cb), this);

      GtkWidget *vbox = gtk_vbox_new (FALSE, 0);
      gtk_container_add (GTK_CONTAINER (mainwindow), vbox);

      adoc = new ApvlvDoc (param->value ("zoom"));
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

      cmd_hide ();

      const char *fs = param->value ("fullscreen");
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
      delete adoc;
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
        gtk_entry_set_text (GTK_ENTRY (statusbar), param->key ("search"));
        cmd_mode = SEARCH;
        cmd_show ();
      }

  void 
    ApvlvView::promptbacksearch ()
      {
        gtk_entry_set_text (GTK_ENTRY (statusbar), param->key ("backsearch"));
        cmd_mode = BACKSEARCH;
        cmd_show ();
      }

  void 
    ApvlvView::promptcommand ()
      {
        gtk_entry_set_text (GTK_ENTRY (statusbar), param->key ("commandmode"));
        cmd_mode = COMMANDMODE;
        cmd_show ();
      }

  void 
    ApvlvView::cmd_show ()
      {
        gtk_widget_set_usize (crtadoc->widget (), width, height - 20);
        crtadoc->setsize (width, height - 20);
        gtk_widget_set_usize (statusbar, width, 20);
        gtk_widget_grab_focus (statusbar);
        gtk_entry_set_position (GTK_ENTRY (statusbar), 1);
        cmd_has = TRUE;
      }

  void 
    ApvlvView::cmd_hide ()
      {
        gtk_widget_set_usize (crtadoc->widget (), width, height);
        crtadoc->setsize (width, height);
        gtk_widget_set_usize (statusbar, width, 0);
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
              }
            else if (cmd == "hsp")
              {
              }
            else
              {
              }
          }
      }

  void
    ApvlvView::apvlv_view_resized_cb (GtkWidget * wid, GtkAllocation * al,
                                      ApvlvView * view)
      {
        gtk_window_get_size (GTK_WINDOW (wid), &view->width, &view->height);
        view->cmd_has ? view->cmd_show () : view->cmd_hide ();
      }

  gint 
    ApvlvView::apvlv_view_keypress_cb (GtkWidget * wid, GdkEvent * ev)
      {
        ApvlvView *view =
          (ApvlvView *) g_object_get_data (G_OBJECT (wid), "view");

        if (view->cmd_has == FALSE && ev->type == GDK_KEY_PRESS)
          {
            view->parse_cmd ((GdkEventKey *) ev);
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
                    view->cmd_hide ();
                  }
                return TRUE;
              }
            else if (gek->keyval == GDK_Escape)
              {
                view->cmd_hide ();
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
        if (view->adoc)
          {
            delete view->adoc;
            view->adoc = NULL;
          }
        gtk_main_quit ();
      }
}

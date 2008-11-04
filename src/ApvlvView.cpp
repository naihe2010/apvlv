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
#include "ApvlvParams.hpp"
#include "ApvlvCmds.hpp"
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
  ApvlvView *gView = NULL;

  ApvlvView::ApvlvView (int argc, char *argv[])
    {
      gtk_init (&argc, &argv);

      mainwindow = gtk_window_new (GTK_WINDOW_TOPLEVEL);
      g_signal_connect (G_OBJECT (mainwindow), "size-allocate",
                        G_CALLBACK (apvlv_view_resized_cb), this);

      width = atoi (gParams->settingvalue ("width"));
      height = atoi (gParams->settingvalue ("height"));

      full_has = FALSE;
      gtk_widget_set_size_request (mainwindow, width, height);

      g_object_set_data (G_OBJECT (mainwindow), "view", this);
      g_signal_connect (G_OBJECT (mainwindow), "key-press-event",
                        G_CALLBACK (apvlv_view_keypress_cb), this);

      GtkWidget *vbox = gtk_vbox_new (FALSE, 2);
      gtk_container_add (GTK_CONTAINER (mainwindow), vbox);

      m_rootWindow = new ApvlvWindow (NULL);
      m_rootWindow->setcurrentWindow (m_rootWindow);
      gtk_box_pack_start (GTK_BOX (vbox), m_rootWindow->widget (), FALSE, FALSE, 0);

      statusbar = gtk_entry_new ();
      gtk_box_pack_end (GTK_BOX (vbox), statusbar, FALSE, FALSE, 0);
      g_object_set_data (G_OBJECT (statusbar), "view", this);
      g_signal_connect (G_OBJECT (statusbar), "key-press-event", G_CALLBACK (apvlv_view_statusbar_cb), this);

      g_signal_connect (G_OBJECT (mainwindow), "delete-event",
                        G_CALLBACK (apvlv_view_delete_cb), this);
      g_signal_connect (G_OBJECT (mainwindow), "destroy-event",
                        G_CALLBACK (apvlv_view_delete_cb), this);

      gtk_widget_show_all (mainwindow);

      cmd_has = FALSE;

      const char *fs = gParams->settingvalue ("fullscreen");
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
      delete m_rootWindow;

      map <string, ApvlvDoc *>::iterator it;
      for (it = m_Docs.begin (); it != m_Docs.end (); ++ it)
        {
          delete it->second;
        }
      m_Docs.clear ();
    }

  void
    ApvlvView::show ()
      {
        gtk_main ();
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
        gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (dia), gParams->settingvalue ("defaultdir"));

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

  bool
    ApvlvView::loadfile (const char *filename)
      {
        const char *abpath = absolutepath (filename);
        ApvlvDoc *ndoc = hasloaded (abpath);
        if (ndoc != NULL)
          {
            currentWindow ()->setDoc (ndoc);
            return true;
          }
        else
          {
            ndoc = currentWindow ()->loadDoc (filename);
            if (ndoc)
              {
                m_Docs[abpath] = ndoc;
                return true;
              }
            else
              {
                return false;
              }
          }
      }

  ApvlvDoc *
    ApvlvView::hasloaded (const char *filename)
      {
        char *abpath = absolutepath (filename);
        if (m_Docs[abpath] != NULL)
          {
            debug ("has loaded: %p", m_Docs[abpath]);
            return m_Docs[abpath];
          }
        return NULL;
      }

  void 
    ApvlvView::promptsearch ()
      {
        prostr = "";
        gtk_entry_set_text (GTK_ENTRY (statusbar), "/");
        cmd_mode = SEARCH;
        cmd_show ();
      }

  void 
    ApvlvView::promptbacksearch ()
      {
        prostr = "";
        gtk_entry_set_text (GTK_ENTRY (statusbar), "?");
        cmd_mode = BACKSEARCH;
        cmd_show ();
      }

  void 
    ApvlvView::promptcommand ()
      {
        prostr = "";
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

        char temp[256];

        if (crtadoc () != NULL && crtadoc ()->filename ())
          {
            snprintf (temp, sizeof temp, "\"%s\"\t%d/%d\t\t%d\%\t\t\t\t%d\%",
                      crtadoc ()->filename (),
                      crtadoc ()->pagenumber (),
                      crtadoc ()->pagesum (),
                      (int) (crtadoc ()->zoomvalue () * 100),
                      (int) (crtadoc ()->scrollrate () * 100)
            );
          }

        gtk_entry_set_text (GTK_ENTRY (statusbar), temp);

        gtk_widget_grab_focus (crtadoc ()->widget ());
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

  returnType 
    ApvlvView::process (int ct, guint key, guint st)
      {
        guint procmd = pro_cmd;
        if (pro_cmd != 0)
          {
            pro_cmd = 0;
            switch (procmd)
              {
              case 'w':
                return currentWindow ()->process (ct, key, st);
                break;

              case 'm':
                markposition (key);
                return MATCH;
                break;

              case '\'':
                jump (key);
                return MATCH;
                break;

              case 'z':
                if (key == 'i')
                  zoomin ();
                else if (key == 'o')
                  zoomout ();
                return MATCH;
                break;

              default:
                break;
              }

            return MATCH;
          }

        if (st == GDK_CONTROL_MASK)
          {
            switch (key)
              {
              case 'f':
                nextpage (ct);
                break;
              case 'b':
                prepage (ct);
                break;
              case 'd':
                halfnextpage (ct);
                break;
              case 'u':
                halfprepage (ct);
                break;
              case 'p':
                scrollup (ct);
                break;
              case 'n':
                scrolldown (ct);
                break;
              case 'w':
                pro_cmd = 'w';
                return NEED_MORE;
                break;
              default:
                break;
              }
          }
        else if (!st)
          {
            switch (key)
              {
              case ':':
                promptcommand ();
                return NEED_MORE;
              case '/':
                promptsearch ();
                return NEED_MORE;
              case '?':
                promptbacksearch ();
                return NEED_MORE;
              case 'k':
                scrollup (ct);
                break;
              case 'j':
                scrolldown (ct);
                break;
              case 'h':
                scrollleft (ct);
                break;
              case 'l':
                scrollright (ct);
                break;
              case 'R':
                reload ();
                break;
              case 'o':
                open ();
                break;
              case 'g':
                showpage (ct);
                break;
              case 'm':
                pro_cmd = 'm';
                return NEED_MORE;
                break;
              case '\'':
                pro_cmd = '\'';
                return NEED_MORE;
                break;
              case 'q':
                quit ();
                break;
              case 'f':
                fullscreen ();
                break;
              case 'z':
                pro_cmd = 'z';
                return NEED_MORE;
                break;
              default:
                return NO_MATCH;
                break;
              }
          }

        return MATCH;
      }

  void
    ApvlvView::run (const char *str)
      {
        switch (cmd_mode)
          {
          case SEARCH:
            crtadoc ()->search (str);
            break;

          case BACKSEARCH:
            crtadoc ()->backsearch (str);
            break;

          case COMMANDMODE:
            debug ("run: [%s]", str);
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
            else if (cmd == "sp")
              {
                currentWindow ()->separate (false);
              }
            else if (cmd == "vsp")
              {
                currentWindow ()->separate (true);
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
                ApvlvWindow *nwin = currentWindow ()->getneighbor (1, 'w', GDK_CONTROL_MASK);
                if (nwin == NULL)
                  {
                    quit ();
                  }
                else
                  {
                    delete currentWindow ();
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
        view->m_rootWindow->setsize (view->width, view->height - 20);
        gtk_widget_set_usize (view->statusbar, view->width, 20);
      }

  gint
    ApvlvView::apvlv_view_keypress_cb (GtkWidget * wid, GdkEvent * ev)
      {
        ApvlvView *view =
          (ApvlvView *) g_object_get_data (G_OBJECT (wid), "view");

#ifndef DEBUG
        GdkEventKey *gek = (GdkEventKey *) ev;
        debug ("ev: type=%d", gek->type);
        debug ("ev: send_event=%d", gek->send_event);
        debug ("ev: state=%d", gek->state);
        debug ("ev: keyval=%d", gek->keyval);
        debug ("ev: hardware_keycode=%d", gek->hardware_keycode);
        debug ("ev: length=%d", gek->length);
        debug ("ev: string=%s", gek->string);
#endif

        if (view->cmd_has == false)
          {
            gCmds->append ((GdkEventKey *) ev);
            return TRUE;
          }

        return FALSE;
      }

  gint
    ApvlvView::apvlv_view_statusbar_cb (GtkWidget * wid, GdkEvent * ev)
      {
        ApvlvView *view = (ApvlvView *) g_object_get_data (G_OBJECT (wid), "view");

        if (view->cmd_has == TRUE)
          {
            GdkEventKey *gek = (GdkEventKey *) ev;
            if (gek->keyval == GDK_Return)
              {
                gchar *str =
                  (gchar *) gtk_entry_get_text (GTK_ENTRY (view->statusbar));
                if (str && strlen (str) > 0)
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

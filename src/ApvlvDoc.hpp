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
#ifndef _APVLV_DOC_H_
#define _APVLV_DOC_H_

#ifdef HAVE_CONFIG_H
# include "config.hpp"
#endif

#include <gtk/gtk.h>
#include <glib/poppler.h>

#include <iostream>
#include <vector>
#include <map>

using namespace std;

namespace apvlv
{
  struct ApvlvDocPosition
    {
      int pagenum;
      double scrollrate;
    };

  class ApvlvWindow;

  class ApvlvDoc
    {
  public:
    ApvlvDoc (const char *zm = "NORMAL");
    ~ApvlvDoc ();

    // 
    // type == true? v separate: h separate
    ApvlvDoc *copy (bool type);

    void vseperate ();
    void hseperate ();

    //
    // type's meaning like 'j', 'k', 'l', 'm'
    ApvlvDoc *getneighbor (const char *type);

    const char *filename () { return doc? filestr.c_str (): NULL; }

    int pagenumber () { return pagenum + 1; }

    int pagesum () { return doc? poppler_document_get_n_pages (doc): 0; }

    double scrollrate ();

    double zoomvalue () { return zoomrate; }

    GtkWidget *widget ();

    bool loadfile (string & filename, bool check = true) { loadfile (filename.c_str (), check); }

    bool loadfile (const char *src, bool check = true);

    bool reload () { savelastposition (); return loadfile (filestr, false); }

    void setsize (int wid, int hei) { width = wid; height = hei; }

    void sizesmaller (int s = 1);

    void sizebigger (int s = 1);

    void markposition (const char s);

    void jump (const char s);

    void showpage (int p, double s = 0.00);

    void nextpage (int times = 1) { showpage (pagenum + times); }
    void prepage (int times = 1) { showpage (pagenum - times); }
    void halfnextpage (int times = 1);
    void halfprepage (int times = 1);

    void scrollup (int times = 1);
    void scrolldown (int times = 1);
    void scrollleft (int times = 1);
    void scrollright (int times = 1);

    void setzoom (const char *s);
    void setzoom (double d);
    void zoomin () { zoomrate *= 1.1; refresh (); }
    void zoomout () { zoomrate /= 1.1; refresh (); }

    void search (const char *str);
    void backsearch (const char *str);

    ApvlvWindow *getWindow ();
    void setWindow (ApvlvWindow *window);

    // 
    // for split window
    // left & right and up & down can't show at once
    ApvlvDoc *left, *right, *up, *down;

  private:
    PopplerPage *getpage (int p);
    void markselection ();
    bool needsearch (const char *str);
    GList * searchpage (int num);
    void refresh ();
    void scrollto (double s);
    bool savelastposition ();
    bool loadlastposition ();

    static gboolean apvlv_doc_first_scroll_cb (gpointer);

    static gboolean apvlv_doc_first_copy_cb (gpointer);

    ApvlvDoc *parent;
    vector <ApvlvDoc *> hchildren, vchildren;

    string filestr;
    PopplerDocument *doc;

    double scrollvalue;
    map <char, ApvlvDocPosition> positions;

    GList *results;
    string searchstr;

    PopplerPage *page;
    guchar *pagedata;
    GdkPixbuf *pixbuf;
    int pagenum;
    double vrate, hrate;
    double pagex, pagey;

    enum
      {
        NORMAL,
        FITWIDTH,
        FITHEIGHT,
        CUSTOM
      } zoommode;
    double zoomrate;
    bool zoominit;
    int lines, chars;
    int width, height;
    GtkAdjustment *vaj, *haj;

    // vbox and hbox for multiple windows
    // vbox will be the main widget
    GtkWidget *scrollwin, *image;

    ApvlvWindow *m_window;
    };
}

#endif

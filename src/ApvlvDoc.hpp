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

#include <gtk/gtk.h>
#include <glib/poppler.h>

#include <iostream>
#include <vector>
using namespace std;

namespace apvlv
{
  class ApvlvDoc
    {
  public:
    ApvlvDoc (const char *zm = "NORMAL", GtkWidget *vbox = NULL, GtkWidget *hbox = NULL);
    ~ApvlvDoc ();

    void vseperate ();
    void hseperate ();

    GtkWidget *widget () { return vbox; }

    bool loadfile (string & filename) { loadfile (filename.c_str ()); }
    bool loadfile (const char *src);

    void setsize (int wid, int hei) { width = wid; height = hei; }
    void showpage (int p);

    void prepage (int times = 1) { showpage (pagenum - times); }
    void nextpage (int times = 1) { showpage (pagenum + times); }

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

  private:
    PopplerPage *getpage (int p);
    void markselection ();
    bool needsearch (const char *str);
    GList * searchpage (int num);
    void refresh ();

    ApvlvDoc *parent;
    vector <ApvlvDoc *> hchildren, vchildren;

    PopplerDocument *doc;

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
    int width, height;
    GtkAdjustment *vaj, *haj;

    // vbox and hbox for multiple windows
    // vbox will be the main widget
    GtkWidget *vbox, *hbox, *scrollwin, *image;
    };
}

#endif

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
#ifndef _APVLV_WINDOW_H_
#define _APVLV_WINDOW_H_

#ifdef HAVE_CONFIG_H
# include "config.hpp"
#endif

#include "ApvlvDoc.hpp"

#include <gtk/gtk.h>

#include <iostream>
using namespace std;

namespace apvlv
{
  class ApvlvDoc;

  class ApvlvWindow
    {
  public:
      ApvlvWindow (ApvlvDoc *doc);
      ~ApvlvWindow ();

      ApvlvWindow *copy (int type);

      GtkWidget *widget () { return m_Doc? m_Doc->widget (): m_blank; }

      ApvlvDoc *loadDoc (const char *filename);

      void setDoc (ApvlvDoc *doc);

      ApvlvDoc *getDoc () { return m_Doc; }

      void setsize (int wid, int hei);

      void vseparate ();
      void hseparate ();

      void firstminner (int times = 1);
      void firstmaxer (int times = 1);
      void secondminner (int times = 1);
      void secondmaxer (int times = 1);

      ApvlvWindow *m_left, *m_right;

  private:

      GtkWidget *m_blank;

      ApvlvDoc *m_Doc;

      int m_width, m_height;

      enum windowtype { SINGLE, VDOUBLE, HDOUBLE } type;
    };
}

#endif

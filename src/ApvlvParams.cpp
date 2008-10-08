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

#include <iostream>
#include <fstream>
#include <sstream>

namespace apvlv
{
  ApvlvParams::ApvlvParams ()
    {
      push ("o", "open");
      push ("q", "quit");
      push ("g", "goto");
      push ("C-f", "nextpage");
      push ("C-b", "prepage");
      push ("k", "scrollup");
      push ("j", "scrolldown");
      push ("h", "scrollleft");
      push ("l", "scrollright");
      push ("zi", "zoomin");
      push ("zo", "zoomout");
      push ("f", "fullscreen");
      push ("/", "search");
      push ("?", "backsearch");
      push (":", "commandmode");
      push ("fullscreen", "no");
      push ("zoom", "fitwidth");
      push ("width", "800");
      push ("height", "600");
      push ("commandtimeout", "1000");
      push ("defaultdir", "/tmp");
    }

  ApvlvParams::~ApvlvParams ()
    {
      params.clear ();
    }

  bool ApvlvParams::loadfile (string & filename)
    {
      fstream os (filename.c_str (), ios::in);
      string str;
      while ((getline (os, str)) != NULL)
        {
          string argu, data, crap;
          stringstream is (str);
          // avoid commet line, continue next
          is >> crap;
          if (crap == "\"" || crap == "")
            {
              continue;
            }
          // parse the line like "set fullscreen=yes" or set "set zoom=1.5"
          else if (crap == "set")
            {
              is >> argu;
              int off = argu.find ('=');
              if (off == string::npos) 
                {
                  is >> crap >> data;
                  if (crap == "=" && data.length () > 0)
                    {
                      push (argu, data);
                      continue;
                    }
                }
              else if (off < 32)
                {
                  char k[32], v[32], *p;
                  memcpy (k, argu.c_str (), off);
                  k[off] = '\0';

                  p = (char *) argu.c_str () + off + 1;
                  while (isspace (*p))
                    p ++;

                  if (*p != '\0')
                    {
                      strncpy (v, argu.c_str () + off + 1, 31);
                      v[31] = '\0';
                      push (k, v);
                      continue;
                    }
                }

              cerr << "Syntax error: set: " << str << endl;
            }
          // like "map n next-page"
          else if (crap == "map")
            {
              is >> argu >> data;
              if (argu.length () > 0 && data.length () > 0)
                {
                  push (argu, data);
                }
              else
                {
                  cerr << "Syntax error: map: " << str << endl;
                }
            }
          else
            {
              cerr << "Unknown rc cmd: " << crap << ": " << str << endl;
            }
        }
    }

  bool
    ApvlvParams::push (string &ch, string &str)
      {
        params[ch] = str;
      }

  const char *
    ApvlvParams::key (const char *s)
      {
        map <string, string>::iterator it;
        for (it = params.begin (); it != params.end (); ++ it)
          {
            if (strcmp ((*it).second.c_str (), s) == 0)
              {
                return (*it).first.c_str ();
              }
          }
      }


  const char *
    ApvlvParams::value (const char *s)
      {
        map <string, string>::iterator it;
        for (it = params.begin (); it != params.end (); ++ it)
          {
            if (strcmp ((*it).first.c_str (), s) == 0)
              {
                return (*it).second.c_str ();
              }
          }
        return NULL;
      }
}

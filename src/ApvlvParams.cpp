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
#include "ApvlvUtil.hpp"
#include "ApvlvParams.hpp"

#include <string.h>

#include <iostream>
#include <fstream>
#include <sstream>

namespace apvlv
{
  ApvlvParams *gParams = NULL;

  ApvlvParams::ApvlvParams ()
    {
      settingpush ("fullscreen", "no");
      settingpush ("zoom", "fitwidth");
      settingpush ("width", "800");
      settingpush ("height", "600");
      settingpush ("commandtimeout", "1000");
      settingpush ("defaultdir", "/tmp");
    }

  ApvlvParams::~ApvlvParams ()
    {
      m_maps.clear ();
      m_settings.clear ();
    }

  bool ApvlvParams::loadfile (const char *filename)
    {
      string str;
      fstream os (filename, ios::in);

      if (! os.is_open ())
        {
          err ("Open configure file %s error", filename);
          return false;
        }

      while ((getline (os, str)) != NULL)
        {
          string argu, data, crap;
          stringstream is (str);
          // avoid commet line, continue next
          is >> crap;
          if (crap[0] == '\"' || crap == "")
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
                      settingpush (argu.c_str (), data.c_str ());
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
                      settingpush (k, v);
                      continue;
                    }
                }

              err ("Syntax error: set: %s", str.c_str ());
            }
          // like "map n next-page"
          else if (crap == "map")
            {
              is >> argu;
              getline (is, data);
              while (data[0] == ' ' || data[0] == '\t')
                data.erase (0, 1);
              if (argu.length () > 0 && data.length () > 0)
                {
                  mappush (argu, data);
                }
              else
                {
                  err ("Syntax error: map: %s", str.c_str ());
                }
            }
          else
            {
              err ("Unknown rc command: %s: %s", crap.c_str (), str.c_str ());
            }
        }
    }

  bool
    ApvlvParams::mappush (string &ch, string &str)
      {
        m_maps[ch] = str;
      }

  bool
    ApvlvParams::settingpush (const char *ch, const char *str)
      {
        string sch (ch), sstr (str);
        m_settings[sch] = sstr;
      }

  const char *
    ApvlvParams::mapvalue (const char *s)
      {
        string ss (s);
        map <string, string>::iterator it;
        it = m_maps.find (ss);
        if (it != m_maps.end ())
          {
            return (*it).second.c_str ();
          }
        return NULL;
      }

  returnType 
    ApvlvParams::getmap (const char *s, int n)
      {
        map <string, string>::iterator it;
        for (it = m_maps.begin (); it != m_maps.end (); ++ it)
          {
            if (strncmp (it->first.c_str (), s, n) == 0)
              {
                if (it->first.size () == n)
                  return MATCH;
                else 
                  return NEED_MORE;
              }
          }
        return NO_MATCH;
      }

  const char *
    ApvlvParams::settingvalue (const char *s)
      {
        string ss (s);
        map <string, string>::iterator it;
        it = m_settings.find (ss);
        if (it != m_settings.end ())
          {
            return (*it).second.c_str ();
          }
        return NULL;
      }
}

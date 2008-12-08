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
#ifndef _APVLV_PARAMS_H_
#define _APVLV_PARAMS_H_

#ifdef HAVE_CONFIG_H
# include "config.hpp"
#endif

#include "ApvlvUtil.hpp"

#include <iostream>
#include <string>
#include <map>

using namespace std;

namespace apvlv
{
  class ApvlvParams
    {
  public:
    ApvlvParams ();
    ~ApvlvParams ();

    bool loadfile (const char *filename);

    bool mappush (string &cmd1, string &cmd2);

    const char *mapvalue (const char *key);

    returnType getmap (const char *s, int n);

    const char *cmd (const char *key);

    bool settingpush (string &ch, string &str);

    bool settingpush (const char *c, const char *s);

    const char *settingvalue (const char *key);
    
    //for debug
    void show ()
      {
        map <string, string>::iterator it;

        cerr << "maps" << endl;
        for (it = m_maps.begin (); it != m_maps.end (); ++ it)
          {
            cerr << "first:[" << (*it).first << "], second[" << (*it).second << "]" << endl;
          }
        cerr << endl;

        cerr << "settings" << endl;
        for (it = m_settings.begin (); it != m_settings.end (); ++ it)
          {
            cerr << "first:[" << (*it).first << "], second[" << (*it).second << "]" << endl;
          }
      }

  private:
    map <string, string> m_maps, m_settings;
    };

  extern ApvlvParams *gParams;
}

#endif

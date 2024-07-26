/*
 * This file is part of the apvlv package
 *
 * Copyright (C) 2008 Alf.
 *
 * Contact: Alf <naihe2010@126.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2.0 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */
/* @CPPFILE ApvlvUtil.h
 *
 *  Author: Alf <naihe2010@126.com>
 */

#ifndef _APVLV_UTIL_H_
#define _APVLV_UTIL_H_ 1

#include <QXmlStreamReader>

using namespace std;

namespace apvlv
{
// Global files
extern string helppdf;
extern string iniexam;
extern string icondir;
extern string iconfile;
extern string iconpage;
extern string translations;

extern string inifile;
extern string sessionfile;
extern string logfile;

void getRuntimePaths ();

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

#ifdef WIN32
#define PATH_SEP_C '\\'
#define PATH_SEP_S "\\"
#else
#define PATH_SEP_C '/'
#define PATH_SEP_S "/"
#endif

optional<unique_ptr<QXmlStreamReader> >
xml_content_get_element (const char *content, size_t length,
                         const vector<string> &names);

string xml_stream_get_attribute_value (QXmlStreamReader *xml,
                                       const string &key);

string xml_content_get_attribute_value (const char *content, size_t length,
                                        const vector<string> &names,
                                        const string &key);

string filename_ext (const string &filename);

// command type
enum
{
  CMD_NONE,
  CMD_MESSAGE,
  CMD_CMD
};

// function return type
enum ReturnType
{
  MATCH,
  NEED_MORE,
  NO_MATCH,
};

// some windows macro
#ifdef WIN32
#define __func__ __FUNCTION__
#define strcasecmp _strcmpi
#endif

// char macro
// because every unsigned char is < 256, so use this marco to stand for
// Ctrl+char, Shift+char
#define CTRL(c) ((c) + 256)

}
#endif

// Local Variables:
// mode: c++
// mode: c++
// End:

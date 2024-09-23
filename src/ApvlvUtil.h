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

#include <QImage>
#include <QXmlStreamReader>

namespace apvlv
{

// Global files
extern std::string helppdf;
extern std::string iniexam;
extern std::string icondir;
extern std::string iconfile;
extern std::string iconpage;
extern std::string translations;

extern std::string inifile;
extern std::string sessionfile;
extern std::string logfile;

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

std::optional<std::unique_ptr<QXmlStreamReader> >
xml_content_get_element (const char *content, size_t length,
                         const std::vector<std::string> &names);

std::string xml_stream_get_attribute_value (QXmlStreamReader *xml,
                                            const std::string &key);

std::string
xml_content_get_attribute_value (const char *content, size_t length,
                                 const std::vector<std::string> &names,
                                 const std::string &key);

std::string filename_ext (const std::string &filename);

void imageArgb32ToRgb32 (QImage &image, int left, int top, int right,
                         int bottom);

}
#endif

// Local Variables:
// mode: c++
// mode: c++
// End:

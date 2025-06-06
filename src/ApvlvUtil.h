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
#define _APVLV_UTIL_H_

#include <QImage>
#include <QXmlStreamReader>
#include <filesystem>
#include <string>
#include <vector>

namespace apvlv
{

// Global files
extern std::string ScriptDir;
extern std::string HelpPdf;
extern std::string IniExam;
extern std::string IconDir;
extern std::string IconFile;
extern std::string IconPage;
extern std::string Translations;

extern std::string IniFile;
extern std::string SessionFile;
extern std::string LogFile;
extern std::string NotesDir;
extern std::string UserScriptDir;

void getRuntimePaths ();

#ifdef WIN32
const char PATH_SEP_C = '\\';
const char *const PATH_SEP_S = "\\";
#else
const char PATH_SEP_C = '/';
const char *const PATH_SEP_S = "/";
#endif

std::optional<std::unique_ptr<QXmlStreamReader>>
xmlContentGetElement (const char *content, size_t length,
                      const std::vector<std::string> &names);

std::string xmlStreamGetAttributeValue (QXmlStreamReader *xml,
                                        const std::string &key);

std::string xmlContentGetAttributeValue (const char *content, size_t length,
                                         const std::vector<std::string> &names,
                                         const std::string &key);

std::string filenameExtension (const std::string &filename);

void imageArgb32ToRgb32 (QImage &image, int left, int top, int right,
                         int bottom);

std::string templateBuild (std::string_view temp, std::string_view token,
                           std::string_view real);

qint64 parseFormattedDataSize (const QString &sizeStr);

qint64 filesystemTimeToMSeconds (std::filesystem::file_time_type ftt);

}
#endif

// Local Variables:
// mode: c++
// mode: c++
// End:

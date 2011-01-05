/*
* This file is part of the libumd package
* Copyright (C) <2010>  <Alf>
*
* Contact: Alf <naihe2010@gmail.com>
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License along
* with this program; if not, write to the Free Software Foundation, Inc.,
* 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*
*/
/* @CFILE umd_internals.h
*
*  Author: Alf <naihe2010@gmail.com>
*/
/* @date Created: 2010/12/07 16:42:27 Alf*/

#ifndef _LIBUMD_INTERNALS_H_
#define _LIBUMD_INTERNALS_H_

#include <iconv.h>

#ifdef WIN32

#define strdup	_strdup

#include <direct.h>
#define mkdir(a,b) _mkdir(a)

#ifndef inline
#define inline
#endif

#endif

#define UMD_L(u,l,...)        do {              \
    if (u->log && l >= u->loglevel) {           \
        u->log (u->logarg, __VA_ARGS__);        \
    }                                           \
} while (0)

#define UMD_DEBUG(u, ...)    UMD_L (u, 0, __VA_ARGS__)
#define UMD_INFO(u, ...)     UMD_L (u, 1, __VA_ARGS__)
#define UMD_ERROR(u, ...)    UMD_L (u, 2, __VA_ARGS__)

/** UMD magic number at begining of a UMD file */
#define UMD_MAGIC               0xde9a9b89

/** UMD seperator between every control segments */
#define UMD_CONTROL_SEPARATOR   '#'

/** UMD seperator between every data segments */
#define UMD_DATA_SEPARATOR      '$'

/** UMD Text book Version */
#define UMD_TEXT_VERSION        0x01
/** UMD Text book Title */
#define UMD_TEXT_TITLE          0x02
/** UMD Text book Author */
#define UMD_TEXT_AUTHOR         0x03
/** UMD Text book Publish Year */
#define UMD_TEXT_YEAR           0x04
/** UMD Text book Publish Month */
#define UMD_TEXT_MONTH          0x05
/** UMD Text book Publish Day */
#define UMD_TEXT_DAY            0x06
/** UMD Text book Type */
#define UMD_TEXT_TYPE           0x07
/** UMD Text book Publisher */
#define UMD_TEXT_PUBLISHER      0x08
/** UMD Text book Seller */
#define UMD_TEXT_VENDOR         0x09
/** UMD Text CID */
#define UMD_TEXT_CID            0x0a
/** UMD Text book Content Length */
#define UMD_TEXT_CONTENT_LEN    0x0b
/** UMD Text book File End */
#define UMD_TEXT_FILE_END       0x0c
/** UMD Text book End */
#define UMD_TEXT_END            0x81
/** UMD Text book Top Page */
#define UMD_TEXT_TOPPAGE        0x82
/** UMD Text book Chapter Offset */
#define UMD_TEXT_CHAPTER_OFFSET 0x83
/** UMD Text book Chapter Offset */
#define UMD_TEXT_CHAPTER_TITLE  0x84
/** UMD Text book Page Offset */
#define UMD_TEXT_PAGE_OFFSET    0x87

#ifndef LIBUMD_PAGE_CHARS
#define LIBUMD_PAGE_CHARS               0x1000
#endif

typedef struct umd_content_s umd_content_t;
typedef struct umd_chapter_context_s umd_chapter_context_t;
typedef struct umd_pageinfo_s umd_pageinfo_t;

struct umd_content_s
{
  unsigned char randval[4];
  size_t length;
  char *content;
  umd_content_t *next;
};

struct umd_chapter_context_s
{
  char *title;
  size_t offset;
  size_t length;

  size_t page_start;
  size_t page_end;
};

struct umd_pageinfo_s
{
  size_t id;
  char *char_begin;
  size_t length;
  umd_pageinfo_t *next;
};

struct umd_s
{
  int book_type;

  char s_title[0x100];
  char s_author[0x100];
  char s_year[0x100];
  char s_month[0x100];
  char s_day[0x100];
  char s_publisher[0x100];
  char s_vendor[0x100];
  char s_type[0x100];

  int cover_type;
  umd_content_t *cover_content;

  umd_chapter_context_t *chapter_list;
  size_t chapter_count;

  umd_pageinfo_t *pageinfo_head;
  umd_pageinfo_t *pageinfo_tail;
  size_t page_count;

  int loglevel;
  umd_log log;
  void *logarg;

  const char *render_font;
  int render_italic;
  int render_bold;

  umd_content_t *content_head;
  umd_content_t *content_tail;

  const char *to_encoding;
  iconv_t iconv;

  char *total_string;
  size_t total_length;
};

#endif

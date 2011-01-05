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
/* @CFILE umd.h
*
*  Author: Alf <naihe2010@gmail.com>
*/
/* @date Created: 2010/12/02 17:43:13 Alf*/

#ifndef _UMD_H_
#define _UMD_H_

#include <stdio.h>

/**
        \mainpage LibUMD documentation

        \section Introduction

        This document describes the data structures and the functions exported
        by the LibUMD library. <br>
        The LibUMD library  provides access and render UMD files.

        \b Sections

        - \ref libumd
 */

/**
  @defgroup libumd LibUMD definitions, data structures and functions
  @{
 */

/** UMD log callback */
#define UMD_LOG(f)         ((void (*) (void *, const char *, ...)) f)

/** UMD log callback function */
typedef void (*umd_log) (void *, const char *, ...);

/** UMD type */
typedef enum
{
  /** common text */
  UMD_TEXT,

  /** image */
  UMD_IMAGE
} umd_type_t;

/** UMD book type */
typedef struct umd_s umd_t;

/** UMD book page */
typedef struct umd_page_s umd_page_t;

/** UMD book cover image */
typedef struct
{
    /** type, 0 means jpeg */
  int type;
    /** raw data of image */
  char *image_data;
    /** data length */
  size_t image_size;
} umd_cover_t;

/** UMD book chapter struct */
typedef struct
{
  /** chapter title */
  char *title;
  /** content */
  char *content;
  /** content length */
  size_t length;
  /** page index */
  size_t page_index;
} umd_chapter_t;

#ifdef __cplusplus
extern "C"
{
#endif

/**
  \brief new a umd struct
  \return umd_t umd struct
 */
  umd_t *umd_new ();

/**
 \brief free the umd struct resources, but not free ths struct
 \param umd umd struct
 \return null
 */
  void umd_free (umd_t * umd);

/**
 \brief destroy a umd struct. This will free the umd struct
 \param umd umd struct pointer
 \return null
 */
  void umd_destroy (umd_t * umd);

/**
  \brief new a umd from a input file
  \param file input file
  \return umd_t umd struct
 */
  umd_t *umd_new_from_file (const char *file);

/**
 \brief parser a umd file to umd struct
 \param umd umd struct
 \param file umd file name
 \return int return 0 on OK or < 0 on error */
  int umd_parser_file (umd_t * umd, const char *file);

/**
 \brief set umd log callback
 \param umd umd struct
 \param loglevel log level: 0, 1, or 2
 \param log log callback function
 \param arg user argument
 \return null
 */
  void umd_set_log (umd_t * umd, int loglevel, umd_log log, void *arg);

/**
 \brief set output encoding
 \param umd umd struct
 \param encoding output encoding string, like UTF8, GBK, ...
 \return null
 */
  void umd_set_encoding (umd_t * umd, const char *encoding);

/**
 \brief set render font face
 \param umd umd struct
 \param face font family, like 'Sans', 'serif', ...
 \param italic weather font is italic
 \param bold weather font is bold
 \return null
 */
  void umd_set_font_face (umd_t * umd, const char *face, int italic,
			  int bold);

/**
 \brief get book title
 \param umd umd struct
 \return new allocated char string. Warning: need freed by user.
 */
  char *umd_get_title (umd_t * umd);

/**
 \brief get book author
 \param umd umd struct
 \return new allocated char string. Warning: need freed by user.
 */
  char *umd_get_author (umd_t * umd);

/**
 \brief get book year
 \param umd umd struct
 \return new allocated char string. Warning: need freed by user.
 */
  char *umd_get_year (umd_t * umd);

/**
 \brief get book month
 \param umd umd struct
 \return new allocated char string. Warning: need freed by user.
 */
  char *umd_get_month (umd_t * umd);

/**
 \brief get book day
 \param umd umd struct
 \return new allocated char string. Warning: need freed by user.
 */
  char *umd_get_day (umd_t * umd);

/**
 \brief get book type
 \param umd umd struct
 \return new allocated char string. Warning: need freed by user.
 */
  char *umd_get_type (umd_t * umd);

/**
 \brief get book publisher
 \param umd umd struct
 \return new allocated char string. Warning: need freed by user.
 */
  char *umd_get_publisher (umd_t * umd);

/**
 \brief get book vendor
 \param umd umd struct
 \return new allocated char string. Warning: need freed by user.
 */
  char *umd_get_vendor (umd_t * umd);

/**
 \brief get book content 
 \param umd umd struct
 \param lenp size_t pointer for save the string length
 \return new allocated char string. Warning: need freed by user.
 */
  char *umd_get_content (umd_t * umd, size_t * lenp);

/**
 \brief get chapter count of a umd struct
 \param umd umd struct
 \return int count
 \return new allocated char string. Warning: need freed by user.
 */
  int umd_get_chapter_n (umd_t * umd);

/**
 \brief get chapter of a umd struct
 \param umd umd struct
 \param id chapter id
 \return umd_chapter_t
 */
  umd_chapter_t *umd_get_nth_chapter (umd_t * umd, size_t id);

/**
 \brief get cover of a umd struct
 \param umd umd struct
 \return umd_cover_t
 */
  umd_cover_t *umd_get_cover (umd_t * umd);

/**
  \brief write a umd_t to a file
  \param umd umd struct
  \param file output file name
  \return int
 */
  int *umd_write_file (umd_t * umd, const char *file);

/**
 \brief write a umd_t to a file descriptor
 \param umd umd struct
 \param fp output file descriptor
 \return int
 */
  int *umd_write_fp (umd_t * umd, FILE * fp);

/**
 \brief printf umd struct for debug
 \param umd umd struct
 \param fp file pointer
 \return null
 */
  void umd_fprintf (umd_t * umd, FILE * fp);

/**
 \brief get page count of a umd struct
 \param umd umd struct
 \return int count
 */
  int umd_get_page_n (umd_t * umd);

/**
 \brief get page of a umd struct
 \param umd umd struct
 \param id page id
 \return umd_page_t
 */
  umd_page_t *umd_get_nth_page (umd_t * umd, size_t id);

/**
 \brief get page content
 \param page umd page struct
 \param buf output buffer
 \param size output buffer size
 \return content size
 */
  int umd_page_get_content (umd_page_t * page, char *buf, size_t size);

/**
 \brief free a page
 \param page umd page struct
 */
  void umd_page_free (umd_page_t * page);

/**
 \brief get page size
 \param page umd_page_t struct
 \param px width pointer
 \param py height pointer
 \return 0 on OK or < 0 on error
 */
  int umd_page_get_size (umd_page_t * page, int *px, int *py);

/**
  \brief render umd page to a gdk-pixbuf
  \param page umd_page_t struct
  \param fromx from x 
  \param fromy from y
  \param tox to x
  \param toy to y
  \param zm zoom rate
  \param rot weather rotation
  \param buffer output buffer
  \param size buffer size
  \return 0 on OK or < 0 on error
  */
  int umd_page_render (umd_page_t * page,
		       int fromx, int fromy,
		       int tox, int toy,
		       double zm, int rot,
		       unsigned char *buffer, size_t size);

#ifdef __cplusplus
}
#endif

/**
  @}
 */

#endif

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
/* @CFILE umd2txt.c
*
*  Author: Alf <naihe2010@gmail.com>
*/
/* @date Created: 2010/12/10 11:44:26 Alf*/

#include <stdlib.h>

#include "umd.h"

int
main (int argc, char *argv[])
{
  umd_t *umd;
  umd_chapter_t *chapter;
  int i, cpt_count;
  size_t length;
  char *string;

  if (argc < 2)
    {
      return 1;
    }

  umd = umd_new_from_file (argv[1]);
  if (umd == NULL)
    {
      return 2;
    }

#define PRINT_ARGU(u,a,n) do {                  \
  string = umd_get_##a (u);                     \
  if (string != NULL)                           \
    {                                           \
      fprintf (stdout, "%s"n, string);          \
      free (string);                            \
    } } while (0)

  PRINT_ARGU (umd, title, "\n\n");
  PRINT_ARGU (umd, type, "\n\n");
  PRINT_ARGU (umd, author, "\n\n");
  PRINT_ARGU (umd, publisher, "\n\n");
  PRINT_ARGU (umd, vendor, "\n\n");

  cpt_count = umd_get_chapter_n (umd);
  if (cpt_count == 0)
    {
      string = umd_get_content (umd, &length);
      if (string == NULL)
	{
	  return 31;
	}
      fwrite (string, 1, length, stdout);
    }
  else
    {
      for (i = 0; i < cpt_count; ++i)
	{
	  chapter = umd_get_nth_chapter (umd, i);
	  if (chapter == NULL)
	    {
	      return 32;
	    }
	  fprintf (stdout, "%d: %s [%u]\n",
		   i + 1, chapter->title, chapter->length);
	  fprintf (stdout,
		   "----------------------------------------------------->\n");
	  fwrite (chapter->content, 1, chapter->length, stdout);
	  fprintf (stdout,
		   "<---------------------------------------------------\n\n");

	  free (chapter->title);
	  free (chapter->content);
	  free (chapter);
	}
    }

  fprintf (stdout, "\n\n");
  PRINT_ARGU (umd, year, ".");
  PRINT_ARGU (umd, month, ".");
  PRINT_ARGU (umd, day, ".");

  umd_destroy (umd);

  return 0;
}

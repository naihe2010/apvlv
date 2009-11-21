/*
* This file is part of the apvlv package
* Copyright (C) <2008>  <Alf>
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
/* @CFILE ApvlvFile.cpp xxxxxxxxxxxxxxxxxxxxxxxxxx.
*
*  Author: Alf <naihe2010@gmail.com>
*/
/* @date Created: 2009/11/20 19:38:30 Alf*/

#include "ApvlvFile.hpp"
#include "ApvlvUtil.hpp"

#include <iostream>
#include <fstream>

#include <sys/stat.h>
#include <glib.h>
#include <glib/poppler.h>

namespace apvlv
{
  ApvlvFile::ApvlvFile (const char *filename, bool check)
  {
  }

  ApvlvFile::~ApvlvFile ()
  {
  }

  ApvlvPDF::ApvlvPDF (const char *filename, bool check):ApvlvFile (filename,
								   check)
  {
    static gchar *mRawdata = NULL;
    static guint mRawdatasize = 0;
    gchar *wfilename;

    if (filename == NULL
	|| *filename == '\0'
	|| g_file_test (filename, G_FILE_TEST_IS_REGULAR) == FALSE
	|| (wfilename =
	    g_locale_from_utf8 (filename, -1, NULL, NULL, NULL)) == NULL)
      {
	errp ("filename error: %s", filename ? filename : "No name");
	throw std::bad_alloc ();
      }

    size_t filelen;
    struct stat sbuf;
    int rt = stat (wfilename, &sbuf);
    if (rt < 0)
      {
	errp ("Can't stat the PDF file: %s.", filename);
	throw std::bad_alloc ();
      }
    filelen = sbuf.st_size;

    if (mRawdata != NULL && mRawdatasize < filelen)
      {
	delete[]mRawdata;
	mRawdata = NULL;
      }

    if (mRawdata == NULL)
      {
	mRawdata = new char[filelen];
	mRawdatasize = filelen;
      }

    ifstream ifs (wfilename, ios::binary);
    if (ifs.is_open ())
      {
	ifs.read (mRawdata, filelen);
	ifs.close ();
      }

    g_free (wfilename);

    GError *error = NULL;
    mDoc = poppler_document_new_from_data (mRawdata, filelen, NULL, &error);

    if (mDoc == NULL && error && error->code == POPPLER_ERROR_ENCRYPTED)
      {
        g_error_free (error);

	GtkWidget *dia = gtk_message_dialog_new (NULL,
						 GTK_DIALOG_DESTROY_WITH_PARENT,
						 GTK_MESSAGE_QUESTION,
						 GTK_BUTTONS_OK_CANCEL,
						 error->message);

	GtkWidget *entry = gtk_entry_new ();
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dia)->vbox), entry, TRUE,
			    TRUE, 10);
	gtk_widget_show (entry);

	int ret = gtk_dialog_run (GTK_DIALOG (dia));
	if (ret == GTK_RESPONSE_OK)
	  {
	    gchar *ans = (gchar *) gtk_entry_get_text (GTK_ENTRY (entry));
	    if (ans != NULL)
	      {
		mDoc =
		  poppler_document_new_from_data (mRawdata, filelen, ans,
						  NULL);
	      }
	  }

	gtk_widget_destroy (dia);
      }

    if (mDoc == NULL)
      {
	throw std::bad_alloc ();
      }
  }

  ApvlvPDF::~ApvlvPDF ()
  {
    if (mDoc)
      {
	debug ("Free the PopplerDocument");
	debug ("And, Maybe there is some bugs in poppler-glib libiray");
	g_object_unref (mDoc);
      }
  }

  bool ApvlvPDF::pagesize (int pn, int rot, double *x, double *y)
  {
    PopplerPage *page = poppler_document_get_page (mDoc, pn);
    if (page != NULL)
      {
	if (rot == 90 || rot == 270)
	  {
	    poppler_page_get_size (page, y, x);
	  }
	else
	  {
	    poppler_page_get_size (page, x, y);
	  }
	return true;
      }
    return false;
  }

  int ApvlvPDF::pagesum ()
  {
    return mDoc ? poppler_document_get_n_pages (mDoc) : 0;
  }

  ApvlvPoses *ApvlvPDF::pagesearch (int pn, bool reverse)
  {
    return NULL;
  }

  bool ApvlvPDF::pagetext (int pn, char **out)
  {
    double x, y;
    pagesize (pn, 0, &x, &y);
    PopplerRectangle rect = { 0, 0, x, y };
    PopplerPage *page = poppler_document_get_page (mDoc, pn);
    *out = poppler_page_get_text (page, POPPLER_SELECTION_WORD, &rect);
    if (*out != NULL)
      {
	return true;
      }
    return false;
  }

  bool ApvlvPDF::render (int pn, int ix, int iy, double zm, int rot,
			 GdkPixbuf * pix, char *buffer)
  {
    PopplerPage *tpage;

    if ((tpage = poppler_document_get_page (mDoc, pn)) == NULL)
      {
	debug ("no this page: %d", pn);
	return false;
      }

    poppler_page_render_to_pixbuf (tpage, 0, 0, ix, iy, zm, rot, pix);
    return true;
  }

  ApvlvLinks *ApvlvPDF::getlinks (int pn)
  {
    PopplerPage *page = poppler_document_get_page (mDoc, pn);
    GList *list = poppler_page_get_link_mapping (page);
    if (list == NULL)
      {
	return NULL;
      }

    ApvlvLinks *links = new ApvlvLinks;

    for (GList * tmp = list; tmp != NULL; tmp = g_list_next (tmp))
      {
	PopplerLinkMapping *map = (PopplerLinkMapping *) tmp->data;
	if (map)
	  {
	    PopplerAction *act = map->action;
	    if (act && *(PopplerActionType *) act == POPPLER_ACTION_GOTO_DEST)
	      {
		PopplerDest *pd = ((PopplerActionGotoDest *) act)->dest;
		if (pd->type == POPPLER_DEST_NAMED)
		  {
		    PopplerDest *destnew = poppler_document_find_dest (mDoc,
								       pd->named_dest);
		    if (destnew != NULL)
		      {
			ApvlvLink link = { "", destnew->page_num - 1 };
			links->insert (links->begin (), link);
			poppler_dest_free (destnew);
		      }
		  }
		else
		  {
		    ApvlvLink link = { "", pd->page_num - 1 };
		    links->insert (links->begin (), link);
		  }
	      }
	  }
      }

    return links;
  }

  ApvlvFileIndex *ApvlvPDF::new_index ()
  {
    if (mIndex != NULL)
      {
	return mIndex;
      }

    PopplerIndexIter *itr = poppler_index_iter_new (mDoc);
    if (itr == NULL)
      {
	return NULL;
      }

    mIndex = new ApvlvFileIndex;
    walk_poppler_index_iter (NULL, itr);

    return mIndex;
  }

  void ApvlvPDF::free_index (ApvlvFileIndex * index)
  {
  }

  bool ApvlvPDF::walk_poppler_index_iter (ApvlvFileIndex * titr,
					  PopplerIndexIter * iter)
  {
    bool has = false;
    do
      {
	ApvlvFileIndex *index = NULL;

	PopplerAction *act = poppler_index_iter_get_action (iter);
	if (act)
	  {
	    if (*(PopplerActionType *) act == POPPLER_ACTION_GOTO_DEST)
	      {
		PopplerActionGotoDest *pagd = (PopplerActionGotoDest *) act;
		if (pagd->dest->type == POPPLER_DEST_NAMED)
		  {
		    PopplerDest *destnew = poppler_document_find_dest (mDoc,
								       pagd->
								       dest->
								       named_dest);
		    int pn = 1;
		    if (destnew != NULL)
		      {
			pn = destnew->page_num - 1;
			poppler_dest_free (destnew);
		      }
		    index = new ApvlvFileIndex;
		    index->page = pn;
		  }
		else
		  {
		    index = new ApvlvFileIndex;
		    index->page = pagd->dest->page_num;
		  }

		if (index != NULL)
		  {
		    has = true;
		    index->title = pagd->title;
		    if (titr != NULL)
		      {
			titr->children.push_back (*index);
			delete index;
			index = &(titr->children[titr->children.size () - 1]);
		      }
		    else
		      {
			mIndex->children.push_back (*index);
			delete index;
			index =
			  &(mIndex->children[titr->children.size () - 1]);
		      }
		  }
	      }
	    poppler_action_free (act);
	  }

	PopplerIndexIter *child = poppler_index_iter_get_child (iter);
	if (child)
	  {
	    bool chas = walk_poppler_index_iter (has ? index : NULL, child);
	    has = has ? has : chas;
	    poppler_index_iter_free (child);
	  }
      }
    while (poppler_index_iter_next (iter));
    return has;
  }

#ifdef HAVE_LIBDJVU
  void handle_ddjvu_messages (ddjvu_context_t * ctx, int wait)
  {
    const ddjvu_message_t *msg;
    if (wait)
      ddjvu_message_wait (ctx);
    while ((msg = ddjvu_message_peek (ctx)))
      {
	debug ("tag: %d", msg->m_any.tag);
	switch (msg->m_any.tag)
	  {
	  case DDJVU_ERROR:
	    break;
	  case DDJVU_INFO:
	    break;
	  case DDJVU_PAGEINFO:
	    break;
	  default:
	    break;
	  }
	ddjvu_message_pop (ctx);
      }
  }
#endif

  ApvlvDJVU::ApvlvDJVU (const char *filename, bool check):ApvlvFile (filename,
								     check)
  {
    mContext = ddjvu_context_create ("apvlv");
    if (mContext)
      {
	mDoc = ddjvu_document_create_by_filename (mContext, filename, FALSE);
      }

    if (mDoc != NULL)
      {
	if (ddjvu_document_get_type (mDoc) != DDJVU_DOCTYPE_UNKNOWN)
	  {
	    debug ("djvu type: %d", ddjvu_document_get_type (mDoc));
	  }
	else
	  {
	    ddjvu_document_release (mDoc);
	    mDoc = NULL;
	    ddjvu_context_release (mContext);
	    mContext = NULL;
	    throw std::bad_alloc ();
	  }
      }
    else
      {
	ddjvu_context_release (mContext);
	mContext = NULL;
	throw std::bad_alloc ();
      }
  }

  ApvlvDJVU::~ApvlvDJVU ()
  {
    if (mContext)
      {
	ddjvu_context_release (mContext);
      }

    if (mDoc)
      {
	ddjvu_document_release (mDoc);
      }
  }

  bool ApvlvDJVU::pagesize (int pn, int rot, double *x, double *y)
  {
    ddjvu_status_t t;
    ddjvu_pageinfo_t info[1];
    while ((t = ddjvu_document_get_pageinfo (mDoc, 0, info)) < DDJVU_JOB_OK)
      {
	handle_ddjvu_messages (mContext, true);
      }

    if (t == DDJVU_JOB_OK)
      {
	*x = info->width;
	*y = info->height;
	debug ("djvu page 1: %f-%f", *x, *y);
      }
    return true;
  }

  int ApvlvDJVU::pagesum ()
  {
    return mDoc ? ddjvu_document_get_pagenum (mDoc) : 0;
  }

  bool ApvlvDJVU::render (int pn, int ix, int iy, double zm, int rot,
			  GdkPixbuf * pix, char *buffer)
  {
#ifdef HAVE_LIBDJVU
    ddjvu_page_t *tpage;

    if ((tpage = ddjvu_page_create_by_pageno (mDoc, pn)) == NULL)
      {
	debug ("no this page: %d", pn);
	return false;
      }

    ddjvu_rect_t prect[1] = { {0, 0, ix, iy}
    };
    ddjvu_rect_t rrect[1] = { {0, 0, ix, iy}
    };
    debug ("for fender: ");
    ddjvu_format_t *format =
      ddjvu_format_create (DDJVU_FORMAT_RGB24, 0, NULL);
    ddjvu_format_set_row_order (format, TRUE);
    while (ddjvu_page_render
	   (tpage, DDJVU_RENDER_COLOR, prect, rrect, format, 3 * ix,
	    (char *) buffer) == FALSE)
      {
	debug ("fender failed");
      }

    /*  
       if (ac->mLinkMappings)
       {
       poppler_page_free_link_mapping (ac->mLinkMappings);
       }
       ac->mLinkMappings =
       g_list_reverse (poppler_page_get_link_mapping (tpage));
       debug ("has mLinkMappings: %p", ac->mLinkMappings); */

    debug ("for fender: ");
    return true;
#else
    return false;
#endif
  }

  ApvlvPoses *ApvlvDJVU::pagesearch (int pn, bool reverse)
  {
    return NULL;
  }

  ApvlvLinks *ApvlvDJVU::getlinks (int pn)
  {
    return NULL;
  }

  bool ApvlvDJVU::pagetext (int pn, char **out)
  {
    return false;
  }

  ApvlvFileIndex *ApvlvDJVU::new_index ()
  {
    return NULL;
  }

  void ApvlvDJVU::free_index (ApvlvFileIndex * index)
  {
  }

}

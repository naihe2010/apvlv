/*
 * This file is part of the apvlv package
 * Copyright (C) <2010>  <Alf>
 *
 * Contact: Alf <naihe2010@126.com>
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
/* @CFILE ApvlvPdf.cc
 *
 *  Author: Alf <naihe2010@126.com>
 */
/* @date Created: 2011/09/16 13:50:18 Alf*/

#include "ApvlvView.h"
#include "ApvlvPdf.h"

#ifndef POPPLER_WITH_GDK
#include <goo/gtypes.h>

static void
copy_cairo_surface_to_pixbuf (cairo_surface_t *surface,
			      GdkPixbuf       *pixbuf)
{
  int cairo_width, cairo_height, cairo_rowstride;
  unsigned char *pixbuf_data, *dst, *cairo_data;
  int pixbuf_rowstride, pixbuf_n_channels;
  unsigned int *src;
  int x, y;

  cairo_width = cairo_image_surface_get_width (surface);
  cairo_height = cairo_image_surface_get_height (surface);
  cairo_rowstride = cairo_image_surface_get_stride (surface);
  cairo_data = cairo_image_surface_get_data (surface);

  pixbuf_data = gdk_pixbuf_get_pixels (pixbuf);
  pixbuf_rowstride = gdk_pixbuf_get_rowstride (pixbuf);
  pixbuf_n_channels = gdk_pixbuf_get_n_channels (pixbuf);

  if (cairo_width > gdk_pixbuf_get_width (pixbuf))
    cairo_width = gdk_pixbuf_get_width (pixbuf);
  if (cairo_height > gdk_pixbuf_get_height (pixbuf))
    cairo_height = gdk_pixbuf_get_height (pixbuf);
  for (y = 0; y < cairo_height; y++)
    {
      src = (unsigned int *) (cairo_data + y * cairo_rowstride);
      dst = pixbuf_data + y * pixbuf_rowstride;
      for (x = 0; x < cairo_width; x++)
	{
	  dst[0] = (*src >> 16) & 0xff;
	  dst[1] = (*src >> 8) & 0xff;
	  dst[2] = (*src >> 0) & 0xff;
	  if (pixbuf_n_channels == 4)
	    dst[3] = (*src >> 24) & 0xff;
	  dst += pixbuf_n_channels;
	  src++;
	}
    }
}

static void
_poppler_page_render_to_pixbuf (PopplerPage *page,
				int src_x, int src_y,
				int src_width, int src_height,
				double scale,
				int rotation,
				gboolean printing,
				GdkPixbuf *pixbuf)
{
  cairo_t *cr;
  cairo_surface_t *surface;

  surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32,
					src_width, src_height);
  cr = cairo_create (surface);
  cairo_save (cr);
  switch (rotation) {
  case 90:
    cairo_translate (cr, src_x + src_width, -src_y);
    break;
  case 180:
    cairo_translate (cr, src_x + src_width, src_y + src_height);
    break;
  case 270:
    cairo_translate (cr, -src_x, src_y + src_height);
    break;
  default:
    cairo_translate (cr, -src_x, -src_y);
  }

  if (scale != 1.0)
    cairo_scale (cr, scale, scale);

  if (rotation != 0)
    cairo_rotate (cr, rotation * G_PI / 180.0);

  if (printing)
    poppler_page_render_for_printing (page, cr);
  else
    poppler_page_render (page, cr);
  cairo_restore (cr);

  cairo_set_operator (cr, CAIRO_OPERATOR_DEST_OVER);
  cairo_set_source_rgb (cr, 1., 1., 1.);
  cairo_paint (cr);

  cairo_destroy (cr);

  copy_cairo_surface_to_pixbuf (surface, pixbuf);
  cairo_surface_destroy (surface);
}

/**
 * poppler_page_render_to_pixbuf:
 * @page: the page to render from
 * @src_x: x coordinate of upper left corner
 * @src_y: y coordinate of upper left corner
 * @src_width: width of rectangle to render
 * @src_height: height of rectangle to render
 * @scale: scale specified as pixels per point
 * @rotation: rotate the document by the specified degree
 * @pixbuf: pixbuf to render into
 *
 * First scale the document to match the specified pixels per point,
 * then render the rectangle given by the upper left corner at
 * (src_x, src_y) and src_width and src_height.
 * This function is for rendering a page that will be displayed.
 * If you want to render a page that will be printed use
 * poppler_page_render_to_pixbuf_for_printing() instead
 *
 * Deprecated: 0.16
 **/
void
poppler_page_render_to_pixbuf (PopplerPage *page,
			       int src_x, int src_y,
			       int src_width, int src_height,
			       double scale,
			       int rotation,
			       GdkPixbuf *pixbuf)
{
  g_return_if_fail (POPPLER_IS_PAGE (page));
  g_return_if_fail (scale > 0.0);
  g_return_if_fail (pixbuf != NULL);

  _poppler_page_render_to_pixbuf (page, src_x, src_y,
				  src_width, src_height,
				  scale, rotation,
				  FALSE,
				  pixbuf);
}
#endif

#include <fstream>
#include <sys/stat.h>

namespace apvlv
{
  ApvlvPDF::ApvlvPDF (const char *filename, bool check):ApvlvFile (filename,
								   check)
  {
    gchar *wfilename;

    if (filename == NULL
	|| *filename == '\0'
	|| g_file_test (filename, G_FILE_TEST_IS_REGULAR) == FALSE
	|| (wfilename =
            g_locale_from_utf8 (filename, -1, NULL, NULL, NULL)) == NULL)
      {
	gView->errormessage ("filename error: %s",
			     filename ? filename : "No name");
	throw std::bad_alloc ();
      }

    size_t filelen;
    struct stat sbuf;
    int rt = stat (wfilename, &sbuf);
    if (rt < 0)
      {
	gView->errormessage ("Can't stat the PDF file: %s.", filename);
	throw std::bad_alloc ();
      }
    filelen = sbuf.st_size;

    if (mRawdata != NULL && mRawdataSize < filelen)
      {
	delete[]mRawdata;
	mRawdata = NULL;
      }

    if (mRawdata == NULL)
      {
	mRawdata = new char[filelen];
	mRawdataSize = filelen;
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
	GtkWidget *dia = gtk_message_dialog_new (NULL,
						 GTK_DIALOG_DESTROY_WITH_PARENT,
						 GTK_MESSAGE_QUESTION,
						 GTK_BUTTONS_OK_CANCEL,
						 "%s", error->message);
	g_error_free (error);


	GtkWidget *entry = gtk_entry_new ();
	gtk_entry_set_visibility (GTK_ENTRY (entry), FALSE);
	gtk_entry_set_invisible_char (GTK_ENTRY (entry), '*');
	gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dia))), entry, TRUE,
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
	g_object_unref (mDoc);
      }
  }

  bool ApvlvPDF::writefile (const char *filename)
  {
    debug ("write %p to %s", this, filename);
    gchar *path = absolutepath (filename);
    if (path == NULL)
      {
	debug ("filename error: %s", filename);
	return false;
      }

    GError *error = NULL;
    gchar *uri = g_filename_to_uri (path, NULL, &error);
    g_free (path);
    if (uri == NULL && error)
      {
	debug ("%d: %s", error->code, error->message);
	return false;
      }

    if (mDoc && uri != NULL)
      {
	gboolean ret = poppler_document_save (mDoc, uri, NULL);
	debug ("write pdf: %p to %s, return %d", mDoc, uri, ret);
	g_free (uri);
	return ret == TRUE ? true : false;
      }
    return false;
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
	g_object_unref(page);
	return true;
      }
    return false;
  }

  int ApvlvPDF::pagesum ()
  {
    return mDoc ? poppler_document_get_n_pages (mDoc) : 0;
  }

  ApvlvPoses *ApvlvPDF::pagesearch (int pn, const char *str, bool reverse)
  {
    PopplerPage *page = poppler_document_get_page (mDoc, pn);
    if (page == NULL)
      {
	return NULL;
      }

    //    debug ("search %s", str);

    GList *list = poppler_page_find_text (page, str);
    if (list == NULL)
      {
	g_object_unref(page);
	return NULL;
      }

    if (reverse)
      {
	list = g_list_reverse (list);
      }

    ApvlvPoses *poses = new ApvlvPoses;
    for (GList * tmp = list; tmp != NULL; tmp = g_list_next (tmp))
      {
	PopplerRectangle *rect = (PopplerRectangle *) tmp->data;
	//      debug ("results: %f-%f,%f-%f", rect->x1, rect->x2, rect->y1,
	//             rect->y2);
	ApvlvPos pos = { rect->x1, rect->x2, rect->y1, rect->y2 };
	poses->push_back (pos);

	poppler_rectangle_free(rect);
      }

    g_list_free(list);
    g_object_unref(page);

    return poses;
  }

  bool ApvlvPDF::pagetext (int pn, int x1, int y1, int x2, int y2, char **out)
  {
    PopplerPage *page = poppler_document_get_page (mDoc, pn);
#if POPPLER_CHECK_VERSION(0, 15, 1)
    PopplerRectangle rect = {
      static_cast <gdouble> (x1),
      static_cast <gdouble> (y2),
      static_cast <gdouble> (x2),
      static_cast <gdouble> (y1)
    };
    *out = poppler_page_get_selected_text (page, POPPLER_SELECTION_WORD, &rect);
#else
    PopplerRectangle rect = { x1, y1, x2, y2 };
    *out = poppler_page_get_text (page, POPPLER_SELECTION_WORD, &rect);
#endif
    g_object_unref(page);
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
    g_object_unref(tpage);
    return true;
  }

  bool ApvlvPDF::pageselectsearch (int pn, int ix, int iy,
				   double zm, int rot, GdkPixbuf * pix,
				   char *buffer, int sel, ApvlvPoses * poses)
  {
    ApvlvPos rect = (*poses)[sel];

    // Caculate the correct position
    //debug ("pagex: %f, pagey: %f, x1: %f, y1: %f, x2: %f, y2: %f", pagex, pagey, rect->x1, rect->y1, rect->x2, rect->y2);
    gint x1 = MAX (rect.x1 * zm + 0.5, 0);
    gint x2 = MAX (rect.x2 * zm - 0.5, 1);
    gint y1 = MAX ((iy - rect.y2 * zm) + 0.5, 0);
    gint y2 = MAX ((iy - rect.y1 * zm) - 0.5, 1);
    //debug ("x1: %d, y1: %d, x2: %d, y2: %d", x1, y1, x2, y2);

    // heightlight the selection
    for (gint y = y1; y < y2; y++)
      {
	for (gint x = x1; x < x2; x++)
	  {
	    gint p = (gint) (y * ix * 3 + (x * 3));
	    buffer[p + 0] = 0xff - buffer[p + 0];
	    buffer[p + 1] = 0xff - buffer[p + 0];
	    buffer[p + 2] = 0xff - buffer[p + 0];
	  }
      }

    // change the back color of the selection
    for (ApvlvPoses::const_iterator itr = poses->begin ();
	 itr != poses->end (); itr++)
      {
	// Caculate the correct position
	x1 = (gint) (itr->x1 * zm);
	x2 = (gint) (itr->x2 * zm);
	y1 = (gint) (iy - itr->y2 * zm);
	y2 = (gint) (iy - itr->y1 * zm);

	for (gint y = y1; y < y2; y++)
	  {
	    for (gint x = x1; x < x2; x++)
	      {
		gint p = (gint) (y * 3 * ix + (x * 3));
		buffer[p + 0] = 0xff - buffer[p + 0];
		buffer[p + 1] = 0xff - buffer[p + 0];
		buffer[p + 2] = 0xff - buffer[p + 0];
	      }
	  }
      }

    return true;
  }

  ApvlvLinks *ApvlvPDF::getlinks (int pn)
  {
    PopplerPage *page = poppler_document_get_page (mDoc, pn);
    GList *list = poppler_page_get_link_mapping (page);
    if (list == NULL)
      {
	g_object_unref(page);
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

    g_list_free(list);
    g_object_unref(page);
    return links;
  }

  ApvlvFileIndex *ApvlvPDF::new_index ()
  {
    if (mIndex != NULL)
      {
	debug ("file %p has index: %p, return", this, mIndex);
	return mIndex;
      }

    PopplerIndexIter *itr = poppler_index_iter_new (mDoc);
    if (itr == NULL)
      {
	debug ("no index.");
	return NULL;
      }

    mIndex = new ApvlvFileIndex;
    walk_poppler_index_iter (mIndex, itr);
    poppler_index_iter_free (itr);

    return mIndex;
  }

  void ApvlvPDF::free_index (ApvlvFileIndex * index)
  {
    delete index;
  }

  bool ApvlvPDF::walk_poppler_index_iter (ApvlvFileIndex * titr,
					  PopplerIndexIter * iter)
  {
    bool has = false;
    do
      {
	has = false;
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
		    index->page = pagd->dest->page_num - 1;
		  }

		if (index != NULL)
		  {
		    has = true;
		    index->title = pagd->title;
		    titr->children.push_back (*index);
		    delete index;
		    index = &(titr->children[titr->children.size () - 1]);
		    debug ("titr: %p, index: %p", titr, index);
		  }
	      }
	    poppler_action_free (act);
	  }

	PopplerIndexIter *child = poppler_index_iter_get_child (iter);
	if (child)
	  {
	    bool chas = walk_poppler_index_iter (has ? index : titr, child);
	    has = has ? has : chas;
	    poppler_index_iter_free (child);
	  }
      }
    while (poppler_index_iter_next (iter));
    return has;
  }

  bool ApvlvPDF::pageprint (int pn, cairo_t * cr)
  {
#ifdef WIN32
    return false;
#else
    PopplerPage *page = poppler_document_get_page (mDoc, pn);
    if (page != NULL)
      {
	poppler_page_render_for_printing (page, cr);
	g_object_unref(page);
	return true;
      }
    else
      {
	return false;
      }
#endif
  }
}

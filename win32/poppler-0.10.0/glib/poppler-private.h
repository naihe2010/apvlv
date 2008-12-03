#ifndef __POPPLER_PRIVATE_H__
#define __POPPLER_PRIVATE_H__

#include <config.h>
#include <PDFDoc.h>
#include <PSOutputDev.h>
#include <Link.h>
#include <Form.h>
#include <Gfx.h>
#include <FontInfo.h>
#include <TextOutputDev.h>
#include <Catalog.h>

#if defined (HAVE_CAIRO)
#include <CairoOutputDev.h>
#elif defined (HAVE_SPLASH)
#include <SplashOutputDev.h>
#endif

struct _PopplerDocument
{
  GObject parent_instance;
  PDFDoc *doc;

#if defined (HAVE_CAIRO)
  CairoOutputDev *output_dev;
#elif defined (HAVE_SPLASH)
  SplashOutputDev *output_dev;
#endif
};

struct _PopplerPSFile
{
  GObject parent_instance;

  PopplerDocument *document;
  PSOutputDev *out;
  char *filename;
  int first_page;
  int last_page;
  double paper_width;
  double paper_height;
  gboolean duplex;
};

struct _PopplerFontInfo
{
  GObject parent_instance;
  PopplerDocument *document;
  FontInfoScanner *scanner;
};

struct _PopplerPage
{
  GObject parent_instance;
  PopplerDocument *document;
  Page *page;
  int index;
  TextOutputDev *text_dev;
  Gfx *gfx;
  Annots *annots;
};

struct _PopplerFormField
{
  GObject parent_instance;
  PopplerDocument *document;
  FormWidget *widget;
};

PopplerPage   *_poppler_page_new   (PopplerDocument *document,
				    Page            *page,
				    int              index);
PopplerAction *_poppler_action_new (PopplerDocument *document,
				    LinkAction      *link,
				    const gchar     *title);
PopplerDest   *_poppler_dest_new_goto (PopplerDocument *document,
				       LinkDest        *link_dest);
PopplerFormField *_poppler_form_field_new (PopplerDocument *document,
					   FormWidget      *field);
PopplerAttachment *_poppler_attachment_new (PopplerDocument *document,
					    EmbFile         *file);
PopplerAnnot      *_poppler_annot_new           (Annot *annot);
PopplerAnnot      *_poppler_annot_text_new      (Annot *annot);
PopplerAnnot      *_poppler_annot_free_text_new (Annot *annot);

char *_poppler_goo_string_to_utf8(GooString *s);
gboolean _poppler_convert_pdf_date_to_gtime (GooString *date,
					     GTime     *gdate);
#endif

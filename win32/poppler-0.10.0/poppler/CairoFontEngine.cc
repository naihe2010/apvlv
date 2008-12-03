//========================================================================
//
// CairoFontEngine.cc
//
// Copyright 2003 Glyph & Cog, LLC
// Copyright 2004 Red Hat, Inc
//
//========================================================================

//========================================================================
//
// Modified under the Poppler project - http://poppler.freedesktop.org
//
// All changes made under the Poppler project to this file are licensed
// under GPL version 2 or later
//
// Copyright (C) 2005-2007 Jeff Muizelaar <jeff@infidigm.net>
// Copyright (C) 2005, 2006 Kristian Høgsberg <krh@redhat.com>
// Copyright (C) 2005 Martin Kretzschmar <martink@gnome.org>
// Copyright (C) 2005 Albert Astals Cid <aacid@kde.org>
// Copyright (C) 2006, 2007 Carlos Garcia Campos <carlosgc@gnome.org>
// Copyright (C) 2007 Koji Otani <sho@bbr.jp>
// Copyright (C) 2008 Chris Wilson <chris@chris-wilson.co.uk>
//
// To see a description of the changes please see the Changelog file that
// came with your tarball or type make ChangeLog if you are building from git
//
//========================================================================

#include <config.h>

#include "config.h"
#include <string.h>
#include "CairoFontEngine.h"
#include "CharCodeToUnicode.h"
#include "GlobalParams.h"
#include <fofi/FoFiTrueType.h>
#include <fofi/FoFiType1C.h>
#include "goo/gfile.h"
#include "Error.h"

#if HAVE_FCNTL_H && HAVE_SYS_MMAN_H && HAVE_SYS_STAT_H
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#define CAN_CHECK_OPEN_FACES 1
#endif

#ifdef USE_GCC_PRAGMAS
#pragma implementation
#endif

static void fileWrite(void *stream, char *data, int len) {
  fwrite(data, 1, len, (FILE *)stream);
}

//------------------------------------------------------------------------
// CairoFont
//------------------------------------------------------------------------

static cairo_user_data_key_t _ft_cairo_key;

static void
_ft_done_face_uncached (void *closure)
{
    FT_Face face = (FT_Face) closure;
    FT_Done_Face (face);
}

static GBool
_ft_new_face_uncached (FT_Library lib,
		       const char *filename,
		       FT_Face *face_out,
		       cairo_font_face_t **font_face_out)
{
  FT_Face face;
  cairo_font_face_t *font_face;

  if (FT_New_Face (lib, filename, 0, &face))
    return gFalse;

  font_face = cairo_ft_font_face_create_for_ft_face (face,
							  FT_LOAD_NO_HINTING |
							  FT_LOAD_NO_BITMAP);
  if (cairo_font_face_set_user_data (font_face,
				     &_ft_cairo_key,
				     face,
				     _ft_done_face_uncached))
  {
    _ft_done_face_uncached (face);
    cairo_font_face_destroy (font_face);
    return gFalse;
  }

  *face_out = face;
  *font_face_out = font_face;
  return gTrue;
}

#if CAN_CHECK_OPEN_FACES
static struct _ft_face_data {
  struct _ft_face_data *prev, *next, **head;

  int fd;
  unsigned long hash;
  size_t size;
  unsigned char *bytes;

  FT_Library lib;
  FT_Face face;
  cairo_font_face_t *font_face;
} *_ft_open_faces;

static unsigned long
_djb_hash (const unsigned char *bytes, size_t len)
{
  unsigned long hash = 5381;
  while (len--) {
    unsigned char c = *bytes++;
    hash *= 33;
    hash ^= c;
  }
  return hash;
}

static GBool
_ft_face_data_equal (struct _ft_face_data *a, struct _ft_face_data *b)
{
  if (a->lib != b->lib)
    return gFalse;
  if (a->size != b->size)
    return gFalse;
  if (a->hash != b->hash)
    return gFalse;

  return memcmp (a->bytes, b->bytes, a->size) == 0;
}

static void
_ft_done_face (void *closure)
{
  struct _ft_face_data *data = (struct _ft_face_data *) closure;

  if (data->next)
    data->next->prev = data->prev;
  if (data->prev)
    data->prev->next = data->next;
  else
    _ft_open_faces = data->next;

  munmap (data->bytes, data->size);
  close (data->fd);

  FT_Done_Face (data->face);
  gfree (data);
}

static GBool
_ft_new_face (FT_Library lib,
	      const char *filename,
	      FT_Face *face_out,
	      cairo_font_face_t **font_face_out)
{
  struct _ft_face_data *l;
  struct stat st;
  struct _ft_face_data tmpl;

  /* if we fail to mmap the file, just pass it to FreeType instead */
  tmpl.fd = open (filename, O_RDONLY);
  if (tmpl.fd == -1)
    return _ft_new_face_uncached (lib, filename, face_out, font_face_out);

  if (fstat (tmpl.fd, &st) == -1) {
    close (tmpl.fd);
    return _ft_new_face_uncached (lib, filename, face_out, font_face_out);
  }

  tmpl.bytes = (unsigned char *) mmap (NULL, st.st_size,
				       PROT_READ, MAP_PRIVATE,
				       tmpl.fd, 0);
  if (tmpl.bytes == MAP_FAILED) {
    close (tmpl.fd);
    return _ft_new_face_uncached (lib, filename, face_out, font_face_out);
  }

  /* check to see if this is a duplicate of any of the currently open fonts */
  tmpl.lib = lib;
  tmpl.size = st.st_size;
  tmpl.hash = _djb_hash (tmpl.bytes, tmpl.size);

  for (l = _ft_open_faces; l; l = l->next) {
    if (_ft_face_data_equal (l, &tmpl)) {
      munmap (tmpl.bytes, tmpl.size);
      close (tmpl.fd);
      *face_out = l->face;
      *font_face_out = cairo_font_face_reference (l->font_face);
      return gTrue;
    }
  }

  /* not a dup, open and insert into list */
  if (FT_New_Face (lib, filename, 0, &tmpl.face)) {
    munmap (tmpl.bytes, tmpl.size);
    close (tmpl.fd);
    return gFalse;
  }

  l = (struct _ft_face_data *) gmallocn (1, sizeof (struct _ft_face_data));
  *l = tmpl;
  l->prev = NULL;
  l->next = _ft_open_faces;
  if (_ft_open_faces)
    _ft_open_faces->prev = l;
  _ft_open_faces = l;

  l->font_face = cairo_ft_font_face_create_for_ft_face (tmpl.face,
							  FT_LOAD_NO_HINTING |
							  FT_LOAD_NO_BITMAP);
  if (cairo_font_face_set_user_data (l->font_face,
				     &_ft_cairo_key,
				     l,
				     _ft_done_face))
  {
    cairo_font_face_destroy (l->font_face);
    _ft_done_face (l);
    return NULL;
  }

  *face_out = l->face;
  *font_face_out = l->font_face;
  return gTrue;
}
#else
#define _ft_new_face _ft_new_face_uncached
#endif

CairoFont *CairoFont::create(GfxFont *gfxFont, XRef *xref, FT_Library lib, GBool useCIDs) {
  Ref embRef;
  Object refObj, strObj;
  GooString *tmpFileName, *fileName,*tmpFileName2;
  DisplayFontParam *dfp;
  FILE *tmpFile;
  int c, i, n;
  GfxFontType fontType;
  char **enc;
  char *name;
  FoFiTrueType *ff;
  FoFiType1C *ff1c;
  Ref ref;
  FT_Face face;
  cairo_font_face_t *font_face;

  Gushort *codeToGID;
  int codeToGIDLen;
  
  dfp = NULL;
  codeToGID = NULL;
  codeToGIDLen = 0;

  GBool substitute = gFalse;
  
  ref = *gfxFont->getID();
  fontType = gfxFont->getType();

  tmpFileName = NULL;

  if (gfxFont->getEmbeddedFontID(&embRef)) {
    if (!openTempFile(&tmpFileName, &tmpFile, "wb", NULL)) {
      error(-1, "Couldn't create temporary font file");
      goto err2;
    }
    
    refObj.initRef(embRef.num, embRef.gen);
    refObj.fetch(xref, &strObj);
    refObj.free();
    if (!strObj.isStream()) {
      error(-1, "Embedded font object is wrong type");
      strObj.free();
      fclose(tmpFile);
      goto err2;
    }
    strObj.streamReset();
    while ((c = strObj.streamGetChar()) != EOF) {
      fputc(c, tmpFile);
    }
    strObj.streamClose();
    strObj.free();
    fclose(tmpFile);
    fileName = tmpFileName;
    
  } else if (!(fileName = gfxFont->getExtFontFile())) {
    // look for a display font mapping or a substitute font
    dfp = NULL;
    if (gfxFont->getName()) {
      dfp = globalParams->getDisplayFont(gfxFont);
    }
    if (!dfp) {
      error(-1, "Couldn't find a font for '%s'",
	    gfxFont->getName() ? gfxFont->getName()->getCString()
	    : "(unnamed)");
      goto err2;
    }
    switch (dfp->kind) {
    case displayFontT1:
      fileName = dfp->t1.fileName;
      fontType = gfxFont->isCIDFont() ? fontCIDType0 : fontType1;
      break;
    case displayFontTT:
      fileName = dfp->tt.fileName;
      fontType = gfxFont->isCIDFont() ? fontCIDType2 : fontTrueType;
      break;
    }
    substitute = gTrue;
  }

  switch (fontType) {
  case fontType1:
  case fontType1C:
    if (! _ft_new_face (lib, fileName->getCString(), &face, &font_face)) {
      error(-1, "could not create type1 face");
      goto err2;
    }
    
    enc = ((Gfx8BitFont *)gfxFont)->getEncoding();
    
    codeToGID = (Gushort *)gmallocn(256, sizeof(int));
    codeToGIDLen = 256;
    for (i = 0; i < 256; ++i) {
      codeToGID[i] = 0;
      if ((name = enc[i])) {
	codeToGID[i] = (Gushort)FT_Get_Name_Index(face, name);
      }
    }
    break;
    
  case fontCIDType2:
    codeToGID = NULL;
    n = 0;
    if (((GfxCIDFont *)gfxFont)->getCIDToGID()) {
      n = ((GfxCIDFont *)gfxFont)->getCIDToGIDLen();
      if (n) {
	codeToGID = (Gushort *)gmallocn(n, sizeof(Gushort));
	memcpy(codeToGID, ((GfxCIDFont *)gfxFont)->getCIDToGID(),
		n * sizeof(Gushort));
      }
    } else {
      ff = FoFiTrueType::load(fileName->getCString());
      if (! ff)
	goto err2;
      codeToGID = ((GfxCIDFont *)gfxFont)->getCodeToGIDMap(ff, &n);
      delete ff;
    }
    codeToGIDLen = n;
    /* Fall through */
  case fontTrueType:
    if (!(ff = FoFiTrueType::load(fileName->getCString()))) {
      error(-1, "failed to load truetype font\n");
      goto err2;
    }
    /* This might be set already for the CIDType2 case */
    if (fontType == fontTrueType) {
      codeToGID = ((Gfx8BitFont *)gfxFont)->getCodeToGIDMap(ff);
      codeToGIDLen = 256;
    }
    if (!openTempFile(&tmpFileName2, &tmpFile, "wb", NULL)) {
      delete ff;
      error(-1, "failed to open truetype tempfile\n");
      goto err2;
    }
    ff->writeTTF(&fileWrite, tmpFile);
    fclose(tmpFile);
    delete ff;

    if (! _ft_new_face (lib, tmpFileName2->getCString(), &face, &font_face)) {
      error(-1, "could not create truetype face\n");
      goto err2;
    }
    unlink (tmpFileName2->getCString());
    delete tmpFileName2;
    break;
    
  case fontCIDType0:
  case fontCIDType0C:

    codeToGID = NULL;
    codeToGIDLen = 0;

    if (!useCIDs)
    {
      if ((ff1c = FoFiType1C::load(fileName->getCString()))) {
        codeToGID = ff1c->getCIDToGIDMap(&codeToGIDLen);
        delete ff1c;
      }
    }

    if (! _ft_new_face (lib, fileName->getCString(), &face, &font_face)) {
      gfree(codeToGID);
      codeToGID = NULL;
      error(-1, "could not create cid face\n");
      goto err2;
    }
    break;
    
  default:
    printf ("font type not handled\n");
    goto err2;
    break;
  }

  // delete the (temporary) font file -- with Unix hard link
  // semantics, this will remove the last link; otherwise it will
  // return an error, leaving the file to be deleted later
  if (fileName == tmpFileName) {
    unlink (fileName->getCString());
    delete tmpFileName;
  }

  return new CairoFont(ref,
		       font_face, face,
		       codeToGID, codeToGIDLen,
		       substitute);

 err2:
  /* hmm? */
  printf ("some font thing failed\n");
  return NULL;
}

CairoFont::CairoFont(Ref ref, cairo_font_face_t *cairo_font_face, FT_Face face,
    Gushort *codeToGID, int codeToGIDLen, GBool substitute) : ref(ref), cairo_font_face(cairo_font_face),
					    face(face), codeToGID(codeToGID),
					    codeToGIDLen(codeToGIDLen), substitute(substitute) { }

CairoFont::~CairoFont() {
  cairo_font_face_destroy (cairo_font_face);
  gfree(codeToGID);
}

GBool
CairoFont::matches(Ref &other) {
  return (other.num == ref.num && other.gen == ref.gen);
}

cairo_font_face_t *
CairoFont::getFontFace(void) {
  return cairo_font_face;
}

unsigned long
CairoFont::getGlyph(CharCode code,
		    Unicode *u, int uLen) {
  FT_UInt gid;

  if (codeToGID && code < codeToGIDLen) {
    gid = (FT_UInt)codeToGID[code];
  } else {
    gid = (FT_UInt)code;
  }
  return gid;
}

double
CairoFont::getSubstitutionCorrection(GfxFont *gfxFont)
{
  double w1, w2,w3;
  CharCode code;
  char *name;

  // for substituted fonts: adjust the font matrix -- compare the
  // width of 'm' in the original font and the substituted font
  if (isSubstitute() && !gfxFont->isCIDFont()) {
    for (code = 0; code < 256; ++code) {
      if ((name = ((Gfx8BitFont *)gfxFont)->getCharName(code)) &&
	  name[0] == 'm' && name[1] == '\0') {
	break;
      }
    }
    if (code < 256) {
      w1 = ((Gfx8BitFont *)gfxFont)->getWidth(code);
      {
	cairo_matrix_t m;
	cairo_matrix_init_identity(&m);
	cairo_font_options_t *options = cairo_font_options_create();
	cairo_font_options_set_hint_style(options, CAIRO_HINT_STYLE_NONE);
	cairo_font_options_set_hint_metrics(options, CAIRO_HINT_METRICS_OFF);
	cairo_scaled_font_t *scaled_font = cairo_scaled_font_create(cairo_font_face, &m, &m, options);

	cairo_text_extents_t extents;
	cairo_scaled_font_text_extents(scaled_font, "m", &extents);

	cairo_scaled_font_destroy(scaled_font);
	cairo_font_options_destroy(options);
	w3 = extents.width;
	w2 = extents.x_advance;
      }
      if (!gfxFont->isSymbolic()) {
	// if real font is substantially narrower than substituted
	// font, reduce the font size accordingly
	if (w1 > 0.01 && w1 < 0.9 * w2) {
	  w1 /= w2;
	  return w1;
	}
      }
    }
  }
  return 1.0;
}

//------------------------------------------------------------------------
// CairoFontEngine
//------------------------------------------------------------------------

CairoFontEngine::CairoFontEngine(FT_Library libA) {
  int i;

  lib = libA;
  for (i = 0; i < cairoFontCacheSize; ++i) {
    fontCache[i] = NULL;
  }
  
  FT_Int major, minor, patch;
  // as of FT 2.1.8, CID fonts are indexed by CID instead of GID
  FT_Library_Version(lib, &major, &minor, &patch);
  useCIDs = major > 2 ||
            (major == 2 && (minor > 1 || (minor == 1 && patch > 7)));
}

CairoFontEngine::~CairoFontEngine() {
  int i;
  
  for (i = 0; i < cairoFontCacheSize; ++i) {
    if (fontCache[i])
      delete fontCache[i];
  }
}

CairoFont *
CairoFontEngine::getFont(GfxFont *gfxFont, XRef *xref) {
  int i, j;
  Ref ref;
  CairoFont *font;
  GfxFontType fontType;
  
  fontType = gfxFont->getType();
  if (fontType == fontType3) {
    /* Need to figure this out later */
    //    return NULL;
  }

  ref = *gfxFont->getID();

  for (i = 0; i < cairoFontCacheSize; ++i) {
    font = fontCache[i];
    if (font && font->matches(ref)) {
      for (j = i; j > 0; --j) {
	fontCache[j] = fontCache[j-1];
      }
      fontCache[0] = font;
      return font;
    }
  }
  
  font = CairoFont::create (gfxFont, xref, lib, useCIDs);
  //XXX: if font is null should we still insert it into the cache?
  if (fontCache[cairoFontCacheSize - 1]) {
    delete fontCache[cairoFontCacheSize - 1];
  }
  for (j = cairoFontCacheSize - 1; j > 0; --j) {
    fontCache[j] = fontCache[j-1];
  }
  fontCache[0] = font;
  return font;
}


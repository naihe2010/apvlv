//========================================================================
//
// CairoFontEngine.h
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
// Copyright (C) 2005, 2006 Kristian Høgsberg <krh@redhat.com>
// Copyright (C) 2005 Albert Astals Cid <aacid@kde.org>
// Copyright (C) 2006, 2007 Jeff Muizelaar <jeff@infidigm.net>
// Copyright (C) 2006 Carlos Garcia Campos <carlosgc@gnome.org>
//
// To see a description of the changes please see the Changelog file that
// came with your tarball or type make ChangeLog if you are building from git
//
//========================================================================

#ifndef CAIROFONTENGINE_H
#define CAIROFONTENGINE_H

#ifdef USE_GCC_PRAGMAS
#pragma interface
#endif

#include "goo/gtypes.h"
#include <cairo-ft.h>

#include "GfxFont.h"

class CairoFont {
public:
  static CairoFont *create(GfxFont *gfxFont, XRef *xref, FT_Library lib, GBool useCIDs);
  ~CairoFont();

  GBool matches(Ref &other);
  cairo_font_face_t *getFontFace(void);
  unsigned long getGlyph(CharCode code, Unicode *u, int uLen);
  double getSubstitutionCorrection(GfxFont *gfxFont);

  GBool isSubstitute() { return substitute; }
private:
  CairoFont(Ref ref, cairo_font_face_t *cairo_font_face, FT_Face face,
      Gushort *codeToGID, int codeToGIDLen, GBool substitute);
  Ref ref;
  cairo_font_face_t *cairo_font_face;
  FT_Face face;

  Gushort *codeToGID;
  int codeToGIDLen;

  GBool substitute;
};

//------------------------------------------------------------------------

#define cairoFontCacheSize 64

//------------------------------------------------------------------------
// CairoFontEngine
//------------------------------------------------------------------------

class CairoFontEngine {
public:

  // Create a font engine.
  CairoFontEngine(FT_Library libA);
  ~CairoFontEngine();

  CairoFont *getFont(GfxFont *gfxFont, XRef *xref);

private:
  CairoFont *fontCache[cairoFontCacheSize];
  FT_Library lib;
  GBool useCIDs;
};

#endif

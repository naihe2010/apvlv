//========================================================================
//
// FileSpec.cc
//
// All changes made under the Poppler project to this file are licensed
// under GPL version 2 or later
//
// Copyright (C) 2008 Carlos Garcia Campos <carlosgc@gnome.org>
//
// To see a description of the changes please see the Changelog file that
// came with your tarball or type make ChangeLog if you are building from git
//
//========================================================================

//========================================================================
//
// Most of the code from Link.cc and PSOutputDev.cc
//
// Copyright 1996-2003 Glyph & Cog, LLC 
//
//========================================================================

#include <config.h>

#include "FileSpec.h"

GBool getFileSpecName (Object *fileSpec, Object *fileName)
{
  if (fileSpec->isString()) {
    fileSpec->copy(fileName);
    return gTrue;
  }
  
  if (fileSpec->isDict()) {
    fileSpec->dictLookup("DOS", fileName);
    if (fileName->isString()) {
      return gTrue;
    }
    fileName->free();
    fileSpec->dictLookup("Mac", fileName);
    if (fileName->isString()) {
      return gTrue;
    }
    fileName->free();
    fileSpec->dictLookup("Unix", fileName);
    if (fileName->isString()) {
      return gTrue;
    }
    fileName->free();
    fileSpec->dictLookup("F", fileName);
    if (fileName->isString()) {
      return gTrue;
    }
    fileName->free();
  }
  return gFalse;
}

GBool getFileSpecNameForPlatform (Object *fileSpec, Object *fileName)
{
  if (fileSpec->isString()) {
    fileSpec->copy(fileName);
    return gTrue;
  }

  Object obj1;
  GooString *name;

  name = NULL;
  
  if (fileSpec->isDict()) {
#ifdef WIN32
    if (!fileSpec->dictLookup("DOS", &obj1)->isString()) {
#else
    if (!fileSpec->dictLookup("Unix", &obj1)->isString()) {
#endif
      obj1.free();
      fileSpec->dictLookup("F", &obj1);
    }
    
    if (obj1.isString()) {
       name = obj1.getString()->copy();
    } else {
      error(-1, "Illegal file spec in link");
    }
    obj1.free();

  // error
  } else {
    error(-1, "Illegal file spec in link");
  }

  // system-dependent path manipulation
  if (name) {
#ifdef WIN32
    int i, j;

    // "//...."             --> "\...."
    // "/x/...."            --> "x:\...."
    // "/server/share/...." --> "\\server\share\...."
    // convert escaped slashes to slashes and unescaped slashes to backslashes
    i = 0;
    if (name->getChar(0) == '/') {
      if (name->getLength() >= 2 && name->getChar(1) == '/') {
	name->del(0);
	i = 0;
      } else if (name->getLength() >= 2 &&
		 ((name->getChar(1) >= 'a' && name->getChar(1) <= 'z') ||
		  (name->getChar(1) >= 'A' && name->getChar(1) <= 'Z')) &&
		 (name->getLength() == 2 || name->getChar(2) == '/')) {
	name->setChar(0, name->getChar(1));
	name->setChar(1, ':');
	i = 2;
      } else {
	for (j = 2; j < name->getLength(); ++j) {
	  if (name->getChar(j-1) != '\\' &&
	      name->getChar(j) == '/') {
	    break;
	  }
	}
	if (j < name->getLength()) {
	  name->setChar(0, '\\');
	  name->insert(0, '\\');
	  i = 2;
	}
      }
    }
    for (; i < name->getLength(); ++i) {
      if (name->getChar(i) == '/') {
	name->setChar(i, '\\');
      } else if (name->getChar(i) == '\\' &&
		 i+1 < name->getLength() &&
		 name->getChar(i+1) == '/') {
	name->del(i);
      }
    }
#else
    // no manipulation needed for Unix
#endif
  } else {
    return gFalse;
  }

  fileName->initString (name);
  return gTrue;
}

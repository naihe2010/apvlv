//========================================================================
//
// DateInfo.h
//
// Copyright (C) 2008 Albert Astals Cid <aacid@kde.org>
//
// To see a description of the changes please see the Changelog file that
// came with your tarball or type make ChangeLog if you are building from git
//
//========================================================================

//========================================================================
//
// Based on code from pdfinfo.cc
//
// Copyright 1998-2003 Glyph & Cog, LLC
//
//========================================================================

#ifndef DATE_INFO_H
#define DATE_INFO_H

#include "goo/gtypes.h"

GBool parseDateString(const char *string, int *year, int *month, int *day, int *hour, int *minute, int *second, char *tz, int *tzHour, int *tzMinute);

#endif

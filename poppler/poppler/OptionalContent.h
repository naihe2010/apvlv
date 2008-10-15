//========================================================================
//
// OptionalContent.h
//
// Copyright 2007 Brad Hards <bradh@kde.org>
//
// Released under the GPL (version 2, or later, at your option)
//
//========================================================================

#ifndef OPTIONALCONTENT_H
#define OPTIONALCONTENT_H

#ifdef USE_GCC_PRAGMAS
#pragma interface
#endif

#include "Object.h"
#include "CharTypes.h"

class GooString;
class GooList;
class XRef;

class OptionalContentGroup; 

//------------------------------------------------------------------------

class OCGs {
public:

  OCGs(Object *ocgObject, XRef *xref);
  ~OCGs();

  bool hasOCGs();
  GooList *getOCGs() const { return optionalContentGroups; }

  OptionalContentGroup* findOcgByRef( const Ref &ref);

  Array* getOrderArray() const { return m_orderArray; }
  Array* getRBGroupsArray() const { return m_rBGroupsArray; }

  bool optContentIsVisible( Object *dictRef );

private:
  bool allOn( Array *ocgArray );
  bool allOff( Array *ocgArray );
  bool anyOn( Array *ocgArray );
  bool anyOff( Array *ocgArray );

  GooList *optionalContentGroups;

  Array *m_orderArray;
  Array *m_rBGroupsArray;
  XRef *m_xref;
};

//------------------------------------------------------------------------

class OptionalContentGroup {
public:
  enum State { On, Off };

  OptionalContentGroup(Dict *dict, XRef *xrefA);

  OptionalContentGroup(GooString *label);

  ~OptionalContentGroup();

  GooString* name() const;

  Ref ref() const;
  void setRef(const Ref ref);

  State state() { return m_state; };
  void setState(State state) { m_state = state; };

private:
  XRef *xref;
  GooString *m_name;
  Ref m_ref;
  State m_state;  
};

#endif

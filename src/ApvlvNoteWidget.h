/*
 * This file is part of the apvlv package
 *
 * Copyright (C) 2008 Alf.
 *
 * Contact: Alf <naihe2010@126.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2.0 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */
/* @CPPFILE ApvlvEditor.h
 *
 *  Author: Alf <naihe2010@126.com>
 */

#ifndef _APVLV_NOTE_WIDGET_H_
#define _APVLV_NOTE_WIDGET_H_

#include <QDialog>
#include <QFrame>
#include <unordered_set>

#include "ApvlvEditor.h"
#include "ApvlvNote.h"

namespace apvlv
{
class CommentEdit : public Editor
{
  Q_OBJECT

public:
private:
  Comment *mComment;
};

class NoteEdit : public QFrame
{
  Q_OBJECT

public:
private:
  Note *mNote;
};

class NoteDialog : QObject
{
  Q_OBJECT

public:
  static QString getTag (const std::unordered_set<std::string> &tags);
};

}

#endif

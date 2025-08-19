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
/* @CPPFILE ApvlvNoteWidget.cc
 *
 *  Author: Alf <naihe2010@126.com>
 */

#include "ApvlvNoteWidget.h"

#include <QInputDialog>
#include <QLabel>
#include <QPushButton>
#include <QCompleter>
#include <QVBoxLayout>

namespace apvlv
{
using namespace std;

QString
NoteDialog::getTag (const string &filename, const unordered_set<string> &tags,
                    const QStringList &tagList)
{
  auto dia = make_unique<QDialog> (nullptr);
  dia->setWindowTitle (QString::fromLocal8Bit (filename));
  dia->setModal (true);
  dia->setSizeGripEnabled (true);

  auto layout = new QVBoxLayout (dia.get ());
  dia->setLayout (layout);

  auto hbox = new QHBoxLayout ();
  auto label = new QLabel (dia.get ());
  label->setText (tr ("Input tag:"));
  hbox->addWidget (label);
  auto entry = new QLineEdit (dia.get ());
  hbox->addWidget (entry);
  layout->addLayout (hbox);

  hbox = new QHBoxLayout ();
  auto ob = new QPushButton (tr ("OK"), dia.get ());
  hbox->addWidget (ob);
  QObject::connect (ob, SIGNAL (clicked (bool)), dia.get (), SLOT (accept ()));
  auto oc = new QPushButton (tr ("Cancel"), dia.get ());
  hbox->addWidget (oc);
  QObject::connect (oc, SIGNAL (clicked (bool)), dia.get (), SLOT (reject ()));
  layout->addLayout (hbox);

  auto completer = new QCompleter(tagList, dia.get());
  completer->setFilterMode (Qt::MatchContains);
  entry->setCompleter (completer);

  auto res = dia->exec ();
  if (res != QDialog::Accepted)
    {
      return {};
    }

  auto str = entry->text ();
  return str;
}

}

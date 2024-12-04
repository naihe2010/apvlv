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
/* @CPPFILE ApvlvDired.cc
 *
 *  Author: Alf <naihe2010@126.com>
 */

#include <QFileDialog>
#include <QPushButton>
#include <QVBoxLayout>
#include <regex>

#include "ApvlvDired.h"

namespace apvlv
{
using namespace std;

DiredDialog::DiredDialog (QWidget *parent) : QDialog (parent)
{
  setLayout (&mVboxLayout);
}

void
DiredDialog::activateItem (QListWidgetItem *item)
{
  auto words = item->data (Qt::UserRole).toStringList ();
  auto path = words[0];
  auto pn = words[1].toInt ();
  emit loadFile (path.toStdString (), pn);
  accept ();
}

}

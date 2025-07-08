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
/* @CPPFILE ApvlvEditor.cc
 *
 *  Author: Alf <naihe2010@126.com>
 */

#include <filesystem>
#include <fstream>

#include "ApvlvNote.h"

namespace apvlv
{
std::string NotesDir = "/tmp";
}

using namespace std;
using namespace apvlv;
int
main (int argc, const char *argv[])
{
  auto note = Note{};
  note.setScore (8.0);
  note.addTag ("abc");
  note.addTag ("def");
  note.addTag ("hijklmn");
  auto comment1 = Comment{};
  comment1.commentText = "lksdflkasdlfkaj";
  comment1.quoteText
      = "lksjdflkajdsklfjkaldklsdlkfasldkjf\nlksdjfkla\nsdflkasd\n";
  comment1.begin = Location{ 0, 1.0, 2.0, 3 };
  comment1.end = Location{ 0, 2.0, 3.0, 4 };
  note.addComment (comment1);
  auto comment2 = Comment{};
  comment2.commentText = "bbbbbbbbbbbbbbbbbbbbbbb";
  comment2.quoteText = "bbbbbbbbbbbbbbbbbbbb\nlksdjfkla\nsdflkasd\n";
  comment2.begin = Location{ 1, 1.0, 2.0, 5 };
  comment2.end = Location{ 1, 2.0, 3.0, 8 };
  note.addComment (comment2);
  note.addReference ("abcdehijklmn1");
  note.addReference ("abcdehijklmn2");
  note.addReference ("abcdehijklmn3");
  note.addLink ("link1sldkfajlksdjfaldsk1");
  note.addLink ("link1sldkfajlksdjfaldsk2");
  note.addLink ("link1sldkfajlksdjfaldsk3");
  auto fos = ofstream{ "/tmp/testNote1.md" };
  note.dumpStream (fos);
  fos.close ();
  auto fis = ifstream{ "/tmp/testNote1.md" };
  auto note1 = Note{};
  note1.loadStream (fis);
  auto fos2 = ofstream{ "/tmp/testNote2.md" };
  note1.dumpStream (fos2);
  fos2.close ();

  return 0;
}

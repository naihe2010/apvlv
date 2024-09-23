/*
 * This file is part of the apvlv package
 * Copyright (C) <2010>  <Alf>
 *
 * Contact: Alf <naihe2010@126.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */
/* @CPPFILE ApvlvOffice.cc
 *
 *  Author: Alf <naihe2010@126.com>
 */

#include <Ole2.h>
#include <OleCtl.h>
#include <QPdfDocument>
#include <Shlwapi.h>
#include <Windows.h>
#include <ocidl.h>

#include "ApvlvAxOffice.h"
#include "ApvlvUtil.h"
#include "ApvlvWebViewWidget.h"

namespace apvlv
{
using namespace std;

FILE_TYPE_DEFINITION (ApvlvOfficeWord, { ".doc", ".docx" });
FILE_TYPE_DEFINITION (ApvlvPowerPoint, { ".ppt", ".pptx" });
FILE_TYPE_DEFINITION (ApvlvExcel, { ".xls", ".xlsx" });

bool
ApvlvOfficeWord::load (const string &filename)
{
  auto qname = QString::fromLocal8Bit (filename);
  mApp = new QAxWidget ("Word.Application");
  mApp->setProperty ("Visible", false);
  mDocs = mApp->querySubObject ("Documents");
  mDoc = mDocs->querySubObject (
      "OpenNoRepairDialog(const QString&, bool, bool, bool)", qname, false,
      true, false);
  if (mDoc == nullptr)
    {
      mApp->dynamicCall ("Quit()");
      delete mApp;
      return false;
    }
  return true;
}

int
ApvlvOfficeWord::sum ()
{
  auto content = mDoc->querySubObject ("Content");
  auto pages = content->dynamicCall ("Information(wdNumberOfPagesInDocument)");
  return pages.toInt ();
}

SizeF
ApvlvOfficeWord::pageSizeF (int pn, int rot)
{
  auto pnstr = QString ("Pages(%1)").arg (pn + 1);
  auto win = mDoc->querySubObject ("ActiveWindow");
  auto pane = win->querySubObject ("ActivePane");
  auto page = pane->querySubObject (pnstr.toStdString ().c_str ());
  auto width = page->property ("Width");
  auto height = page->property ("Height");
  return { width.toDouble (), height.toDouble () };
}

bool
ApvlvOfficeWord::pageText (int pn, const Rectangle &rect, string &text)
{
  auto content = mDoc->querySubObject ("Content");
  return false;
}

bool
ApvlvOfficeWord::pageRender (int pn, double zm, int rot, QImage *pix)
{
  char szFormatName[1024];
  const char *lpFormatName;
  static UINT auPriorityList[] = { CF_TEXT, CF_BITMAP };

  auto gf = GetPriorityClipboardFormat (auPriorityList, 2);

  auto pnstr = QString ("Pages(%1)").arg (pn + 1);
  auto win = mDoc->querySubObject ("ActiveWindow");
  auto pane = win->querySubObject ("ActivePane");
  auto page = pane->querySubObject (pnstr.toStdString ().c_str ());

  auto selection = mApp->querySubObject ("Selection");
  selection->dynamicCall ("GoTo(int, int, int, const QVariant&)", 1, 1, 1,
                          page->property ("Start"));
  selection->dynamicCall ("MoveDown(int, int, int)", 5, 1, 0);
  selection->dynamicCall ("EndKey(int, int)", 6, 1);
  selection->dynamicCall ("CopyAsPicture()");

  Sleep (1000);
  auto clip = QApplication::clipboard ();
  auto mime = clip->mimeData ();
  for (auto const &t : mime->formats ())
    {
      qDebug ("clipboard contains: %s\n", t.toStdString ().c_str ());
    }

  // HWND hWnd = (HWND)mApp->winId ();
  if (OpenClipboard (NULL) == FALSE)
    {
      qDebug ("open clipboard error: %d\n", GetLastError ());
      return false;
    }

  auto uFormat = EnumClipboardFormats (0);
  while (uFormat)
    {
      if (GetClipboardFormatNameA (uFormat, szFormatName,
                                   sizeof (szFormatName)))
        lpFormatName = szFormatName;
      else
        lpFormatName = "(unknown)";
      qDebug ("get file: %s\n", lpFormatName);
      uFormat = EnumClipboardFormats (uFormat);
    }

  if (IsClipboardFormatAvailable (CF_BITMAP) == FALSE)
    {
      qDebug ("no picture: %d\n", GetLastError ());
      CloseClipboard ();
      return false;
    }

  HBITMAP hBitmap = static_cast<HBITMAP> (GetClipboardData (CF_BITMAP));
  if (hBitmap == nullptr)
    {
      qDebug ("no image: %d\n", GetLastError ());
      CloseClipboard ();
      return false;
    }
  IPicture *iPicture = nullptr;
  auto hr = OleCreatePictureIndirect (reinterpret_cast<LPPICTDESC> (hBitmap),
                                      IID_IPicture, TRUE, (void **)&iPicture);
  if (FAILED (hr))
    {
      qDebug ("failed create: %d\n", GetLastError ());
      CloseClipboard ();
      return false;
    }
  LPSTREAM stream = NULL;
  hr = SHCreateStreamOnFileA ("z:\\a.bmp", STGM_CREATE | STGM_WRITE, &stream);
  if (FAILED (hr))
    {
      iPicture->Release ();
      CloseClipboard ();
      return false;
    }

  hr = iPicture->SaveAsFile (stream, FALSE, NULL);
  iPicture->Release ();
  CloseClipboard ();
  if (FAILED (hr))
    {
      qDebug ("save failed\n");
      return false;
    }

  return true;
}

bool
ApvlvOfficeWord::pageRender (int pn, double zm, int rot, WebView *webview)
{
  webview->setZoomFactor (zm);
  QUrl url = QString ("apvlv:///%1").arg (pn);
  webview->load (url);
  return true;
}

optional<QByteArray>
ApvlvOfficeWord::pathContent (const string &path)
{
  auto pn = QString::fromLocal8Bit (path).toInt ();
  auto pageRange = mDoc->querySubObject (
      "GoTo(int, int, int, const QVariant&)", 1, 1, pn + 1);
  auto endRange = mDoc->querySubObject ("GoTo(int, int, int, const QVariant&)",
                                        1, 1, pn + 2);
  if (pageRange && endRange)
    {
      int endPosition;
      if (endRange->property ("Start").toInt () == 1)
        {
          auto content = mDoc->querySubObject ("Content");
          endPosition = content->property ("End").toInt ();
          delete content;
        }
      else
        {
          endPosition = endRange->property ("Start").toInt () - 1;
        }

      pageRange->setProperty ("End", endPosition);

      pageRange->dynamicCall ("Copy()");
    }
  else
    {
      qWarning ("Failed to get page range\n");
      return nullptr;
    }

  auto clip = QApplication::clipboard ();
  auto mime = clip->mimeData ();
  return QByteArray::fromStdString (mime->html ().toStdString ());
}

bool
ApvlvPowerPoint::load (const string &filename)
{
  auto qname = QString::fromLocal8Bit (filename);
  mApp = new QAxWidget ("PowerPoint.Application");
  mApp->setProperty ("Visible", false);
  mDocs = mApp->querySubObject ("Presentations");
  mDoc = mDocs->querySubObject ("Open(const QString&, bool, bool, bool)",
                                qname, true, false, false);
  if (mDoc == nullptr)
    {
      mApp->dynamicCall ("Quit()");
      delete mApp;
      return false;
    }
  return true;
}

int
ApvlvPowerPoint::sum ()
{
  auto slides = mDoc->querySubObject ("Slides");
  auto count = slides->property ("Count");
  return count.toInt ();
}

SizeF
ApvlvPowerPoint::pageSizeF (int pn, int rot)
{
  auto page = mDoc->querySubObject ("PageSetup");
  auto width = page->property ("SlideWidth");
  auto height = page->property ("SlideHeight");
  return { width.toDouble (), height.toDouble () };
}

bool
ApvlvPowerPoint::pageText (int pn, const Rectangle &rect, string &text)
{
  auto content = mDoc->querySubObject ("Content");
  return false;
}

bool
ApvlvPowerPoint::pageRender (int pn, double zm, int rot, QImage *pix)
{
  auto slides = mDoc->querySubObject ("Slides");
  auto slide = slides->querySubObject ("Item(int)", pn + 1);
  auto temppath
      = QString ("%1/apvlv_%2.png").arg (QDir::tempPath ()).arg (rand ());
  temppath.replace ("/", "\\");
  slide->dynamicCall ("Export(const QString &, const QString &)", temppath,
                      QString ("PNG"));

  if (QFile::exists (temppath) == false)
    {
      return false;
    }
  *pix = QImage (temppath);
  QFile::remove (temppath);
  return true;
}

ExcelWidget *
ApvlvExcel::getWidget ()
{
  auto wid = new ExcelWidget ();
  wid->setFile (this);
  wid->createWidget ();
  return wid;
}

QWidget *
ExcelWidget::createWidget ()
{
  auto qname = QString::fromLocal8Bit (mFile->getFilename ());
  mAxWidget = new QAxWidget ("Excel.Workbook");
  mAxWidget->setProperty ("Visible", true);
  mAxWidget->setProperty ("ReadOnly", true);
  mAxWidget->setControl (qname);
  return mAxWidget;
}

void
ExcelWidget::showPage (int p, double s)
{
  auto widget = dynamic_cast<QAxWidget *> (mWidget);
  auto sheets = widget->querySubObject ("Sheets");
  auto sheet = sheets->querySubObject ("Item(int)", p + 1);
  sheet->dynamicCall ("Activate()");
  mPageNumber = p;
  mScrollValue = s;
}

void
ExcelWidget::showPage (int p, const string &anchor)
{
  showPage (p, 0.0);
  mPageNumber = p;
  mScrollValue = 0;
}

bool
ApvlvExcel::load (const string &filename)
{
  auto qname = QString::fromLocal8Bit (filename);
  mApp = new QAxWidget ("Excel.Workbook");
  mApp->setProperty ("Visible", false);
  mApp->setProperty ("ReadOnly", true);
  mApp->setControl (qname);
  mDoc = nullptr;
  return true;
}

int
ApvlvExcel::sum ()
{
  auto sheets = mApp->querySubObject ("Sheets");
  auto count = sheets->property ("Count");
  return count.toInt ();
}

bool
ApvlvExcel::pageText (int pn, const Rectangle &rect, string &text)
{
  return false;
  // auto content = mDoc->querySubObject ("Content");
}

}

// Local Variables:
// mode: c++
// End:

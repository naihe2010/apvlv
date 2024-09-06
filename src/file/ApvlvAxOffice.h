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
/* @CPPFILE ApvlvOffice.h
 *
 *  Author: Alf <naihe2010@126.com>
 */

#ifndef _APVLV_AXOFFICE_H_
#define _APVLV_AXOFFICE_H_

#include <QtAxContainer>

#include "ApvlvFile.h"
#include "ApvlvFileWidget.h"

namespace apvlv
{
class AxOffice
{
public:
  virtual ~AxOffice ()
  {
    if (mDoc)
      mDoc->dynamicCall ("Close()");
    if (mApp)
      mApp->dynamicCall ("Quit()");
    delete mDoc;
    delete mApp;
  }

protected:
  QAxWidget *mApp;
  QAxObject *mDocs;
  QAxObject *mDoc;
};

class ApvlvOfficeWord : public File, public AxOffice
{
  FILE_TYPE_DECLARATION (ApvlvOfficeWord);

public:
  explicit ApvlvOfficeWord (const string &filename, bool check = true);

  int sum () override;

  Size pageSize (int page, int rot) override;

  bool pageText (int pn, const Rectangle &rect, string &text) override;

  bool pageRender (int pn, double zm, int rot, QImage *pix) override;

  bool pageRender (int pn, double zm, int rot, WebView *webview) override;

  optional<QByteArray> pathContent (const string &path) override;
};

class ApvlvPowerPoint : public File, public AxOffice
{
  FILE_TYPE_DECLARATION (ApvlvPowerPoint);

public:
  explicit ApvlvPowerPoint (const string &filename, bool check = true);

  int sum () override;

  Size pageSize (int page, int rot) override;

  bool pageText (int pn, const Rectangle &rect, string &text) override;

  bool pageRender (int pn, double zm, int rot, QImage *pix) override;
};

class ExcelWidget : public FileWidget
{
public:
  QWidget *createWidget () override;

  void showPage (int, double s) override;
  void showPage (int, const string &anchor) override;

private:
  QAxWidget *mAxWidget;
};

class ApvlvExcel : public File, public AxOffice
{
  FILE_TYPE_DECLARATION (ApvlvExcel);

public:
  explicit ApvlvExcel (const string &filename, bool check = true);

  [[nodiscard]] virtual DISPLAY_TYPE
  getDisplayType () const override
  {
    return DISPLAY_TYPE_CUSTOM;
  }

  ExcelWidget *getWidget () override;

  int sum () override;

  bool pageText (int pn, const Rectangle &rect, string &text) override;
};

}

#endif

/* Local Variables: */
/* mode: c++ */
/* End: */

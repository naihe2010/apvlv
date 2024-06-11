//
// Created by p on 24-6-15.
//

#ifndef _TESTWEBENGINE_H_
#define _TESTWEBENGINE_H_

#include <QMainWindow>
#include <QTimer>
#include <QWebEngineProfile>
#include <QWebEngineUrlSchemeHandler>
#include <QWebEngineView>

using namespace std;

class ApvlvFile
{
};

class ApvlvSchemeHandler : public QWebEngineUrlSchemeHandler
{
  Q_OBJECT
public:
  explicit ApvlvSchemeHandler (ApvlvFile *file) { mFile = file; }
  ~ApvlvSchemeHandler () = default;

  void requestStarted (QWebEngineUrlRequestJob *job) override;

private:
  ApvlvFile *mFile;
};

class TestWebEngine : public QMainWindow
{
  Q_OBJECT
public:
  TestWebEngine () = default;
  ~TestWebEngine () = default;
  void setupUi ();

private:
  unique_ptr<QWebEngineView> mWeb[1];
  unique_ptr<QWebEngineProfile> mWebProfile[1];
  unique_ptr<QTimer> mTimer;

private slots:
  void timeout ();
};

#endif //_TESTWEBENGINE_H_

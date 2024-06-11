//
// Created by p on 24-6-15.
//

#include "TestWebEngine.h"
#include <QApplication>
#include <QTimer>
#include <QWebEngineProfile>
#include <QWebEngineUrlRequestJob>
#include <QWebEngineUrlScheme>
#include <iostream>

int
main (int argc, char *argv[])
{
  QWebEngineUrlScheme scheme ("apvlv");
  scheme.setSyntax (QWebEngineUrlScheme::Syntax::Path);
  // scheme.setFlags (QWebEngineUrlScheme::SecureScheme);
  QWebEngineUrlScheme::registerScheme (scheme);

  QApplication app (argc, argv);

  TestWebEngine test_web_engine;
  test_web_engine.setupUi ();

  QApplication::exec ();

  return 0;
}

void
TestWebEngine::setupUi ()
{
  mWebProfile[0] = make_unique<QWebEngineProfile> ();
  auto handler = new ApvlvSchemeHandler (nullptr);
  mWebProfile[0]->installUrlSchemeHandler (QByteArray ("apvlv"), handler);
  mWeb[0] = make_unique<QWebEngineView> (mWebProfile[0].get ());

  setCentralWidget (mWeb[0].get ());

  mTimer = make_unique<QTimer> ();
  QObject::connect (mTimer.get (), SIGNAL (timeout ()), this,
                    SLOT (timeout ()));
  mTimer->start (3000);

  showNormal ();
}

void
TestWebEngine::timeout ()
{
  auto url = QUrl ("apvlv:///1");
  cout << "load " << url.toString ().toStdString () << endl;
  mWeb[0]->load (url);
  mTimer->stop ();
}
void
ApvlvSchemeHandler::requestStarted (QWebEngineUrlRequestJob *job)
{
  cout << "method be called" << endl;
  const QUrl url = job->requestUrl ();
  job->reply (QByteArray ("text/html"), nullptr);
}
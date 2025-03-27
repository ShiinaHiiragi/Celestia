/***************************************************************************
                          qtmain.cpp  -  description
                             -------------------
    begin                : Tue Jul 16 22:28:19 CEST 2002
    copyright            : (C) 2002 by Christophe Teyssier
    email                : chris@teyssier.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#define SERVER_VERSION "0.1"

#include <iostream>
#include <optional>

#include <QtGlobal>
#include <QApplication>
#include <QBitmap>
#include <QColor>
#include <QCoreApplication>
#include <QDesktopServices>
#include <QDir>
#include <QLibraryInfo>
#include <QLocale>
#include <QObject>
#include <QPixmap>
#include <QSplashScreen>
#include <QString>
#include <QTranslator>

#include <celutil/gettext.h>
#include <celutil/localeutil.h>
#include <celmath/frustum.h>
#include "qtappwin.h"
#include "qtcommandline.h"
#include "qtgettext.h"

#include "http_message.h"
#include "http_server.h"


using simple_http_server::HttpMethod;
using simple_http_server::HttpRequest;
using simple_http_server::HttpResponse;
using simple_http_server::HttpServer;
using simple_http_server::HttpStatusCode;

CelestiaCore *appCore;

HttpResponse status_version(const HttpRequest &_) {
    std::cout << "GET /version" << "\n";

    HttpResponse response(HttpStatusCode::Ok);
    response.SetHeader("Content-Type", "application/json");
    response.SetContent(SERVER_VERSION);
    return response;
}

HttpResponse status_dump(const HttpRequest &request) {
    std::string query = request.content();
    std::cout << "GET /dump with " << query << "\n";

    try {
        HttpResponse response(HttpStatusCode::Ok);
        response.SetHeader("Content-Type", "application/json");
        response.SetContent(appCore->getStatus());
        return response;
    } catch (...) {
        HttpResponse response(HttpStatusCode::BadRequest);
        return response;
    }
}

#ifdef ENABLE_NLS
namespace
{

inline void
bindTextDomainUTF8(const char* domainName, const QString& directory)
{
#ifdef _WIN32
    wbindtextdomain(domainName, directory.toStdWString().c_str());
#else
    bindtextdomain(domainName, directory.toUtf8().data());
#endif
    bind_textdomain_codeset(domainName, "UTF-8");
}

} // end unnamed namespace
#endif

int main(int argc, char *argv[])
{
    using namespace celestia::qt;

#ifndef GL_ES
    QCoreApplication::setAttribute(Qt::AA_UseDesktopOpenGL);
#else
    QCoreApplication::setAttribute(Qt::AA_UseOpenGLES);
#endif
    QApplication app(argc, argv);

    // Gettext integration
    CelestiaCore::initLocale();
#ifdef ENABLE_NLS
    QString localeDir = LOCALEDIR;
    bindTextDomainUTF8("celestia", localeDir);
    bindTextDomainUTF8("celestia-data", localeDir);
    textdomain("celestia");
#endif

    if (QTranslator qtTranslator;
        qtTranslator.load("qt_" + QLocale::system().name(),
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
                          QLibraryInfo::location(QLibraryInfo::TranslationsPath)))
#else
                          QLibraryInfo::path(QLibraryInfo::TranslationsPath)))
#endif
    {
        app.installTranslator(&qtTranslator);
    }

    CelestiaQTranslator celestiaTranslator;
    app.installTranslator(&celestiaTranslator);

    Q_INIT_RESOURCE(icons);

    QCoreApplication::setOrganizationName("Celestia Development Team");
    QCoreApplication::setOrganizationDomain("celestiaproject.space");
    QCoreApplication::setApplicationName("Celestia QT");
    QCoreApplication::setApplicationVersion(VERSION);

    CelestiaCommandLineOptions options = ParseCommandLine(app);

    std::optional<QSplashScreen> splash{std::nullopt};
    if (!options.skipSplashScreen)
    {
        QDir splashDir(CONFIG_DATA_DIR "/splash");
        if (!options.startDirectory.isEmpty())
        {
            QDir newSplashDir = QString("%1/splash").arg(options.startDirectory);
            if (newSplashDir.exists("splash.png"))
                splashDir = std::move(newSplashDir);
        }

        QPixmap pixmap(splashDir.filePath("splash.png"));
        splash.emplace(pixmap);
        splash->setMask(pixmap.mask());


        // TODO: resolve issues with pixmap alpha channel
        splash->show();
        app.processEvents();
    }

    CelestiaAppWindow window;

    if (splash.has_value())
    {
        // Connect the splash screen to the main window so that it
        // can receive progress notifications as Celestia files required
        // for startup are loaded.
        QObject::connect(&window, SIGNAL(progressUpdate(const QString&, int, const QColor&)),
                         &*splash, SLOT(showMessage(const QString&, int, const QColor&)));
    }

    window.init(options);
    window.show();
    window.startAppCore();

    if (splash.has_value())
    {
        splash->finish(&window);
    }

    // Set the main window to be the cel url handler
    QDesktopServices::setUrlHandler("cel", &window, "handleCelUrl");

    int port = 8000;
    if (argc > 1) {
        port = std::stoi(argv[1]);
    }

    std::string host = "0.0.0.0";
    HttpServer server(host, port);

    server.RegisterHttpRequestHandler("/version", HttpMethod::HEAD, status_version);
    server.RegisterHttpRequestHandler("/version", HttpMethod::GET, status_version);

    server.RegisterHttpRequestHandler("/dump", HttpMethod::HEAD, status_dump);
    server.RegisterHttpRequestHandler("/dump", HttpMethod::GET, status_dump);

    appCore = window.getAppCore();
    server.Start();
    std::cout << "Server listening on " << host << ":" << port << std::endl;

    int ret = app.exec();
    QDesktopServices::unsetUrlHandler("cel");
    return ret;
}

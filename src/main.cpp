#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QDir>
#include <QQuickWindow>
#include <QNetworkProxy>
#include <QSslConfiguration>
#include <QProcess>
#include <QtQml/qqmlextensionplugin.h>
#include <QLoggingCategory>
#include "Version.h"
#include "AppInfo.h"
#include "component/CircularReveal.h"
#include "component/FileWatcher.h"
#include "component/FpsItem.h"
#include "component/OpenGLItem.h"
#include "helper/Log.h"
#include "helper/SettingsHelper.h"
#include "helper/InitializrHelper.h"
#include "helper/TranslateHelper.h"
#include "helper/Network.h"

#ifdef FLUENTUI_BUILD_STATIC_LIB
#if (QT_VERSION > QT_VERSION_CHECK(6, 2, 0))
Q_IMPORT_QML_PLUGIN(FluentUIPlugin)
#endif
#include <FluentUI.h>
#endif

#ifdef WIN32
#include "app_dmp.h"
#endif

int main(int argc, char *argv[])
{
    const char *uri = "example";
    int major = 1;
    int minor = 0;
#ifdef WIN32
    ::SetUnhandledExceptionFilter(MyUnhandledExceptionFilter);
    qputenv("QT_QPA_PLATFORM","windows:darkmode=2");
#endif
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
    qputenv("QT_QUICK_CONTROLS_STYLE","Basic");
#else
    qputenv("QT_QUICK_CONTROLS_STYLE","Default");
#endif
#ifdef Q_OS_LINUX
    //fix bug UOSv20 does not print logs
    qputenv("QT_LOGGING_RULES","");
    //fix bug UOSv20 v-sync does not work
    qputenv("QSG_RENDER_LOOP","basic");
#endif
    QGuiApplication::setOrganizationName("ysme");
    QGuiApplication::setOrganizationDomain("https://blog.ysme.top/");
    QGuiApplication::setApplicationName("D01_TOOL");
    QGuiApplication::setApplicationDisplayName("D01_TOOL TEST");
    QGuiApplication::setApplicationVersion(APPLICATION_VERSION);
    QGuiApplication::setQuitOnLastWindowClosed(false);
    SettingsHelper::getInstance()->init(argv);
    Log::setup(argv,uri);

#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
    QQuickWindow::setGraphicsApi(QSGRendererInterface::OpenGL);
#endif
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
    QGuiApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QGuiApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
#if (QT_VERSION >= QT_VERSION_CHECK(5, 14, 0))
    QGuiApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
#endif
#endif
    QGuiApplication app(argc, argv);
    //@uri example
    qmlRegisterType<CircularReveal>(uri, major, minor, "CircularReveal");
    qmlRegisterType<FileWatcher>(uri, major, minor, "FileWatcher");
    qmlRegisterType<FpsItem>(uri, major, minor, "FpsItem");
    qmlRegisterType<NetworkCallable>(uri,major,minor,"NetworkCallable");
    qmlRegisterType<NetworkParams>(uri,major,minor,"NetworkParams");
    qmlRegisterType<OpenGLItem>(uri,major,minor,"OpenGLItem");
    qmlRegisterUncreatableMetaObject(NetworkType::staticMetaObject, uri, major, minor, "NetworkType", "Access to enums & flags only");

    QQmlApplicationEngine engine;
    TranslateHelper::getInstance()->init(&engine);
    engine.rootContext()->setContextProperty("AppInfo",AppInfo::getInstance());
    engine.rootContext()->setContextProperty("SettingsHelper",SettingsHelper::getInstance());
    engine.rootContext()->setContextProperty("InitializrHelper",InitializrHelper::getInstance());
    engine.rootContext()->setContextProperty("TranslateHelper",TranslateHelper::getInstance());
    engine.rootContext()->setContextProperty("Network",Network::getInstance());
#ifdef FLUENTUI_BUILD_STATIC_LIB
    FluentUI::getInstance()->registerTypes(&engine);
#endif
    const QUrl url(QStringLiteral("qrc:/qml/App.qml"));
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
        &app, [url](QObject *obj, const QUrl &objUrl) {
            if (!obj && url == objUrl)
                QCoreApplication::exit(-1);
        }, Qt::QueuedConnection);
    engine.load(url);
    const int exec = QGuiApplication::exec();
    if (exec == 931) {
        QProcess::startDetached(qApp->applicationFilePath(), qApp->arguments());
    }
    return exec;
}

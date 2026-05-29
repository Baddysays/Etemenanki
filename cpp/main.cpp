#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QGuiApplication>
#include <QIcon>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQmlError>
#include <QQuickStyle>

#include "app_settings.h"
#include "document_loader.h"
#include "setup_manager.h"
#include "translator_backend.h"

static int runPdfPipelineCliTest(int argc, char* argv[], const QString& filePath)
{
    QCoreApplication app(argc, argv);
    DocumentLoader::ensureReady();

    const QString logPath = DocumentLoader::pythonDebugLogPath();
    auto logLine = [&logPath](const QString& line) {
        QFile f(logPath);
        if (f.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text))
            f.write((line + QStringLiteral("\n")).toUtf8());
    };

    const QString copy = DocumentLoader::copyPdfForPythonWork(filePath);
    logLine(QStringLiteral("[cli] copy: %1").arg(copy.isEmpty() ? QStringLiteral("FAILED") : copy));

    if (copy.isEmpty())
        return 2;

    const DocumentLoadResult extracted = DocumentLoader::extractPdfLayout(filePath, copy);
    logLine(QStringLiteral("[cli] extract ok=%1 err=%2")
                .arg(extracted.ok)
                .arg(extracted.error));

    if (!extracted.ok)
        return 3;

    const QString outPdf =
        QFileInfo(copy).absolutePath() + QStringLiteral("/cli_test_output.pdf");
    const PdfBuildResult built =
        DocumentLoader::buildTranslatedPdf(copy, extracted.pdfLayout, outPdf);
    logLine(QStringLiteral("[cli] build ok=%1 out=%2 err=%3")
                .arg(built.ok)
                .arg(built.outputPath)
                .arg(built.error));

    return built.ok ? 0 : 4;
}

static QString brandingFilePath(const QString& fileName)
{
    return QDir(QCoreApplication::applicationDirPath())
        .filePath(QStringLiteral("assets/branding/") + fileName);
}

static QString findBrandingFile(const QString& fileName)
{
    QStringList candidates = {brandingFilePath(fileName)};
    QDir dir(QCoreApplication::applicationDirPath());
    for (int i = 0; i < 6; ++i) {
        candidates << dir.filePath(QStringLiteral("assets/branding/") + fileName);
        if (!dir.cdUp())
            break;
    }
    for (const QString& path : candidates) {
        if (QFile::exists(path))
            return QDir::toNativeSeparators(path);
    }
    return {};
}

static QString startupLogPath()
{
    return QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("qml_startup.log"));
}

static void appendStartupLog(const QString& line)
{
    QFile f(startupLogPath());
    if (f.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text))
        f.write((line + QStringLiteral("\n")).toUtf8());
}

static void setupBranding(QGuiApplication& app, QQmlApplicationEngine& engine)
{
    const QString iconPath = findBrandingFile(QStringLiteral("app.ico"));
    QIcon appIcon;
    if (!iconPath.isEmpty()) {
        appIcon = QIcon(iconPath);
        if (!appIcon.isNull())
            app.setWindowIcon(appIcon);
    }
    if (appIcon.isNull()) {
        const QString pngIcon = findBrandingFile(QStringLiteral("app-icon.png"));
        if (!pngIcon.isEmpty()) {
            appIcon.addFile(pngIcon, {512, 512});
            if (!appIcon.isNull())
                app.setWindowIcon(appIcon);
        }
    }

    const QString headerPath = findBrandingFile(QStringLiteral("logo-header.png"));
    const QString horizontalPath = findBrandingFile(QStringLiteral("logo-horizontal.png"));
    const QString header = !headerPath.isEmpty() ? headerPath : horizontalPath;
    engine.rootContext()->setContextProperty(
        QStringLiteral("brandLogoHeader"),
        header.isEmpty() ? QUrl() : QUrl::fromLocalFile(header));
}

int main(int argc, char* argv[])
{
    if (argc >= 3 && QString::fromLocal8Bit(argv[1]) == QStringLiteral("--pdf-test")) {
        const QString filePath = QString::fromLocal8Bit(argv[2]);
        return runPdfPipelineCliTest(argc, argv, filePath);
    }

    QQuickStyle::setStyle(QStringLiteral("Basic"));
    QGuiApplication app(argc, argv);
    QQmlApplicationEngine engine;

    AppSettings appSettings;
    SetupManager setupManager;
    setupManager.setAppSettings(&appSettings);
    TranslatorBackend backend;
    backend.setAppSettings(&appSettings);
    QObject::connect(&appSettings, &AppSettings::changed, &backend, &TranslatorBackend::onAppUiLanguageChanged);
    engine.rootContext()->setContextProperty("settings", &appSettings);
    engine.rootContext()->setContextProperty("setup", &setupManager);
    engine.rootContext()->setContextProperty("backend", &backend);
    setupBranding(app, engine);

    QFile::remove(startupLogPath());
    appendStartupLog(QStringLiteral("[startup] loading Main.qml"));

    QObject::connect(&engine, &QQmlApplicationEngine::warnings,
                     &app, [](const QList<QQmlError>& warnings) {
        for (const QQmlError& warning : warnings) {
            const QString msg = warning.toString();
            qWarning().noquote() << msg;
            appendStartupLog(QStringLiteral("[qml] ") + msg);
        }
    });

    const QUrl url(u"qrc:/Etemenanki/qml_cpp/Main.qml"_qs);
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
                     &app, [url](QObject* obj, const QUrl& objUrl) {
        if (!obj && url == objUrl) {
            appendStartupLog(QStringLiteral("[fatal] failed to create root object: ") + objUrl.toString());
            QCoreApplication::exit(-1);
        }
    }, Qt::QueuedConnection);

    engine.load(url);
    if (engine.rootObjects().isEmpty())
        appendStartupLog(QStringLiteral("[fatal] engine.rootObjects() is empty after load"));
    return app.exec();
}

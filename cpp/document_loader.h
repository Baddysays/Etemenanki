#pragma once

#include <QHash>
#include <QJsonArray>
#include <QProcess>
#include <QJsonObject>
#include <QString>
#include <QStringList>
#include <QVariantMap>

struct DocumentLoadResult
{
    bool ok = false;
    QString error;
    QString text;
    QStringList pages;
    QList<QJsonArray> pageBlocks;
    QJsonObject pdfLayout;
    QJsonObject workflowMeta;
    int pageCount = 0;
    QString encoding;
    QString workflowId;
};

struct ExportDocumentResult
{
    bool ok = false;
    QString error;
    QString outputPath;
};

struct PdfBuildResult
{
    bool ok = false;
    QString error;
    QString outputPath;
    QJsonObject layout;
};

struct ExternalPdfTranslateResult
{
    bool ok = false;
    QString error;
    QString outputPath;
};

struct ExternalPdfTranslateRequest
{
    QString engine;
    QString inputPath;
    QString outputPath;
    QString srcLang;
    QString dstLang;
    QString runtime;
    QString model;
    QString ollamaUrl;
    QString cloudBase;
    QString cloudKey;
};

/** Shared timeout for C++ wait and Python subprocess (seconds / ms). */
inline constexpr int kExternalPdfTranslateTimeoutSec = 7200;
inline constexpr int kExternalPdfTranslateTimeoutMs = kExternalPdfTranslateTimeoutSec * 1000;

struct ExternalPdfTranslateCommand
{
    QString python;
    QString script;
    QStringList args;
    QHash<QString, QString> extraEnv;
};

struct ExtractRuntimeInfo
{
    bool ready = false;
    QString pythonPath;
    QString scriptPath;
    QString message;
};

class DocumentLoader
{
public:
    static ExtractRuntimeInfo probe();
    static bool ensureReady();
    static DocumentLoadResult load(const QString& filePath);
    static DocumentLoadResult extractPdfLayout(const QString& filePath,
                                               const QString& prebuiltWorkPath = {});
    static QString copyPdfForPythonWork(const QString& sourcePath);
    static PdfBuildResult buildTranslatedPdf(const QString& sourcePath,
                                             const QJsonObject& layout,
                                             const QString& outputPath);
    static PdfBuildResult buildTranslatedPdfFromPages(const QString& sourcePath,
                                                      const QJsonObject& layout,
                                                      const QStringList& translatedPages,
                                                      const QString& outputPath,
                                                      const QString& prebuiltWorkPath = {});
    static QString activePythonPath();
    static ExportDocumentResult exportDocument(const QString& sourcePath,
                                               const QString& outputPath,
                                               const QString& workflowId,
                                               const QJsonObject& workflowMeta);
    static QVariantMap probePdfEngines();
    static ExternalPdfTranslateResult translatePdfExternal(const ExternalPdfTranslateRequest& request);

    static ExternalPdfTranslateCommand buildExternalPdfTranslateCommand(
        const ExternalPdfTranslateRequest& request);

    static ExternalPdfTranslateResult parseExternalPdfTranslateOutput(
        const ExternalPdfTranslateRequest& request,
        const QByteArray& stdoutOut,
        const QByteArray& stderrOut,
        int exitCode,
        bool processStarted);

    static void configurePythonProcess(QProcess& proc,
                                       const QHash<QString, QString>& extraEnv = {});

    static void logPythonRun(const QString& tag,
                             const QString& commandLine,
                             const QByteArray& stdoutOut,
                             const QByteArray& stderrOut,
                             int exitCode,
                             bool processStarted);

    static QString pythonDebugLogPath();

private:
    static QString cachedPython();
    static QString cachedScript();
    static void setCache(const QString& python, const QString& script);
};

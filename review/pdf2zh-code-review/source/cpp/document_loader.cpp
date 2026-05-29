#include "document_loader.h"

#include "translation_workflow.h"

#include "runtime_config.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QProcessEnvironment>
#include <QStandardPaths>
#include <QStringConverter>
#include <QTemporaryFile>
#include <QMutex>
#include <QPdfDocument>
#include <QPdfSelection>
#include <QTextStream>
#include <QThread>
#include <QTimer>
#include <QUuid>
#include <QVariantMap>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

namespace {

QMutex g_cacheMutex;
QString g_pythonPath;
QString g_scriptPath;

QStringList ancestorDirs()
{
    QStringList dirs;
    QDir dir(QCoreApplication::applicationDirPath());
    for (int i = 0; i < 10; ++i) {
        dirs << QDir::cleanPath(dir.absolutePath());
        if (!dir.cdUp())
            break;
    }
    return dirs;
}

QString readPythonPathFile()
{
    const QString appDir = QCoreApplication::applicationDirPath();
    QStringList candidates = {
        appDir + QStringLiteral("/tools/python_path.txt"),
    };
    for (const QString& root : ancestorDirs()) {
        candidates << root + QStringLiteral("/tools/python_path.txt");
    }

    for (const QString& path : candidates) {
        QFile f(path);
        if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
            continue;
        const QString line = QString::fromUtf8(f.readLine()).trimmed().remove(QChar(0xFEFF));
        if (!line.isEmpty() && QFile::exists(line))
            return QDir::toNativeSeparators(line);
    }
    return {};
}

QString configuredPython()
{
#if ETE_HAVE_VENV
    const QString path = QStringLiteral(ETE_PYTHON_EXECUTABLE);
    if (!path.isEmpty() && QFile::exists(path))
        return QDir::toNativeSeparators(path);
#endif
    return readPythonPathFile();
}

bool pythonCanImportDeps(const QString& python, QByteArray* errorOut);

bool isMainThread()
{
    const QCoreApplication* app = QCoreApplication::instance();
    return app && QThread::currentThread() == app->thread();
}

bool tryCachePython(const QString& python, const QString& script)
{
    if (python.isEmpty() || !QFile::exists(python))
        return false;
    if (isMainThread()) {
        if (!pythonCanImportDeps(python, nullptr))
            return false;
    }
    {
        QMutexLocker lock(&g_cacheMutex);
        g_pythonPath = python;
        g_scriptPath = script;
    }
    return true;
}

QStringList pythonSearchPaths()
{
    QStringList paths;
    const QString configured = configuredPython();
    if (!configured.isEmpty())
        paths << configured;

    const QString appDir = QCoreApplication::applicationDirPath();
    paths << appDir + QStringLiteral("/engines/python/python.exe");
    paths << appDir + QStringLiteral("/python/python.exe");

    for (const QString& root : ancestorDirs()) {
        paths << root + QStringLiteral("/.venv/Scripts/python.exe");
        paths << root + QStringLiteral("/venv/Scripts/python.exe");
    }

    paths << QStringLiteral("py");
    paths << QStringLiteral("python");
    paths << QStringLiteral("python3");
    return paths;
}

QString findExtractScript()
{
    const QString appDir = QCoreApplication::applicationDirPath();
    QStringList candidates = {
        appDir + QStringLiteral("/tools/extract_document.py"),
    };

    for (const QString& root : ancestorDirs()) {
        candidates << root + QStringLiteral("/tools/extract_document.py");
    }

#if defined(ETE_PROJECT_SOURCE_DIR)
    candidates << QString::fromUtf8(ETE_PROJECT_SOURCE_DIR) + QStringLiteral("/tools/extract_document.py");
#endif

    for (const QString& path : candidates) {
        if (QFile::exists(path))
            return QDir::toNativeSeparators(path);
    }
    return candidates.value(0);
}

QString findExportScript()
{
    const QString appDir = QCoreApplication::applicationDirPath();
    QStringList candidates = {
        appDir + QStringLiteral("/tools/export_document.py"),
    };

    for (const QString& root : ancestorDirs()) {
        candidates << root + QStringLiteral("/tools/export_document.py");
    }

#if defined(ETE_PROJECT_SOURCE_DIR)
    candidates << QString::fromUtf8(ETE_PROJECT_SOURCE_DIR) + QStringLiteral("/tools/export_document.py");
#endif

    for (const QString& path : candidates) {
        if (QFile::exists(path))
            return QDir::toNativeSeparators(path);
    }
    return candidates.value(0);
}

QString resolvePythonPath()
{
    const QString appDir = QCoreApplication::applicationDirPath();
    QStringList candidates;

    const QString configured = configuredPython();
    if (!configured.isEmpty())
        candidates << configured;

    candidates << readPythonPathFile();
    candidates << appDir + QStringLiteral("/python/python.exe");

    for (const QString& root : ancestorDirs()) {
        candidates << root + QStringLiteral("/.venv/Scripts/python.exe");
        candidates << root + QStringLiteral("/venv/Scripts/python.exe");
    }

    candidates << QStringLiteral("py");
    candidates << QStringLiteral("python");
    candidates << QStringLiteral("python3");

    for (const QString& python : candidates) {
        if (python == QStringLiteral("py") || python == QStringLiteral("python")
            || python == QStringLiteral("python3")) {
            continue;
        }
        if (QFile::exists(python))
            return QDir::toNativeSeparators(python);
    }
    return {};
}

bool primePythonCache(bool verifyImports)
{
    const QString script = findExtractScript();
    if (!QFile::exists(script))
        return false;

    const QString python = resolvePythonPath();
    if (python.isEmpty())
        return false;

    if (verifyImports && isMainThread() && !pythonCanImportDeps(python, nullptr))
        return false;

    QMutexLocker lock(&g_cacheMutex);
    g_pythonPath = python;
    g_scriptPath = script;
    return true;
}

QByteArray firstJsonLine(const QByteArray& output)
{
    const QList<QByteArray> lines = output.split('\n');
    for (const QByteArray& line : lines) {
        const QByteArray trimmed = line.trimmed();
        if (trimmed.startsWith('{') || trimmed.startsWith('['))
            return trimmed;
    }
    return output.trimmed();
}

QString etemenankiDebugDir()
{
    const QString dir =
        QStandardPaths::writableLocation(QStandardPaths::TempLocation) + QStringLiteral("/Etemenanki");
    QDir().mkpath(dir);
    return dir;
}

QString etemenankiPythonDebugLogPath()
{
    return QDir::toNativeSeparators(etemenankiDebugDir() + QStringLiteral("/python_debug.log"));
}

void appendDebugLog(const QString& tag, const QString& body)
{
    QFile log(etemenankiPythonDebugLogPath());
    if (!log.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text))
        return;

    QTextStream stream(&log);
    stream << QStringLiteral("--- %1 [%2] ---\n")
                  .arg(QDateTime::currentDateTime().toString(Qt::ISODate), tag)
           << body << QStringLiteral("\n\n");
}

QString pythonOutputSnippet(const QByteArray& stdoutOut, const QByteArray& stderrOut)
{
    QStringList parts;
    const QString out = QString::fromUtf8(stdoutOut).trimmed();
    const QString err = QString::fromUtf8(stderrOut).trimmed();
    if (!out.isEmpty())
        parts << QStringLiteral("stdout: %1").arg(out.left(280));
    if (!err.isEmpty())
        parts << QStringLiteral("stderr: %1").arg(err.left(280));
    if (parts.isEmpty())
        return QStringLiteral("(пустой вывод Python — проверьте pymupdf и python_path.txt)");
    return parts.join(QStringLiteral(" | "));
}

void appendPythonRunLog(const QString& tag,
                        const QString& commandLine,
                        const QByteArray& stdoutOut,
                        const QByteArray& stderrOut,
                        int exitCode,
                        bool processStarted)
{
    QString body;
    QTextStream stream(&body);
    stream << QStringLiteral("cmd: ") << commandLine << QLatin1Char('\n');
    stream << QStringLiteral("started: ") << (processStarted ? QStringLiteral("yes")
                                                             : QStringLiteral("no"))
           << QStringLiteral(" exit: ") << exitCode << QLatin1Char('\n');
    if (!stdoutOut.isEmpty())
        stream << QStringLiteral("stdout:\n") << QString::fromUtf8(stdoutOut.left(12000)) << QLatin1Char('\n');
    if (!stderrOut.isEmpty())
        stream << QStringLiteral("stderr:\n") << QString::fromUtf8(stderrOut.left(12000)) << QLatin1Char('\n');
    appendDebugLog(tag, body);
}

QString pythonScriptErrorMessage(const QByteArray& stdoutOut,
                                 const QByteArray& stderrOut,
                                 int exitCode,
                                 const QString& fallback)
{
    const QString logHint =
        QStringLiteral(" Подробности: %1").arg(etemenankiPythonDebugLogPath());

    for (const QByteArray& blob : {stdoutOut, stderrOut}) {
        const QJsonDocument doc = QJsonDocument::fromJson(firstJsonLine(blob));
        if (!doc.isObject())
            continue;
        const QJsonObject obj = doc.object();
        if (obj.contains(QStringLiteral("ok")) && !obj.value(QStringLiteral("ok")).toBool()) {
            QString err = obj.value(QStringLiteral("error")).toString().trimmed();
            if (err.isEmpty() || err == QStringLiteral("Unknown error")) {
                err = QStringLiteral("pymupdf: %1").arg(pythonOutputSnippet(stdoutOut, stderrOut));
            }
            return (err + logHint).left(400);
        }
    }

    const QString stderrText = QString::fromUtf8(stderrOut).trimmed();
    if (!stderrText.isEmpty())
        return (stderrText.left(280) + logHint).left(400);

    const QString snippet = pythonOutputSnippet(stdoutOut, stderrOut);
    if (exitCode >= 0) {
        return QStringLiteral("%1 (код %2) — %3%4")
            .arg(fallback)
            .arg(exitCode)
            .arg(snippet)
            .arg(logHint)
            .left(400);
    }
    return QStringLiteral("%1 — %2%3").arg(fallback, snippet, logHint).left(400);
}

bool copyFileWithReadWrite(const QString& nativeSource, const QString& dest, QString* detailOut)
{
    QFile in(nativeSource);
    if (!in.open(QIODevice::ReadOnly)) {
        if (detailOut)
            *detailOut = QStringLiteral("QFile::open(ReadOnly): %1").arg(in.errorString());
        return false;
    }

    QFile out(dest);
    if (!out.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        if (detailOut)
            *detailOut = QStringLiteral("QFile::open(WriteOnly): %1").arg(out.errorString());
        in.close();
        return false;
    }

    const qint64 written = out.write(in.readAll());
    in.close();
    out.close();

    if (written <= 0) {
        if (detailOut)
            *detailOut = QStringLiteral("read/write: записано 0 байт");
        return false;
    }
    return true;
}

bool robustFileCopy(const QString& nativeSource, const QString& dest, QString* detailOut)
{
#ifdef Q_OS_WIN
    if (::CopyFileW(reinterpret_cast<LPCWSTR>(nativeSource.utf16()),
                    reinterpret_cast<LPCWSTR>(dest.utf16()),
                    FALSE)) {
        return true;
    }

    const DWORD winErr = ::GetLastError();
    if (detailOut) {
        if (winErr == ERROR_SHARING_VIOLATION) {
            *detailOut = QStringLiteral("CopyFileW: файл занят (SHARING_VIOLATION, %1)").arg(winErr);
        } else {
            *detailOut = QStringLiteral("CopyFileW: код %1").arg(winErr);
        }
    }
#endif

    if (QFile::copy(nativeSource, dest))
        return true;

    QString rwDetail;
    if (copyFileWithReadWrite(nativeSource, dest, &rwDetail)) {
        if (detailOut)
            detailOut->clear();
        return true;
    }

    if (detailOut && detailOut->isEmpty())
        *detailOut = rwDetail;
    else if (detailOut && !rwDetail.isEmpty())
        *detailOut += QStringLiteral("; ") + rwDetail;

    return false;
}

bool runProcess(QProcess& proc, int timeoutMs, QByteArray* stdoutOut, QByteArray* stderrOut)
{
    proc.setProcessChannelMode(QProcess::SeparateChannels);

    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert(QStringLiteral("PYTHONIOENCODING"), QStringLiteral("utf-8"));
    env.insert(QStringLiteral("PYTHONUTF8"), QStringLiteral("1"));
    env.insert(QStringLiteral("PYTHONUNBUFFERED"), QStringLiteral("1"));
    env.insert(QStringLiteral("ETE_APP_DIR"),
               QDir::toNativeSeparators(QCoreApplication::applicationDirPath()));

    const QString program = QDir::toNativeSeparators(proc.program());
    if (program.endsWith(QStringLiteral("python.exe"), Qt::CaseInsensitive)
        || program.endsWith(QStringLiteral("python3.exe"), Qt::CaseInsensitive)) {
        const QString binDir = QDir::toNativeSeparators(QFileInfo(program).absolutePath());
        const QString pathKey = QStringLiteral("PATH");
        const QString existing = env.value(pathKey);
        if (!existing.contains(binDir, Qt::CaseInsensitive))
            env.insert(pathKey, binDir + QStringLiteral(";") + existing);

        if (program.contains(QStringLiteral(".venv"), Qt::CaseInsensitive)
            || program.contains(QStringLiteral("\\venv\\"), Qt::CaseInsensitive)) {
            QDir venvRoot(binDir);
            if (venvRoot.cdUp()) {
                const QString venvPath = QDir::toNativeSeparators(venvRoot.absolutePath());
                env.insert(QStringLiteral("VIRTUAL_ENV"), venvPath);

                QFile cfgFile(venvRoot.absoluteFilePath(QStringLiteral("pyvenv.cfg")));
                if (cfgFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
                    while (!cfgFile.atEnd()) {
                        const QString line = QString::fromUtf8(cfgFile.readLine()).trimmed();
                        if (!line.startsWith(QStringLiteral("home"), Qt::CaseInsensitive))
                            continue;
                        const int eq = line.indexOf(QLatin1Char('='));
                        if (eq < 0)
                            break;
                        const QString home = QDir::toNativeSeparators(line.mid(eq + 1).trimmed());
                        if (!home.isEmpty() && QFile::exists(home)) {
                            const QString homeScripts = home + QStringLiteral("/Scripts");
                            QString pathPrefix = home + QStringLiteral(";") + binDir;
                            if (QFile::exists(homeScripts))
                                pathPrefix = home + QStringLiteral(";") + homeScripts + QStringLiteral(";") + binDir;
                            env.insert(pathKey, pathPrefix + QStringLiteral(";") + existing);
                        }
                        break;
                    }
                }
            }
        }
    }

    proc.setProcessEnvironment(env);

    proc.start();
    if (!proc.waitForStarted(30000)) {
        if (stderrOut) {
            const QString err = QStringLiteral("%1 (QProcess::%2)")
                                    .arg(proc.errorString())
                                    .arg(static_cast<int>(proc.error()));
            *stderrOut = err.toUtf8();
        }
        return false;
    }

    QEventLoop loop;
    QTimer killer;
    killer.setSingleShot(true);
    QObject::connect(&proc, &QProcess::finished, &loop, &QEventLoop::quit);
    QObject::connect(&killer, &QTimer::timeout, &loop, [&proc, &loop]() {
        if (proc.state() != QProcess::NotRunning)
            proc.kill();
        loop.quit();
    });
    killer.start(timeoutMs);
    loop.exec();

    if (proc.state() != QProcess::NotRunning) {
        proc.kill();
        proc.waitForFinished(3000);
        if (stderrOut && stderrOut->isEmpty())
            *stderrOut = QByteArrayLiteral("timeout");
        return false;
    }

    if (stdoutOut)
        *stdoutOut = proc.readAllStandardOutput();
    if (stderrOut) {
        const QByteArray errPipe = proc.readAllStandardError();
        if (!errPipe.isEmpty())
            *stderrOut = errPipe;
    }
    return proc.exitStatus() == QProcess::NormalExit;
}

bool runPythonScript(const QString& python,
                     const QString& script,
                     const QStringList& scriptArgs,
                     int timeoutMs,
                     QByteArray* stdoutOut,
                     QByteArray* stderrOut,
                     int* exitCodeOut = nullptr)
{
    const QString pyExe = QDir::toNativeSeparators(QFileInfo(python).absoluteFilePath());
    const QString scriptPath = QDir::toNativeSeparators(QFileInfo(script).absoluteFilePath());
    if (!QFile::exists(pyExe) || !QFile::exists(scriptPath)) {
        if (stderrOut) {
            const QString msg =
                QStringLiteral("missing: %1%2")
                    .arg(!QFile::exists(pyExe) ? pyExe + QStringLiteral(" ") : QString(),
                         !QFile::exists(scriptPath) ? scriptPath : QString());
            *stderrOut = msg.trimmed().toUtf8();
        }
        return false;
    }

    QProcess proc;
    proc.setProgram(pyExe);
    proc.setWorkingDirectory(QFileInfo(scriptPath).absolutePath());

    QStringList args;
    if (python == QStringLiteral("py"))
        args << QStringLiteral("-3");
    args << scriptPath;
    for (const QString& arg : scriptArgs) {
        // Do not use toNativeSeparators on URLs — it turns http:// into http:\\ and breaks Ollama.
        if (arg.startsWith(QStringLiteral("http://"), Qt::CaseInsensitive)
            || arg.startsWith(QStringLiteral("https://"), Qt::CaseInsensitive)) {
            args << arg;
        } else {
            args << QDir::toNativeSeparators(arg);
        }
    }
    proc.setArguments(args);

    const QString commandLine =
        proc.program() + QStringLiteral(" ") + proc.arguments().join(QStringLiteral(" "));

    const bool started = runProcess(proc, timeoutMs, stdoutOut, stderrOut);
    const int exitCode = started ? proc.exitCode() : -1;
    if (exitCodeOut)
        *exitCodeOut = exitCode;

    appendPythonRunLog(QStringLiteral("python"), commandLine,
                       stdoutOut ? *stdoutOut : QByteArray(),
                       stderrOut ? *stderrOut : QByteArray(), exitCode, started);

    return started;
}

bool pythonCanImportDeps(const QString& python, QByteArray* errorOut)
{
    static const char* kCheckScript =
        "import docx,importlib.util,sys;"
        "sys.exit(0 if any(importlib.util.find_spec(m) for m in ('fitz','pypdf','PyPDF2')) else 1)";

    QProcess proc;
    proc.setProgram(QDir::toNativeSeparators(python));
    if (python == QStringLiteral("py"))
        proc.setArguments({QStringLiteral("-3"), QStringLiteral("-c"), QString::fromUtf8(kCheckScript)});
    else
        proc.setArguments({QStringLiteral("-c"), QString::fromUtf8(kCheckScript)});

    QByteArray stderrOut;
    if (!runProcess(proc, 30000, nullptr, &stderrOut))
        return false;
    if (proc.exitCode() != 0) {
        if (errorOut)
            *errorOut = stderrOut;
        return false;
    }
    return true;
}

bool installPythonDeps(const QString& python, QByteArray* logOut)
{
    QProcess proc;
    proc.setProgram(python);
    if (python == QStringLiteral("py")) {
            proc.setArguments({QStringLiteral("-3"), QStringLiteral("-m"), QStringLiteral("pip"),
                           QStringLiteral("install"), QStringLiteral("-q"), QStringLiteral("PyPDF2"),
                           QStringLiteral("pypdf"), QStringLiteral("pymupdf"),
                           QStringLiteral("pymupdf-fonts"), QStringLiteral("python-docx"),
                           QStringLiteral("openpyxl")});
    } else {
        proc.setArguments({QStringLiteral("-m"), QStringLiteral("pip"), QStringLiteral("install"),
                           QStringLiteral("-q"), QStringLiteral("PyPDF2"), QStringLiteral("pypdf"),
                           QStringLiteral("pymupdf"), QStringLiteral("pymupdf-fonts"),
                           QStringLiteral("pymupdf-fonts"), QStringLiteral("python-docx"),
                           QStringLiteral("openpyxl")});
    }

    QByteArray stderrOut;
    QByteArray stdoutOut;
    if (!runProcess(proc, 180000, &stdoutOut, &stderrOut))
        return false;
    if (proc.exitCode() != 0) {
        if (logOut)
            *logOut = stderrOut.isEmpty() ? stdoutOut : stderrOut;
        return false;
    }
    return true;
}

QString decodeTextBytes(const QByteArray& raw, QString* encodingOut)
{
    if (raw.startsWith("\xEF\xBB\xBF")) {
        if (encodingOut) *encodingOut = QStringLiteral("utf-8-sig");
        return QString::fromUtf8(raw.mid(3));
    }
    if (raw.size() >= 2 && raw.startsWith("\xFF\xFE")) {
        if (encodingOut) *encodingOut = QStringLiteral("utf-16-le");
        return QStringDecoder(QStringDecoder::Utf16LE).decode(raw.mid(2));
    }
    if (raw.size() >= 2 && raw.startsWith("\xFE\xFF")) {
        if (encodingOut) *encodingOut = QStringLiteral("utf-16-be");
        return QStringDecoder(QStringDecoder::Utf16BE).decode(raw.mid(2));
    }

    QStringDecoder utf8(QStringDecoder::Utf8);
    const QString asUtf8 = utf8.decode(raw);
    if (!utf8.hasError()) {
        if (encodingOut) *encodingOut = QStringLiteral("utf-8");
        return asUtf8;
    }

    for (const char* name : {"windows-1251", "KOI8-R", "ISO-8859-5", "ISO-8859-1"}) {
        QStringDecoder dec(name);
        const QString out = dec.decode(raw);
        if (!dec.hasError()) {
            if (encodingOut) *encodingOut = QString::fromLatin1(name);
            return out;
        }
    }

    if (encodingOut) *encodingOut = QStringLiteral("utf-8-replace");
    return QString::fromUtf8(raw);
}

QString findPdfLayoutScript()
{
    const QString appDir = QCoreApplication::applicationDirPath();
    QStringList candidates = {
        appDir + QStringLiteral("/tools/pdf_layout.py"),
    };

    for (const QString& root : ancestorDirs()) {
        candidates << root + QStringLiteral("/tools/pdf_layout.py");
    }

#if defined(ETE_PROJECT_SOURCE_DIR)
    candidates << QString::fromUtf8(ETE_PROJECT_SOURCE_DIR) + QStringLiteral("/tools/pdf_layout.py");
#endif

    for (const QString& path : candidates) {
        if (QFile::exists(path))
            return QDir::toNativeSeparators(path);
    }
    return candidates.value(0);
}

QString findPdfEnginesScript()
{
    const QString appDir = QCoreApplication::applicationDirPath();
    QStringList candidates = {
        appDir + QStringLiteral("/tools/pdf_engines.py"),
    };

    for (const QString& root : ancestorDirs()) {
        candidates << root + QStringLiteral("/tools/pdf_engines.py");
    }

#if defined(ETE_PROJECT_SOURCE_DIR)
    candidates << QString::fromUtf8(ETE_PROJECT_SOURCE_DIR) + QStringLiteral("/tools/pdf_engines.py");
#endif

    for (const QString& path : candidates) {
        if (QFile::exists(path))
            return QDir::toNativeSeparators(path);
    }
    return candidates.value(0);
}

QString layoutPagesToPlainText(const QJsonObject& layout)
{
    QStringList parts;
    const QJsonArray pages = layout.value(QStringLiteral("pages")).toArray();
    for (const QJsonValue& pageValue : pages) {
        const QJsonObject page = pageValue.toObject();
        QStringList pageLines;
        for (const QJsonValue& itemValue : page.value(QStringLiteral("items")).toArray()) {
            const QString text = itemValue.toObject().value(QStringLiteral("text")).toString().trimmed();
            if (!text.isEmpty())
                pageLines << text;
        }
        for (const QJsonValue& tableValue : page.value(QStringLiteral("tables")).toArray()) {
            for (const QJsonValue& cellValue :
                 tableValue.toObject().value(QStringLiteral("cells")).toArray()) {
                const QString text = cellValue.toObject().value(QStringLiteral("text")).toString().trimmed();
                if (!text.isEmpty())
                    pageLines << text;
            }
        }
        parts << pageLines.join(QStringLiteral("\n"));
    }
    return parts.join(QStringLiteral("\n\n"));
}

DocumentLoadResult runPdfLayoutExtract(const QString& python,
                                       const QString& script,
                                       const QString& filePath)
{
    DocumentLoadResult result;

    QByteArray stdoutOut;
    QByteArray stderrOut;
    int exitCode = -1;
    if (!runPythonScript(python, script, {QStringLiteral("extract"), filePath}, 180000, &stdoutOut,
                         &stderrOut, &exitCode)) {
        const QString detail = QString::fromUtf8(stderrOut).trimmed();
        result.error = detail.isEmpty()
            ? QStringLiteral("Не удалось запустить pdf_layout.py")
            : detail.left(220);
        return result;
    }

    if (exitCode != 0) {
        result.error = pythonScriptErrorMessage(stdoutOut, stderrOut, exitCode,
                                               QStringLiteral("Ошибка pdf_layout.py"));
        return result;
    }

    const QJsonDocument doc = QJsonDocument::fromJson(firstJsonLine(stdoutOut));
    if (!doc.isObject()) {
        result.error = QStringLiteral("Некорректный JSON от extract: %1. Лог: %2")
                           .arg(pythonOutputSnippet(stdoutOut, stderrOut), etemenankiPythonDebugLogPath());
        return result;
    }

    const QJsonObject obj = doc.object();
    if (!obj.value(QStringLiteral("ok")).toBool()) {
        result.error = obj.value(QStringLiteral("error")).toString();
        if (result.error.isEmpty())
            result.error = QStringLiteral("Не удалось извлечь layout PDF");
        return result;
    }

    const QJsonObject layout = obj.value(QStringLiteral("pdf_layout")).toObject();
    const QJsonArray pagesArr = layout.value(QStringLiteral("pages")).toArray();
    if (pagesArr.isEmpty()) {
        result.error = QStringLiteral("PDF layout пуст");
        return result;
    }

    result.ok = true;
    result.pdfLayout = layout;
    result.encoding = QStringLiteral("pdf-layout");
    result.pageCount = pagesArr.size();
    result.text = layoutPagesToPlainText(layout);

    for (const QJsonValue& pageValue : pagesArr) {
        const QJsonObject page = pageValue.toObject();
        QStringList pageLines;
        for (const QJsonValue& itemValue : page.value(QStringLiteral("items")).toArray()) {
            const QString text = itemValue.toObject().value(QStringLiteral("text")).toString().trimmed();
            if (!text.isEmpty())
                pageLines << text;
        }
        for (const QJsonValue& tableValue : page.value(QStringLiteral("tables")).toArray()) {
            for (const QJsonValue& cellValue :
                 tableValue.toObject().value(QStringLiteral("cells")).toArray()) {
                const QString text = cellValue.toObject().value(QStringLiteral("text")).toString().trimmed();
                if (!text.isEmpty())
                    pageLines << text;
            }
        }
        result.pages << pageLines.join(QStringLiteral("\n"));
    }
    if (result.pages.isEmpty())
        result.pages << result.text;
    return result;
}

DocumentLoadResult loadPdfQt(const QString& filePath)
{
    DocumentLoadResult result;

    QPdfDocument pdf;
    pdf.load(filePath);

    if (pdf.status() != QPdfDocument::Status::Ready) {
        result.error = QStringLiteral("Qt PDF: не удалось открыть файл");
        return result;
    }

    QStringList pages;
    pages.reserve(pdf.pageCount());
    for (int i = 0; i < pdf.pageCount(); ++i) {
        const QPdfSelection selection = pdf.getAllText(i);
        pages << (selection.isValid() ? selection.text().trimmed() : QString());
    }
    if (pages.isEmpty())
        pages << QString();

    QStringList nonEmpty;
    nonEmpty.reserve(pages.size());
    for (const QString& page : pages) {
        const QString trimmed = page.trimmed();
        if (!trimmed.isEmpty())
            nonEmpty << trimmed;
    }

    if (nonEmpty.isEmpty()) {
        result.error = QStringLiteral("Нет текстового слоя в PDF (скан без OCR)");
        return result;
    }

    result.ok = true;
    result.pages = pages;
    result.pageCount = pages.size();
    result.text = nonEmpty.join(QStringLiteral("\n\n"));
    result.encoding = QStringLiteral("qtpdf");
    return result;
}

DocumentLoadResult loadPlainText(const QString& filePath)
{
    DocumentLoadResult result;
    QFile f(filePath);
    if (!f.open(QIODevice::ReadOnly)) {
        result.error = QStringLiteral("Не удалось открыть файл");
        return result;
    }
    const QByteArray raw = f.readAll();
    f.close();

    QString encoding;
    const QString text = decodeTextBytes(raw, &encoding);
    result.ok = true;
    result.encoding = encoding;
    result.text = text;
    result.pages = {text};
    result.pageCount = 1;
    return result;
}

DocumentLoadResult runExtractScript(const QString& python,
                                    const QString& script,
                                    const QString& filePath,
                                    const QString& extractFlag = {})
{
    DocumentLoadResult result;

    QProcess proc;
    proc.setProgram(python);
    QStringList args;
    if (python == QStringLiteral("py"))
        args << QStringLiteral("-3");
    args << script;
    if (!extractFlag.isEmpty())
        args << extractFlag;
    args << filePath;
    proc.setArguments(args);

    QByteArray stdoutOut;
    QByteArray stderrOut;
    if (!runProcess(proc, 180000, &stdoutOut, &stderrOut)) {
        result.error = QStringLiteral("Не удалось запустить Python для извлечения текста");
        return result;
    }

    if (proc.exitCode() != 0) {
        const QString errText = QString::fromUtf8(stderrOut.isEmpty() ? stdoutOut : stderrOut).trimmed();
        result.error = errText.isEmpty()
            ? QStringLiteral("Ошибка извлечения текста (код %1)").arg(proc.exitCode())
            : errText;
        return result;
    }

    const QJsonDocument doc = QJsonDocument::fromJson(stdoutOut);
    if (!doc.isObject()) {
        result.error = QStringLiteral("Некорректный ответ скрипта извлечения");
        return result;
    }

    const QJsonObject obj = doc.object();
    if (!obj.value(QStringLiteral("ok")).toBool()) {
        result.error = obj.value(QStringLiteral("error")).toString();
        if (result.error.isEmpty())
            result.error = QStringLiteral("Не удалось извлечь текст из документа");
        return result;
    }

    result.ok = true;
    result.text = obj.value(QStringLiteral("text")).toString();
    result.encoding = obj.value(QStringLiteral("encoding")).toString();
    result.pageCount = obj.value(QStringLiteral("page_count")).toInt(1);

    const QJsonArray pagesArr = obj.value(QStringLiteral("pages")).toArray();
    for (const QJsonValue& v : pagesArr)
        result.pages << v.toString();
    if (result.pages.isEmpty())
        result.pages << result.text;

    const QJsonArray blocksArr = obj.value(QStringLiteral("page_blocks")).toArray();
    for (const QJsonValue& pageValue : blocksArr)
        result.pageBlocks << pageValue.toArray();

    if (result.pageCount <= 0)
        result.pageCount = result.pages.size();

    result.workflowId = obj.value(QStringLiteral("workflow_id")).toString();
    if (result.workflowId.isEmpty()) {
        const WorkflowInfo info = TranslationWorkflow::forSuffix(QFileInfo(filePath).suffix());
        result.workflowId = info.id;
    }
    result.workflowMeta = obj.value(QStringLiteral("workflow_meta")).toObject();
    return result;
}

} // namespace

QString DocumentLoader::activePythonPath()
{
    return cachedPython();
}

QString DocumentLoader::cachedPython()
{
    QMutexLocker lock(&g_cacheMutex);
    return g_pythonPath;
}

QString DocumentLoader::cachedScript()
{
    QMutexLocker lock(&g_cacheMutex);
    return g_scriptPath;
}

void DocumentLoader::setCache(const QString& python, const QString& script)
{
    QMutexLocker lock(&g_cacheMutex);
    g_pythonPath = python;
    g_scriptPath = script;
}

ExtractRuntimeInfo DocumentLoader::probe()
{
    ExtractRuntimeInfo info;
    info.scriptPath = findExtractScript();

    if (!QFile::exists(info.scriptPath)) {
        info.message = QStringLiteral("Не найден tools/extract_document.py");
        return info;
    }

    {
        QMutexLocker lock(&g_cacheMutex);
        if (!g_pythonPath.isEmpty() && pythonCanImportDeps(g_pythonPath, nullptr)) {
            info.pythonPath = g_pythonPath;
            info.ready = true;
            info.message = QStringLiteral("PDF/DOCX: готово");
            return info;
        }
    }

    QByteArray lastError;
    for (const QString& python : pythonSearchPaths()) {
        if (python != QStringLiteral("py") && python != QStringLiteral("python")
            && python != QStringLiteral("python3") && !QFile::exists(python)) {
            continue;
        }

        QByteArray importError;
        if (!pythonCanImportDeps(python, &importError)) {
            if (installPythonDeps(python, &importError) && pythonCanImportDeps(python, nullptr)) {
                setCache(python, info.scriptPath);
                info.pythonPath = python;
                info.ready = true;
                info.message = QStringLiteral("PDF/DOCX: зависимости установлены");
                return info;
            }
            lastError = importError;
            continue;
        }

        setCache(python, info.scriptPath);
        info.pythonPath = python;
        info.ready = true;
        info.message = QStringLiteral("PDF/DOCX: готово");
        return info;
    }

    const QString err = QString::fromUtf8(lastError).trimmed();
    info.message = err.isEmpty()
        ? QStringLiteral("Установите Python 3 и PyPDF2, python-docx")
        : QStringLiteral("Python: ") + err.left(200);
    return info;
}

bool DocumentLoader::ensureReady()
{
    const QString script = findExtractScript();
    if (!QFile::exists(script))
        return false;

    {
        QMutexLocker lock(&g_cacheMutex);
        if (!g_pythonPath.isEmpty() && QFile::exists(g_pythonPath))
            return true;
    }

    if (primePythonCache(isMainThread()))
        return true;

    return primePythonCache(false);
}

DocumentLoadResult DocumentLoader::extractPdfLayout(const QString& filePath,
                                                    const QString& prebuiltWorkPath)
{
    DocumentLoadResult result;
    if (!ensureReady()) {
        result.error = probe().message;
        return result;
    }

    const QString layoutScript = findPdfLayoutScript();
    if (!QFile::exists(layoutScript)) {
        result.error = QStringLiteral("Не найден tools/pdf_layout.py");
        return result;
    }

    const QString workPath = !prebuiltWorkPath.isEmpty() && QFile::exists(prebuiltWorkPath)
        ? prebuiltWorkPath
        : copyPdfForPythonWork(filePath);
    if (workPath.isEmpty()) {
        result.error = QStringLiteral("Не удалось скопировать PDF (закройте файл в других программах)");
        return result;
    }

    return runPdfLayoutExtract(cachedPython(), layoutScript, workPath);
}

QString DocumentLoader::copyPdfForPythonWork(const QString& sourcePath)
{
    const QFileInfo info(sourcePath);
    if (!info.exists() || !info.isFile())
        return {};

    const QString dest = QDir::toNativeSeparators(
        etemenankiDebugDir() + QStringLiteral("/work_")
        + QUuid::createUuid().toString(QUuid::WithoutBraces) + QStringLiteral(".pdf"));
    const QString nativeSource = QDir::toNativeSeparators(info.absoluteFilePath());

    QString copyDetail;
    if (!robustFileCopy(nativeSource, dest, &copyDetail)) {
        appendDebugLog(QStringLiteral("copy-fail"),
                       QStringLiteral("src: %1\ndest: %2\n%3")
                           .arg(nativeSource, dest, copyDetail));
        return {};
    }

    const QFileInfo destInfo(dest);
    if (!destInfo.exists() || destInfo.size() != info.size()) {
        appendDebugLog(QStringLiteral("copy-size-mismatch"),
                       QStringLiteral("src: %1 (%2 bytes)\ndest: %3 (%4 bytes)")
                           .arg(nativeSource)
                           .arg(info.size())
                           .arg(dest)
                           .arg(destInfo.size()));
        QFile::remove(dest);
        return {};
    }

    appendDebugLog(QStringLiteral("copy-ok"),
                   QStringLiteral("src: %1 (%2 bytes)\ndest: %3")
                       .arg(nativeSource)
                       .arg(info.size())
                       .arg(dest));
    return dest;
}

QString DocumentLoader::pythonDebugLogPath()
{
    return etemenankiPythonDebugLogPath();
}

DocumentLoadResult DocumentLoader::load(const QString& filePath)
{
    const QFileInfo info(filePath);
    const QString suffix = info.suffix().toLower();

    if (suffix == QStringLiteral("txt") || suffix == QStringLiteral("md"))
        return loadPlainText(filePath);

    if (suffix == QStringLiteral("pdf")) {
        DocumentLoadResult result = loadPdfQt(filePath);
        if (result.ok) {
            if (result.encoding.isEmpty())
                result.encoding = QStringLiteral("qt-pdf");
            return result;
        }

        if (!ensureReady()) {
            if (result.error.isEmpty())
                result.error = probe().message;
            return result;
        }

        return extractPdfLayout(filePath);
    }

    if (TranslationWorkflow::isHubExtractSuffix(suffix)) {
        if (!ensureReady()) {
            DocumentLoadResult result;
            result.error = probe().message;
            return result;
        }

        DocumentLoadResult result = runExtractScript(cachedPython(), cachedScript(), filePath, {});
        if (!result.ok) {
            setCache(QString(), QString());
            if (ensureReady())
                result = runExtractScript(cachedPython(), cachedScript(), filePath, {});
        }
        return result;
    }

    DocumentLoadResult result;
    result.error = QStringLiteral("Неподдерживаемый формат: .") + suffix;
    return result;
}

void mergeTranslatedPagesIntoLayout(QJsonObject& layout, const QStringList& translatedPages)
{
    QJsonArray pages = layout.value(QStringLiteral("pages")).toArray();
    for (int pageIndex = 0; pageIndex < pages.size(); ++pageIndex) {
        if (pageIndex >= translatedPages.size())
            break;

        QJsonObject page = pages.at(pageIndex).toObject();
        int translatedCount = 0;
        for (const QJsonValue& value : page.value(QStringLiteral("items")).toArray()) {
            if (!value.toObject().value(QStringLiteral("text_translated")).toString().trimmed().isEmpty())
                ++translatedCount;
        }
        for (const QJsonValue& tableValue : page.value(QStringLiteral("tables")).toArray()) {
            for (const QJsonValue& cellValue :
                 tableValue.toObject().value(QStringLiteral("cells")).toArray()) {
                if (!cellValue.toObject().value(QStringLiteral("text_translated")).toString().trimmed().isEmpty())
                    ++translatedCount;
            }
        }
        if (translatedCount > 0)
            continue;

        const QString pageText = translatedPages.at(pageIndex).trimmed();
        if (pageText.isEmpty())
            continue;

        const QStringList plainLines = pageText.split(QLatin1Char('\n'), Qt::SkipEmptyParts);
        int lineIdx = 0;

        QJsonArray updatedItems;
        for (const QJsonValue& value : page.value(QStringLiteral("items")).toArray()) {
            QJsonObject item = value.toObject();
            const QString source = item.value(QStringLiteral("text")).toString().trimmed();
            if (source.isEmpty()) {
                updatedItems.append(item);
                continue;
            }
            if (lineIdx < plainLines.size())
                item.insert(QStringLiteral("text_translated"), plainLines.at(lineIdx++));
            else
                item.insert(QStringLiteral("text_translated"), source);
            updatedItems.append(item);
        }

        QJsonArray updatedTables;
        for (const QJsonValue& tableValue : page.value(QStringLiteral("tables")).toArray()) {
            QJsonObject table = tableValue.toObject();
            QJsonArray updatedCells;
            for (const QJsonValue& cellValue : table.value(QStringLiteral("cells")).toArray()) {
                QJsonObject cell = cellValue.toObject();
                const QString source = cell.value(QStringLiteral("text")).toString().trimmed();
                if (source.isEmpty()) {
                    updatedCells.append(cell);
                    continue;
                }
                if (lineIdx < plainLines.size())
                    cell.insert(QStringLiteral("text_translated"), plainLines.at(lineIdx++));
                else
                    cell.insert(QStringLiteral("text_translated"), source);
                updatedCells.append(cell);
            }
            table.insert(QStringLiteral("cells"), updatedCells);
            updatedTables.append(table);
        }

        page.insert(QStringLiteral("items"), updatedItems);
        page.insert(QStringLiteral("tables"), updatedTables);
        pages[pageIndex] = page;
    }
    layout.insert(QStringLiteral("pages"), pages);
}

PdfBuildResult DocumentLoader::buildTranslatedPdfFromPages(const QString& sourcePath,
                                                           const QJsonObject& layoutIn,
                                                           const QStringList& translatedPages,
                                                           const QString& outputPath,
                                                           const QString& prebuiltWorkPath)
{
    const QString workSource = !prebuiltWorkPath.isEmpty() && QFile::exists(prebuiltWorkPath)
        ? prebuiltWorkPath
        : copyPdfForPythonWork(sourcePath);
    if (workSource.isEmpty()) {
        PdfBuildResult result;
        result.error = QStringLiteral("Не удалось скопировать PDF для сборки");
        return result;
    }

    QJsonObject layout = layoutIn;
    if (layout.isEmpty()) {
        const DocumentLoadResult extracted = extractPdfLayout(workSource, workSource);
        if (!extracted.ok || extracted.pdfLayout.isEmpty()) {
            PdfBuildResult result;
            result.error = extracted.error.isEmpty()
                ? QStringLiteral("Не удалось извлечь структуру PDF")
                : extracted.error;
            return result;
        }
        layout = extracted.pdfLayout;
    }

    mergeTranslatedPagesIntoLayout(layout, translatedPages);
    PdfBuildResult result = buildTranslatedPdf(workSource, layout, outputPath);
    if (result.ok)
        result.layout = layout;
    return result;
}

PdfBuildResult DocumentLoader::buildTranslatedPdf(const QString& sourcePath,
                                                  const QJsonObject& layout,
                                                  const QString& outputPath)
{
    PdfBuildResult result;
    if (!ensureReady())
        primePythonCache(false);

    const QString python = cachedPython().isEmpty() ? resolvePythonPath() : cachedPython();
    if (python.isEmpty() || !QFile::exists(python)) {
        result.error = QStringLiteral("Python не найден (tools/python_path.txt)");
        return result;
    }

    {
        QMutexLocker lock(&g_cacheMutex);
        g_pythonPath = python;
        if (g_scriptPath.isEmpty())
            g_scriptPath = findExtractScript();
    }

    const QString layoutScript = findPdfLayoutScript();
    if (!QFile::exists(layoutScript)) {
        result.error = QStringLiteral("Не найден tools/pdf_layout.py");
        return result;
    }

    const QString layoutDir =
        QStandardPaths::writableLocation(QStandardPaths::TempLocation) + QStringLiteral("/Etemenanki");
    QDir().mkpath(layoutDir);
    const QString layoutPath =
        layoutDir + QLatin1Char('/') + QUuid::createUuid().toString(QUuid::WithoutBraces) + QStringLiteral("_layout.json");

    QFile layoutFile(layoutPath);
    if (!layoutFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        result.error = QStringLiteral("Не удалось создать временный layout");
        return result;
    }
    layoutFile.write(QJsonDocument(layout).toJson(QJsonDocument::Compact));
    layoutFile.close();

    QByteArray stdoutOut;
    QByteArray stderrOut;
    int exitCode = -1;
    if (!runPythonScript(python, layoutScript,
                         {QStringLiteral("build"), sourcePath, layoutPath, outputPath}, 180000,
                         &stdoutOut, &stderrOut, &exitCode)) {
        QFile::remove(layoutPath);
        result.error = pythonScriptErrorMessage(stdoutOut, stderrOut, -1,
                                               QStringLiteral("Не удалось запустить pdf_layout.py"));
        return result;
    }

    QFile::remove(layoutPath);

    if (exitCode != 0) {
        QFile::remove(layoutPath);
        result.error = pythonScriptErrorMessage(stdoutOut, stderrOut, exitCode,
                                               QStringLiteral("Ошибка сборки PDF"));
        return result;
    }

    const QJsonDocument doc = QJsonDocument::fromJson(firstJsonLine(stdoutOut));
    if (!doc.isObject()) {
        result.error = QStringLiteral("Некорректный JSON от build: %1. Лог: %2")
                           .arg(pythonOutputSnippet(stdoutOut, stderrOut), etemenankiPythonDebugLogPath());
        return result;
    }

    if (!doc.object().value(QStringLiteral("ok")).toBool()) {
        result.error = pythonScriptErrorMessage(stdoutOut, stderrOut, exitCode,
                                               QStringLiteral("Ошибка сборки PDF"));
        return result;
    }

    if (!QFile::exists(outputPath)) {
        result.error = QStringLiteral("PDF не создан (Python ok, файла нет). Лог: %1")
                           .arg(etemenankiPythonDebugLogPath());
        return result;
    }

    result.ok = true;
    result.outputPath = outputPath;
    return result;
}

ExportDocumentResult DocumentLoader::exportDocument(const QString& sourcePath,
                                                    const QString& outputPath,
                                                    const QString& workflowId,
                                                    const QJsonObject& workflowMeta)
{
    ExportDocumentResult result;
    if (!ensureReady()) {
        result.error = probe().message;
        return result;
    }

    const QString python = cachedPython();
    const QString script = findExportScript();
    if (script.isEmpty() || !QFile::exists(script)) {
        result.error = QStringLiteral("Не найден tools/export_document.py");
        return result;
    }

    const QString tempDir =
        QStandardPaths::writableLocation(QStandardPaths::TempLocation) + QStringLiteral("/Etemenanki");
    QDir().mkpath(tempDir);
    const QString metaPath =
        tempDir + QLatin1Char('/') + QUuid::createUuid().toString(QUuid::WithoutBraces) + QStringLiteral("_meta.json");

    QJsonObject envelope;
    envelope.insert(QStringLiteral("workflow_id"), workflowId);
    envelope.insert(QStringLiteral("workflow_meta"), workflowMeta);
    envelope.insert(QStringLiteral("source_path"), sourcePath);
    envelope.insert(QStringLiteral("output_path"), outputPath);

    QFile metaFile(metaPath);
    if (!metaFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        result.error = QStringLiteral("Не удалось подготовить метаданные экспорта");
        return result;
    }
    metaFile.write(QJsonDocument(envelope).toJson(QJsonDocument::Compact));
    metaFile.close();

    QProcess proc;
    proc.setProgram(python);
    QStringList args;
    if (python == QStringLiteral("py"))
        args << QStringLiteral("-3");
    args << script << metaPath;
    proc.setArguments(args);

    QByteArray stdoutOut;
    QByteArray stderrOut;
    if (!runProcess(proc, 180000, &stdoutOut, &stderrOut)) {
        result.error = QStringLiteral("Не удалось запустить Python для экспорта");
        return result;
    }

    const QJsonDocument doc = QJsonDocument::fromJson(firstJsonLine(stdoutOut));
    if (doc.isObject()) {
        const QJsonObject obj = doc.object();
        if (obj.value(QStringLiteral("ok")).toBool()) {
            result.ok = true;
            result.outputPath = obj.value(QStringLiteral("output_path")).toString(outputPath);
            return result;
        }
        result.error = obj.value(QStringLiteral("error")).toString();
        if (result.error.isEmpty())
            result.error = QStringLiteral("Ошибка экспорта документа");
        return result;
    }

    if (proc.exitCode() != 0) {
        result.error = QString::fromUtf8(stderrOut.isEmpty() ? stdoutOut : stderrOut).trimmed();
        if (result.error.isEmpty())
            result.error = QStringLiteral("Ошибка экспорта (код %1)").arg(proc.exitCode());
        return result;
    }

    if (QFile::exists(outputPath)) {
        result.ok = true;
        result.outputPath = outputPath;
        return result;
    }

    result.error = QStringLiteral("Экспорт не создал файл");
    return result;
}

QVariantMap DocumentLoader::probePdfEngines()
{
    QVariantMap out;
    if (!ensureReady())
        primePythonCache(false);

    const QString python = cachedPython().isEmpty() ? resolvePythonPath() : cachedPython();
    const QString script = findPdfEnginesScript();
    if (python.isEmpty() || !QFile::exists(python) || !QFile::exists(script)) {
        out.insert(QStringLiteral("ok"), false);
        out.insert(QStringLiteral("error"), QStringLiteral("Python или tools/pdf_engines.py недоступны"));
        return out;
    }

    QByteArray stdoutOut;
    QByteArray stderrOut;
    int exitCode = -1;
    const QStringList args = {
        QStringLiteral("probe"),
        QStringLiteral("--engine"),
        QStringLiteral("all"),
    };
    if (!runPythonScript(python, script, args, 60000, &stdoutOut, &stderrOut, &exitCode)) {
        out.insert(QStringLiteral("ok"), false);
        out.insert(QStringLiteral("error"), QStringLiteral("Не удалось запустить pdf_engines.py probe"));
        return out;
    }

    const QJsonDocument doc = QJsonDocument::fromJson(firstJsonLine(stdoutOut));
    if (!doc.isObject()) {
        out.insert(QStringLiteral("ok"), false);
        out.insert(QStringLiteral("error"), pythonScriptErrorMessage(stdoutOut, stderrOut, exitCode,
                                                                     QStringLiteral("probe PDF engines")));
        return out;
    }

    const QJsonObject obj = doc.object();
    out.insert(QStringLiteral("ok"), obj.value(QStringLiteral("ok")).toBool());
    const QJsonObject engines = obj.value(QStringLiteral("engines")).toObject();
    QVariantMap engineMap;
    for (auto it = engines.begin(); it != engines.end(); ++it) {
        const QJsonObject entry = it.value().toObject();
        engineMap.insert(it.key(), QVariantMap{
                                       {QStringLiteral("available"), entry.value(QStringLiteral("available")).toBool()},
                                       {QStringLiteral("message"), entry.value(QStringLiteral("message")).toString()},
                                   });
    }
    out.insert(QStringLiteral("engines"), engineMap);
    return out;
}

ExternalPdfTranslateResult DocumentLoader::translatePdfExternal(const ExternalPdfTranslateRequest& request)
{
    ExternalPdfTranslateResult result;
    if (request.engine == QStringLiteral("etemenanki")) {
        result.error = QStringLiteral("Встроенный движок использует стандартный пайплайн Etemenanki");
        return result;
    }

    if (!ensureReady())
        primePythonCache(false);

    const QString python = cachedPython().isEmpty() ? resolvePythonPath() : cachedPython();
    const QString script = findPdfEnginesScript();
    if (python.isEmpty() || !QFile::exists(python) || !QFile::exists(script)) {
        result.error = QStringLiteral("Python или tools/pdf_engines.py недоступны");
        return result;
    }

    QDir().mkpath(QFileInfo(request.outputPath).absolutePath());

    const QStringList args = {
        QStringLiteral("translate"),
        QStringLiteral("--engine"),
        request.engine,
        QStringLiteral("--input"),
        request.inputPath,
        QStringLiteral("--output"),
        request.outputPath,
        QStringLiteral("--src-lang"),
        request.srcLang,
        QStringLiteral("--dst-lang"),
        request.dstLang,
        QStringLiteral("--runtime"),
        request.runtime,
        QStringLiteral("--model"),
        request.model,
        QStringLiteral("--ollama-url"),
        request.ollamaUrl,
        QStringLiteral("--cloud-base"),
        request.cloudBase,
        QStringLiteral("--cloud-key"),
        request.cloudKey,
    };

    QByteArray stdoutOut;
    QByteArray stderrOut;
    int exitCode = -1;
    const int timeoutMs = 7200000;
    if (!runPythonScript(python, script, args, timeoutMs, &stdoutOut, &stderrOut, &exitCode)) {
        result.error = QStringLiteral("Не удалось запустить внешний PDF-движок");
        return result;
    }

    const QJsonDocument doc = QJsonDocument::fromJson(firstJsonLine(stdoutOut));
    if (!doc.isObject()) {
        result.error = pythonScriptErrorMessage(stdoutOut, stderrOut, exitCode,
                                               QStringLiteral("Внешний PDF-движок"));
        return result;
    }

    const QJsonObject obj = doc.object();
    if (!obj.value(QStringLiteral("ok")).toBool()) {
        result.error = obj.value(QStringLiteral("error")).toString();
        if (result.error.isEmpty())
            result.error = pythonScriptErrorMessage(stdoutOut, stderrOut, exitCode,
                                                   QStringLiteral("Внешний PDF-движок"));
        return result;
    }

    const QString output = obj.value(QStringLiteral("output_path")).toString(request.outputPath);
    if (!QFile::exists(output)) {
        result.error = QStringLiteral("Внешний движок не создал выходной PDF");
        return result;
    }

    result.ok = true;
    result.outputPath = output;
    return result;
}

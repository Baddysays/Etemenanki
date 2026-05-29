#include "external_pdf_translate_worker.h"

#include "document_loader.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTimer>

namespace {

QStringList buildProcessArguments(const QString& python,
                                  const QString& script,
                                  const QStringList& scriptArgs)
{
    QStringList args;
    if (python == QStringLiteral("py"))
        args << QStringLiteral("-3");
    args << QDir::toNativeSeparators(QFileInfo(script).absoluteFilePath());
    for (const QString& arg : scriptArgs) {
        if (arg.startsWith(QStringLiteral("http://"), Qt::CaseInsensitive)
            || arg.startsWith(QStringLiteral("https://"), Qt::CaseInsensitive)) {
            args << arg;
        } else {
            args << QDir::toNativeSeparators(arg);
        }
    }
    return args;
}

} // namespace

ExternalPdfTranslateWorker::ExternalPdfTranslateWorker(ExternalPdfTranslateRequest request,
                                                       QObject* parent)
    : QObject(parent)
    , m_request(std::move(request))
{
}

void ExternalPdfTranslateWorker::start()
{
    const ExternalPdfTranslateCommand cmd =
        DocumentLoader::buildExternalPdfTranslateCommand(m_request);
    if (cmd.python.isEmpty() || !QFile::exists(cmd.python) || !QFile::exists(cmd.script)) {
        ExternalPdfTranslateResult result;
        result.error = QStringLiteral("Python или tools/pdf_engines.py недоступны");
        emit finished(result);
        return;
    }

    QDir().mkpath(QFileInfo(m_request.outputPath).absolutePath());

    m_proc = new QProcess(this);
    m_proc->setProgram(QDir::toNativeSeparators(QFileInfo(cmd.python).absoluteFilePath()));
    m_proc->setWorkingDirectory(QFileInfo(cmd.script).absolutePath());
    m_proc->setArguments(buildProcessArguments(cmd.python, cmd.script, cmd.args));
    m_proc->setProcessChannelMode(QProcess::SeparateChannels);
    DocumentLoader::configurePythonProcess(*m_proc, cmd.extraEnv);

    connect(m_proc, &QProcess::readyReadStandardError, this,
            &ExternalPdfTranslateWorker::onReadyReadStandardError);
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    connect(m_proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this,
            &ExternalPdfTranslateWorker::onProcessFinished);
#else
    connect(m_proc, static_cast<void (QProcess::*)(int)>(&QProcess::finished), this,
            [this](int code) { onProcessFinished(code, QProcess::NormalExit); });
#endif

    m_proc->start();
    if (!m_proc->waitForStarted(30000)) {
        ExternalPdfTranslateResult result;
        result.error = QStringLiteral("Не удалось запустить внешний PDF-движок: %1")
                           .arg(m_proc->errorString());
        emit finished(result);
        m_proc->deleteLater();
        m_proc = nullptr;
        return;
    }

    auto* killer = new QTimer(this);
    killer->setSingleShot(true);
    killer->setInterval(kExternalPdfTranslateTimeoutMs);
    connect(killer, &QTimer::timeout, this, [this, killer]() {
        if (m_proc && m_proc->state() != QProcess::NotRunning) {
            m_proc->kill();
            m_cancelled = true;
        }
        killer->deleteLater();
    });
    killer->start();
}

void ExternalPdfTranslateWorker::cancel()
{
    m_cancelled = true;
    if (m_proc && m_proc->state() != QProcess::NotRunning)
        m_proc->kill();
}

void ExternalPdfTranslateWorker::onReadyReadStandardError()
{
    if (!m_proc)
        return;

    m_stderrBuf += m_proc->readAllStandardError();
    const QString chunk = QString::fromUtf8(m_stderrBuf.right(4096));
    const QStringList lines = chunk.split(QLatin1Char('\n'), Qt::SkipEmptyParts);
    for (auto it = lines.crbegin(); it != lines.crend(); ++it) {
        const QString line = it->trimmed();
        if (line.isEmpty())
            continue;
        if (line.contains(QLatin1Char('%')) || line.contains(QStringLiteral("it/s"))
            || line.contains(QStringLiteral("pdf2zh"), Qt::CaseInsensitive)) {
            emit progressUpdated(line.left(200));
            break;
        }
    }
}

void ExternalPdfTranslateWorker::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    Q_UNUSED(exitStatus);

    if (m_proc) {
        m_stdoutBuf += m_proc->readAllStandardOutput();
        m_stderrBuf += m_proc->readAllStandardError();
    }

    const QString commandLine =
        (m_proc ? m_proc->program() : QString()) + QStringLiteral(" ")
        + (m_proc ? m_proc->arguments().join(QStringLiteral(" ")) : QString());
    DocumentLoader::logPythonRun(QStringLiteral("python"), commandLine, m_stdoutBuf, m_stderrBuf,
                                 exitCode, m_proc != nullptr);

    ExternalPdfTranslateResult result;
    if (m_cancelled) {
        result.error = QStringLiteral("Перевод отменен");
        emit finished(result);
        if (m_proc) {
            m_proc->deleteLater();
            m_proc = nullptr;
        }
        return;
    }

    result = DocumentLoader::parseExternalPdfTranslateOutput(m_request, m_stdoutBuf, m_stderrBuf,
                                                             exitCode, m_proc != nullptr);

    if (m_proc) {
        m_proc->deleteLater();
        m_proc = nullptr;
    }
    emit finished(result);
}

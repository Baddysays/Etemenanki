#include "setup_manager.h"

#include "app_settings.h"
#include "version.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDesktopServices>
#include <QDir>
#include <QEventLoop>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSettings>
#include <QStandardPaths>
#include <QUrl>
#include <QTcpSocket>

namespace {

QString findFileUpwards(const QString& relativePath)
{
    QDir dir(QCoreApplication::applicationDirPath());
    for (int i = 0; i < 8; ++i) {
        const QString path = dir.filePath(relativePath);
        if (QFile::exists(path))
            return path;
        if (!dir.cdUp())
            break;
    }
    return {};
}

} // namespace

SetupManager::SetupManager(QObject* parent)
    : QObject(parent)
    , m_net(new QNetworkAccessManager(this))
    , m_githubRepo(QStringLiteral("Baddysays/Etemenanki"))
    , m_githubUrl(QStringLiteral("https://github.com/Baddysays/Etemenanki"))
    , m_releasesUrl(QStringLiteral("https://github.com/Baddysays/Etemenanki/releases"))
    , m_updateJsonUrl(QStringLiteral(
          "https://raw.githubusercontent.com/Baddysays/Etemenanki/main/releases/update.json"))
{
    QSettings store;
    m_setupComplete = store.value(QStringLiteral("setupComplete"), false).toBool();
    loadManifestOnce();
}

void SetupManager::loadManifestOnce()
{
    if (m_manifestLoaded)
        return;
    m_manifestLoaded = true;
    const QString path = findFileUpwards(QStringLiteral("tools/bootstrap/install_manifest.json"));
    if (path.isEmpty())
        return;
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly))
        return;
    const QJsonObject obj = QJsonDocument::fromJson(f.readAll()).object();
    m_githubRepo = obj.value(QStringLiteral("github_repo")).toString(m_githubRepo);
    m_githubUrl = obj.value(QStringLiteral("github_url")).toString(m_githubUrl);
    m_releasesUrl = obj.value(QStringLiteral("releases_url")).toString(m_releasesUrl);
    m_updateJsonUrl = obj.value(QStringLiteral("update_json_url")).toString(m_updateJsonUrl);
}

bool SetupManager::setupComplete() const { return m_setupComplete; }
bool SetupManager::busy() const { return m_busy; }
QString SetupManager::statusText() const { return m_statusText; }
QString SetupManager::logText() const { return m_logText; }
int SetupManager::downloadProgress() const { return m_downloadProgress; }
QVariantMap SetupManager::hardware() const { return m_hardware; }
QVariantList SetupManager::recommendations() const { return m_recommendations; }
QStringList SetupManager::ollamaInstalled() const { return m_ollamaInstalled; }
QVariantMap SetupManager::depsStatus() const { return m_depsStatus; }
bool SetupManager::probeReady() const { return m_probeReady; }
QVariantMap SetupManager::updateInfo() const { return m_updateInfo; }

QString SetupManager::appVersion() const
{
    return QStringLiteral(ETE_VERSION_STRING);
}

int SetupManager::versionCode() const
{
    return ETE_VERSION_CODE;
}

QString SetupManager::githubUrl() const
{
    return m_githubUrl;
}

QString SetupManager::releasesUrl() const
{
    return m_releasesUrl;
}

void SetupManager::setAppSettings(AppSettings* settings)
{
    m_appSettings = settings;
}

void SetupManager::setSetupComplete(bool value)
{
    if (m_setupComplete == value)
        return;
    m_setupComplete = value;
    QSettings store;
    store.setValue(QStringLiteral("setupComplete"), value);
    store.sync();
    emit setupChanged();
}

void SetupManager::markSetupComplete()
{
    setSetupComplete(true);
    if (m_appSettings)
        m_appSettings->refreshAvailableModels();
}

namespace {

QString findOllamaExecutable()
{
    const QString fromPath = QStandardPaths::findExecutable(QStringLiteral("ollama"));
    if (!fromPath.isEmpty())
        return fromPath;
    const QString localAppData =
        QStandardPaths::writableLocation(QStandardPaths::HomeLocation)
        + QStringLiteral("/AppData/Local/Programs/Ollama/ollama.exe");
    if (QFile::exists(localAppData))
        return localAppData;
    return {};
}

bool tcpPortOpen(const QString& baseUrl, quint16 defaultPort)
{
    const QUrl url(baseUrl);
    const QString host = url.host().isEmpty() ? QStringLiteral("127.0.0.1") : url.host();
    const quint16 port =
        url.port() > 0 ? static_cast<quint16>(url.port()) : defaultPort;

    QTcpSocket socket;
    socket.connectToHost(host, port);
    if (!socket.waitForConnected(1500))
        return false;
    socket.disconnectFromHost();
    return true;
}

bool ollamaPortOpen(const QString& baseUrl)
{
    return tcpPortOpen(baseUrl, 11434);
}

QString embeddedServerBaseUrl()
{
    const QString manifestPath = findFileUpwards(QStringLiteral("engines/llm/manifest.json"));
    if (manifestPath.isEmpty())
        return QStringLiteral("http://127.0.0.1:11435");
    QFile f(manifestPath);
    if (!f.open(QIODevice::ReadOnly))
        return QStringLiteral("http://127.0.0.1:11435");
    const QJsonObject obj = QJsonDocument::fromJson(f.readAll()).object();
    const QString host = obj.value(QStringLiteral("server_host")).toString(QStringLiteral("127.0.0.1"));
    const int port = obj.value(QStringLiteral("server_port")).toInt(11435);
    return QStringLiteral("http://%1:%2").arg(host).arg(port);
}

bool embeddedPortOpen()
{
    return tcpPortOpen(embeddedServerBaseUrl(), 11435);
}

} // namespace

bool SetupManager::ensureOllamaServing()
{
    if (!m_appSettings)
        return false;

    const QString mode = m_appSettings->localAiMode();
    if (mode == QStringLiteral("embedded"))
        return ensureEmbeddedLlmServing();

    const QString baseUrl = m_appSettings->ollamaBaseUrl();
    if (ollamaPortOpen(baseUrl)) {
        setStatusText(QStringLiteral("Ollama is running"));
        return true;
    }

    if (mode == QStringLiteral("ollama")) {
        setStatusText(QStringLiteral("Start Ollama: ollama serve"));
        return false;
    }

    const QString script = findFileUpwards(QStringLiteral("tools/start_ollama.ps1"));
    if (script.isEmpty()) {
        setStatusText(QStringLiteral("Ollama is not running — install from ollama.com"));
        return false;
    }

    setStatusText(QStringLiteral("Starting Ollama…"));

    QProcess proc;
    proc.setProgram(QStringLiteral("powershell"));
    proc.setArguments({
        QStringLiteral("-NoProfile"),
        QStringLiteral("-ExecutionPolicy"),
        QStringLiteral("Bypass"),
        QStringLiteral("-File"),
        script,
        QStringLiteral("-GpuIndex"),
        QString::number(m_appSettings->preferredGpuIndex()),
        QStringLiteral("-OllamaUrl"),
        baseUrl,
    });
    proc.setProcessChannelMode(QProcess::MergedChannels);
    proc.start();
    if (!proc.waitForStarted(10000)) {
        setStatusText(QStringLiteral("Could not start Ollama helper script"));
        return false;
    }
    if (!proc.waitForFinished(90000)) {
        proc.kill();
        setStatusText(QStringLiteral("Ollama start timed out — open ollama.com and install"));
        return false;
    }

    if (ollamaPortOpen(baseUrl)) {
        setStatusText(QStringLiteral("Ollama started"));
        if (m_appSettings)
            m_appSettings->refreshAvailableModels();
        return true;
    }

    const QString out = QString::fromUtf8(proc.readAllStandardOutput()).trimmed();
    setStatusText(out.isEmpty() ? QStringLiteral("Ollama did not start — check GPU settings")
                                : out.left(200));
    return false;
}

bool SetupManager::ensureEmbeddedLlmServing()
{
    if (embeddedPortOpen()) {
        setStatusText(QStringLiteral("Built-in AI is running"));
        if (m_appSettings)
            m_appSettings->refreshAvailableModels();
        return true;
    }

    const QString script = findFileUpwards(QStringLiteral("tools/embedded_llm.py"));
    const QString py = pythonExecutable();
    if (script.isEmpty() || py.isEmpty()) {
        setStatusText(QStringLiteral("Built-in AI: Python or embedded_llm.py not found"));
        return false;
    }

    setStatusText(QStringLiteral("Starting built-in AI…"));

    QProcess proc;
    proc.setProgram(py);
    proc.setArguments({QDir::toNativeSeparators(script), QStringLiteral("serve")});
    proc.setProcessChannelMode(QProcess::MergedChannels);
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert(QStringLiteral("ETEMENANKI_ROOT"), QCoreApplication::applicationDirPath());
    env.insert(QStringLiteral("PYTHONIOENCODING"), QStringLiteral("utf-8"));
    env.insert(QStringLiteral("PYTHONUTF8"), QStringLiteral("1"));
    proc.setProcessEnvironment(env);
    proc.start();
    if (!proc.waitForStarted(10000)) {
        setStatusText(QStringLiteral("Could not start built-in AI"));
        return false;
    }

    // Keep UI responsive while waiting (pip/first start can take a while)
    const qint64 deadlineMs = QDateTime::currentMSecsSinceEpoch() + 180000;
    while (QDateTime::currentMSecsSinceEpoch() < deadlineMs) {
        if (proc.waitForFinished(250))
            break;
        QCoreApplication::processEvents(QEventLoop::AllEvents, 50);
        if (embeddedPortOpen())
            break;
    }
    if (proc.state() != QProcess::NotRunning) {
        // Server may already be listening while helper still exits; give it a moment
        if (!embeddedPortOpen())
            proc.kill();
        else
            proc.waitForFinished(5000);
    }

    if (embeddedPortOpen()) {
        setStatusText(QStringLiteral("Built-in AI started"));
        if (m_appSettings)
            m_appSettings->refreshAvailableModels();
        return true;
    }

    const QString out = QString::fromUtf8(proc.readAllStandardOutput()).trimmed();
    setStatusText(out.isEmpty() ? QStringLiteral("Built-in AI did not start — check model / llama-cpp")
                                : out.left(240));
    return false;
}

void SetupManager::downloadEmbeddedModel()
{
    if (m_busy)
        return;
    const QString script = findFileUpwards(QStringLiteral("tools/embedded_llm.py"));
    const QString py = pythonExecutable();
    if (script.isEmpty() || py.isEmpty()) {
        setStatusText(QStringLiteral("Python or embedded_llm.py not found"));
        return;
    }

    if (m_process) {
        m_process->deleteLater();
        m_process = nullptr;
    }

    m_outputBuffer.clear();
    m_downloadProgress = 0;
    emit downloadProgressChanged();
    m_logText.clear();
    emit logTextChanged();
    setBusy(true);
    setStatusText(QStringLiteral("Preparing built-in model download…"));

    m_process = new QProcess(this);
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert(QStringLiteral("ETEMENANKI_ROOT"), QCoreApplication::applicationDirPath());
    env.insert(QStringLiteral("PYTHONIOENCODING"), QStringLiteral("utf-8"));
    env.insert(QStringLiteral("PYTHONUTF8"), QStringLiteral("1"));
    env.insert(QStringLiteral("PYTHONUNBUFFERED"), QStringLiteral("1"));
    m_process->setProcessEnvironment(env);
    m_process->setProgram(py);
    m_process->setArguments({QStringLiteral("-u"), script, QStringLiteral("install-embedded")});
    m_process->setProcessChannelMode(QProcess::MergedChannels);

    connect(m_process, &QProcess::readyReadStandardOutput, this, [this]() {
        ingestProcessOutput(m_process->readAllStandardOutput());
    });
    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this,
            [this](int code, QProcess::ExitStatus) {
                ingestProcessOutput(m_process->readAllStandardOutput());
                m_downloadProgress = -1;
                emit downloadProgressChanged();
                setBusy(false);
                setStatusText(code == 0 ? QStringLiteral("Built-in model ready (100%)")
                                      : QStringLiteral("Download failed — Settings → open install log"));
                if (m_appSettings)
                    m_appSettings->refreshAvailableModels();
                if (code == 0)
                    probeHardware();
                m_process->deleteLater();
                m_process = nullptr;
            });
    m_process->start();
    if (!m_process->waitForStarted(15000)) {
        setBusy(false);
        m_downloadProgress = -1;
        emit downloadProgressChanged();
        setStatusText(QStringLiteral("Could not start model download"));
    }
}

void SetupManager::openWindowsGpuSettings()
{
    QDesktopServices::openUrl(QUrl(QStringLiteral("ms-settings:display-advancedgraphics")));
}

void SetupManager::appendLog(const QString& line)
{
    if (line.trimmed().isEmpty())
        return;
    m_logText += line + QLatin1Char('\n');
    emit logTextChanged();
}

void SetupManager::handleEmbeddedOutputLine(const QString& line)
{
    const QString trimmed = line.trimmed();
    if (trimmed.startsWith(QStringLiteral("ETEMENANKI_PROGRESS "))) {
        const int pct = trimmed.mid(20).trimmed().toInt();
        if (m_downloadProgress != pct) {
            m_downloadProgress = qBound(0, pct, 100);
            emit downloadProgressChanged();
        }
        setStatusText(QStringLiteral("Downloading built-in model: %1% (~1.7 GB)").arg(m_downloadProgress));
        return;
    }
    if (trimmed.startsWith(QStringLiteral("ETEMENANKI_PHASE "))) {
        const QString phase = trimmed.mid(17).trimmed();
        if (phase == QStringLiteral("pip")) {
            m_downloadProgress = 0;
            emit downloadProgressChanged();
            setStatusText(QStringLiteral("Preparing download (Python packages)…"));
        } else if (phase == QStringLiteral("download")) {
            setStatusText(QStringLiteral("Downloading built-in model (~1.7 GB)…"));
        }
        return;
    }
    appendLog(trimmed);
}

void SetupManager::ingestProcessOutput(const QByteArray& bytes)
{
    if (bytes.isEmpty())
        return;
    m_outputBuffer += bytes;
    int idx = -1;
    while ((idx = m_outputBuffer.indexOf('\n')) >= 0) {
        const QByteArray lineBytes = m_outputBuffer.left(idx);
        m_outputBuffer.remove(0, idx + 1);
        handleEmbeddedOutputLine(QString::fromUtf8(lineBytes).trimmed());
    }
}

void SetupManager::openInstallLog()
{
    const QString tempLog = QStandardPaths::writableLocation(QStandardPaths::TempLocation)
                              + QStringLiteral("/Etemenanki-install-deps.log");
    QStringList candidates = {tempLog};
    const QString appLog = QCoreApplication::applicationDirPath()
                           + QStringLiteral("/logs/install-deps.log");
    candidates << appLog;
    for (const QString& path : candidates) {
        if (QFile::exists(path)) {
            QDesktopServices::openUrl(QUrl::fromLocalFile(path));
            return;
        }
    }
    setStatusText(QStringLiteral("Install log not found"));
}

void SetupManager::setBusy(bool value)
{
    if (m_busy == value)
        return;
    m_busy = value;
    emit busyChanged();
}

void SetupManager::setStatusText(const QString& text)
{
    if (m_statusText == text)
        return;
    m_statusText = text;
    emit statusTextChanged();
}

QJsonObject SetupManager::parseProbeJson(const QByteArray& stdoutBytes)
{
    const QString text = QString::fromUtf8(stdoutBytes).trimmed();
    if (text.isEmpty())
        return {};

    // Probe prints a single JSON object; take the last line that looks like JSON.
    const QStringList lines = text.split(QLatin1Char('\n'), Qt::SkipEmptyParts);
    for (int i = lines.size() - 1; i >= 0; --i) {
        const QString line = lines.at(i).trimmed();
        if (!line.startsWith(QLatin1Char('{')))
            continue;
        const QJsonDocument doc = QJsonDocument::fromJson(line.toUtf8());
        if (doc.isObject())
            return doc.object();
    }

    const int jsonStart = text.indexOf(QLatin1Char('{'));
    if (jsonStart < 0)
        return {};
    const QJsonDocument doc = QJsonDocument::fromJson(text.mid(jsonStart).toUtf8());
    return doc.isObject() ? doc.object() : QJsonObject{};
}

void SetupManager::applyProbeResult(const QJsonObject& root)
{
    if (root.isEmpty() || !root.value(QStringLiteral("ok")).toBool(false))
        return;

    m_hardware = root.toVariantMap();
    m_recommendations = root.value(QStringLiteral("recommendations")).toArray().toVariantList();
    m_depsStatus = root.value(QStringLiteral("deps")).toObject().toVariantMap();
    emit depsStatusChanged();

    QStringList installed;
    for (const QJsonValue& v : root.value(QStringLiteral("ollama_installed")).toArray()) {
        const QString name = v.toString().trimmed();
        if (!name.isEmpty())
            installed << name;
    }
    m_ollamaInstalled = installed;

    const bool ready = root.contains(QStringLiteral("ram_gb")) && !m_recommendations.isEmpty();
    if (m_probeReady != ready) {
        m_probeReady = ready;
        emit probeReadyChanged();
    }

    emit hardwareChanged();
    emit recommendationsChanged();
    emit ollamaInstalledChanged();

    const double ram = root.value(QStringLiteral("ram_gb")).toDouble();
    const QString tier = root.value(QStringLiteral("hardware_tier")).toString();
    const double vram = root.value(QStringLiteral("vram_gb")).toDouble(-1.0);
    QString status = QStringLiteral("RAM %1 GB").arg(ram, 0, 'f', 1);
    if (vram >= 0.0)
        status += QStringLiteral(", VRAM %1 GB").arg(vram, 0, 'f', 1);
    if (!tier.isEmpty())
        status += QStringLiteral(" · %1").arg(tier);
    if (!installed.isEmpty())
        status += QStringLiteral(" · Ollama: %1 model(s)").arg(installed.size());
    const int recGpu = root.value(QStringLiteral("recommended_gpu_index")).toInt(-1);
    if (m_appSettings && m_appSettings->preferredGpuIndex() < 0 && recGpu >= 0)
        m_appSettings->setPreferredGpuIndex(recGpu);
    if (m_depsStatus.value(QStringLiteral("python")).toBool()) {
        const QString pyVer = m_depsStatus.value(QStringLiteral("python_version")).toString();
        status += QStringLiteral(" · Python");
        if (!pyVer.isEmpty())
            status += QStringLiteral(" %1").arg(pyVer);
        if (m_depsStatus.value(QStringLiteral("python_libs")).toBool())
            status += QStringLiteral(" OK");
        else
            status += QStringLiteral(" (libs missing)");
    } else {
        status += QStringLiteral(" · Python: not found");
    }
    setStatusText(status);
}

QString SetupManager::bootstrapScriptPath() const
{
    return findFileUpwards(QStringLiteral("tools/bootstrap/bootstrap.py"));
}

QString SetupManager::pythonExecutable() const
{
    const QString appDir = QCoreApplication::applicationDirPath();
    const QString bundled = QDir(appDir).filePath(QStringLiteral("engines/python/python.exe"));
    if (QFile::exists(bundled))
        return QDir::toNativeSeparators(bundled);

    const QString pathTxt = findFileUpwards(QStringLiteral("tools/python_path.txt"));
    if (!pathTxt.isEmpty()) {
        QFile f(pathTxt);
        if (f.open(QIODevice::ReadOnly)) {
            QString line = QString::fromUtf8(f.readAll()).trimmed().split(QLatin1Char('\n')).value(0).trimmed();
            line.remove(QChar(0xFEFF));
            if (!line.isEmpty()) {
                QFileInfo fi(line);
                if (fi.isRelative()) {
                    const QString fromApp = QDir(appDir).filePath(line);
                    if (QFile::exists(fromApp))
                        return QDir::toNativeSeparators(fromApp);
                    const QString fromTxt = QDir(QFileInfo(pathTxt).absolutePath()).filePath(line);
                    // tools/python_path.txt → ../engines/python/python.exe when relative is engines\...
                    const QString fromRoot = QDir(QFileInfo(pathTxt).absoluteDir().absoluteFilePath(QStringLiteral(".."))).filePath(line);
                    if (QFile::exists(fromRoot))
                        return QDir::toNativeSeparators(fromRoot);
                    if (QFile::exists(fromTxt))
                        return QDir::toNativeSeparators(fromTxt);
                } else if (QFile::exists(line)) {
                    return QDir::toNativeSeparators(line);
                }
            }
        }
    }

    const QString upwards = findFileUpwards(QStringLiteral("engines/python/python.exe"));
    if (!upwards.isEmpty())
        return QDir::toNativeSeparators(upwards);

    return QStringLiteral("python");
}

QString SetupManager::githubRepo() const
{
    return m_githubRepo;
}

QString SetupManager::updateJsonUrl() const
{
    return m_updateJsonUrl;
}

void SetupManager::probeHardware()
{
    if (m_busy)
        return;
    const QString script = bootstrapScriptPath();
    const QString py = pythonExecutable();
    if (script.isEmpty()) {
        setStatusText(QStringLiteral("bootstrap.py not found"));
        return;
    }

    setBusy(true);
    setStatusText(QStringLiteral("Analyzing hardware..."));
    appendLog(QStringLiteral("[scan] Detecting RAM, GPU, Python and Ollama..."));

    if (m_process) {
        m_process->deleteLater();
        m_process = nullptr;
    }
    m_process = new QProcess(this);
    m_process->setProgram(py);
    m_process->setArguments({script, QStringLiteral("probe")});
    {
        QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
        env.insert(QStringLiteral("ETEMENANKI_ROOT"), QCoreApplication::applicationDirPath());
        env.insert(QStringLiteral("PYTHONIOENCODING"), QStringLiteral("utf-8"));
        env.insert(QStringLiteral("PYTHONUTF8"), QStringLiteral("1"));
        m_process->setProcessEnvironment(env);
    }
    connect(m_process, &QProcess::readyReadStandardError, this, [this]() {
        appendLog(QString::fromUtf8(m_process->readAllStandardError()));
    });
    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this,
            [this](int code, QProcess::ExitStatus) {
                setBusy(false);
                const QByteArray out = m_process->readAllStandardOutput();
                const QJsonObject root = parseProbeJson(out);
                const bool ok = code == 0 && !root.isEmpty() && root.value(QStringLiteral("ok")).toBool(false);
                if (ok) {
                    applyProbeResult(root);
                    const double ram = root.value(QStringLiteral("ram_gb")).toDouble();
                    const QString tier = root.value(QStringLiteral("hardware_tier")).toString();
                    appendLog(QStringLiteral("[scan] OK — %1 GB RAM, profile «%2», %3 model(s) recommended")
                                  .arg(ram, 0, 'f', 1)
                                  .arg(tier)
                                  .arg(m_recommendations.size()));
                } else {
                    if (!out.isEmpty())
                        appendLog(QString::fromUtf8(out).trimmed());
                    setStatusText(QStringLiteral("Scan failed (exit %1). Check Python and bootstrap.py.")
                                      .arg(code));
                    if (m_probeReady) {
                        m_probeReady = false;
                        emit probeReadyChanged();
                    }
                }
                emit probeFinished(ok);
                m_process->deleteLater();
                m_process = nullptr;
            });
    m_process->start();
}

void SetupManager::runSetup(const QStringList& ollamaModels,
                          bool installPdf2zh,
                          bool installPythonDeps,
                          bool installEmbeddedModel)
{
    if (m_busy)
        return;
    const QString script = bootstrapScriptPath();
    const QString py = pythonExecutable();
    if (script.isEmpty()) {
        setStatusText(QStringLiteral("bootstrap.py not found"));
        emit setupFinished(false);
        return;
    }

    setBusy(true);
    setStatusText(QStringLiteral("Installing components..."));
    m_logText.clear();
    emit logTextChanged();
    appendLog(QStringLiteral("[install] Starting setup..."));

    QStringList args = {script, QStringLiteral("install")};
    if (installPdf2zh)
        args << QStringLiteral("--pdf2zh");
    if (installPythonDeps)
        args << QStringLiteral("--python-deps");
    if (!ollamaModels.isEmpty()) {
        args << QStringLiteral("--ollama-models");
        args << ollamaModels;
    }
    if (installEmbeddedModel)
        args << QStringLiteral("--embedded-model");

    if (m_process) {
        m_process->deleteLater();
        m_process = nullptr;
    }
    m_process = new QProcess(this);
    m_process->setProgram(py);
    m_process->setArguments(args);
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert(QStringLiteral("ETEMENANKI_ROOT"), QCoreApplication::applicationDirPath());
    env.insert(QStringLiteral("PYTHONIOENCODING"), QStringLiteral("utf-8"));
    env.insert(QStringLiteral("PYTHONUTF8"), QStringLiteral("1"));
    m_process->setProcessEnvironment(env);
    connect(m_process, &QProcess::readyReadStandardOutput, this, [this]() {
        appendLog(QString::fromUtf8(m_process->readAllStandardOutput()));
    });
    connect(m_process, &QProcess::readyReadStandardError, this, [this]() {
        appendLog(QString::fromUtf8(m_process->readAllStandardError()));
    });
    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this,
            [this, ollamaModels, installEmbeddedModel](int code, QProcess::ExitStatus) {
                setBusy(false);
                const bool ok = code == 0;
                setStatusText(ok ? QStringLiteral("Setup finished") : QStringLiteral("Setup finished with errors"));
                if (m_appSettings) {
                    m_appSettings->refreshAvailableModels();
                    if (installEmbeddedModel)
                        m_appSettings->setSelectedLocalModel(QStringLiteral("gemma-2-2b-it-q4"));
                    else if (!ollamaModels.isEmpty())
                        m_appSettings->setSelectedLocalModel(ollamaModels.first());
                }
                if (ok)
                    setSetupComplete(true);
                emit setupFinished(ok);
                m_process->deleteLater();
                m_process = nullptr;
            });
    m_process->start();
}

void SetupManager::checkForUpdates()
{
    setStatusText(QStringLiteral("Checking for updates..."));
    QNetworkRequest netReq{QUrl(updateJsonUrl())};
    netReq.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("Etemenanki-Updater/") + appVersion());
    QNetworkReply* reply = m_net->get(netReq);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            m_updateInfo = QVariantMap{
                {QStringLiteral("available"), false},
                {QStringLiteral("message"), reply->errorString()},
                {QStringLiteral("current"), appVersion()},
            };
            setStatusText(reply->errorString());
            emit updateInfoChanged();
            return;
        }
        const QJsonObject obj = QJsonDocument::fromJson(reply->readAll()).object();
        const int remoteCode = obj.value(QStringLiteral("version_code")).toInt();
        const QString remoteName = obj.value(QStringLiteral("version_name")).toString();
        const QString setupUrl = obj.value(QStringLiteral("setup_url")).toString();
        const QString portableUrl = obj.value(QStringLiteral("portable_url")).toString();
        const QString notes = obj.value(QStringLiteral("release_notes")).toString();
        const bool mandatory = obj.value(QStringLiteral("mandatory")).toBool(false);
        const bool newer = remoteCode > versionCode();
        m_updateInfo = QVariantMap{
            {QStringLiteral("available"), newer},
            {QStringLiteral("latest"), remoteName},
            {QStringLiteral("latest_code"), remoteCode},
            {QStringLiteral("current"), appVersion()},
            {QStringLiteral("current_code"), versionCode()},
            {QStringLiteral("url"), setupUrl},
            {QStringLiteral("portable_url"), portableUrl},
            {QStringLiteral("notes"), notes.left(800)},
            {QStringLiteral("mandatory"), mandatory},
        };
        setStatusText(newer ? QStringLiteral("Update available: %1").arg(remoteName)
                            : QStringLiteral("Up to date (%1)").arg(appVersion()));
        emit updateInfoChanged();
    });
}

void SetupManager::openUpdateDownload()
{
    const QString url = m_updateInfo.value(QStringLiteral("url")).toString();
    if (!url.isEmpty())
        QDesktopServices::openUrl(QUrl(url));
}

void SetupManager::openOllamaDownload()
{
    QDesktopServices::openUrl(QUrl(QStringLiteral("https://ollama.com/download/windows")));
}

void SetupManager::openGitHubReleases()
{
    QDesktopServices::openUrl(QUrl(releasesUrl()));
}

void SetupManager::openGitHubRepo()
{
    QDesktopServices::openUrl(QUrl(githubUrl()));
}

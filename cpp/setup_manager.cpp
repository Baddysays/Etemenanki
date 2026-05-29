#include "setup_manager.h"

#include "app_settings.h"
#include "version.h"

#include <QCoreApplication>
#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSettings>
#include <QStandardPaths>
#include <QUrl>

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
QVariantMap SetupManager::hardware() const { return m_hardware; }
QVariantList SetupManager::recommendations() const { return m_recommendations; }
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

void SetupManager::appendLog(const QString& line)
{
    m_logText += line + QLatin1Char('\n');
    emit logTextChanged();
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

QString SetupManager::bootstrapScriptPath() const
{
    return findFileUpwards(QStringLiteral("tools/bootstrap/bootstrap.py"));
}

QString SetupManager::pythonExecutable() const
{
    const QString pathTxt = findFileUpwards(QStringLiteral("tools/python_path.txt"));
    if (!pathTxt.isEmpty()) {
        QFile f(pathTxt);
        if (f.open(QIODevice::ReadOnly)) {
            const QString line = QString::fromUtf8(f.readAll()).trimmed().split(QLatin1Char('\n')).value(0).trimmed();
            if (QFile::exists(line))
                return line;
        }
    }
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
    appendLog(QStringLiteral("[probe] starting"));

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
        m_process->setProcessEnvironment(env);
    }
    connect(m_process, &QProcess::readyReadStandardError, this, [this]() {
        appendLog(QString::fromUtf8(m_process->readAllStandardError()));
    });
    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this,
            [this](int code, QProcess::ExitStatus) {
                setBusy(false);
                const QByteArray out = m_process->readAllStandardOutput();
                if (!out.isEmpty())
                    appendLog(QString::fromUtf8(out));
                const int jsonStart = out.indexOf('{');
                if (jsonStart >= 0) {
                    const QJsonDocument doc = QJsonDocument::fromJson(out.mid(jsonStart));
                    const QJsonObject root = doc.object();
                    m_hardware = root.toVariantMap();
                    m_recommendations = root.value(QStringLiteral("recommendations")).toArray().toVariantList();
                    setStatusText(QStringLiteral("Hardware: %1 GB RAM, tier %2")
                                      .arg(root.value(QStringLiteral("ram_gb")).toDouble())
                                      .arg(root.value(QStringLiteral("hardware_tier")).toString()));
                } else {
                    setStatusText(QStringLiteral("Probe failed (exit %1)").arg(code));
                }
                emit probeFinished();
                m_process->deleteLater();
                m_process = nullptr;
            });
    m_process->start();
}

void SetupManager::runSetup(const QStringList& ollamaModels, bool installPdf2zh, bool installPythonDeps)
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
    appendLog(QStringLiteral("[install] starting"));

    QStringList args = {script, QStringLiteral("install")};
    if (installPdf2zh)
        args << QStringLiteral("--pdf2zh");
    if (installPythonDeps)
        args << QStringLiteral("--python-deps");
    if (!ollamaModels.isEmpty()) {
        args << QStringLiteral("--ollama-models");
        args << ollamaModels;
    }

    if (m_process) {
        m_process->deleteLater();
        m_process = nullptr;
    }
    m_process = new QProcess(this);
    m_process->setProgram(py);
    m_process->setArguments(args);
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert(QStringLiteral("ETEMENANKI_ROOT"), QCoreApplication::applicationDirPath());
    m_process->setProcessEnvironment(env);
    connect(m_process, &QProcess::readyReadStandardOutput, this, [this]() {
        appendLog(QString::fromUtf8(m_process->readAllStandardOutput()));
    });
    connect(m_process, &QProcess::readyReadStandardError, this, [this]() {
        appendLog(QString::fromUtf8(m_process->readAllStandardError()));
    });
    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this,
            [this, ollamaModels](int code, QProcess::ExitStatus) {
                setBusy(false);
                const bool ok = code == 0;
                setStatusText(ok ? QStringLiteral("Setup finished") : QStringLiteral("Setup finished with errors"));
                if (m_appSettings) {
                    m_appSettings->refreshAvailableModels();
                    if (!ollamaModels.isEmpty())
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

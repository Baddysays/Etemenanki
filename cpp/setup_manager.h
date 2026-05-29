#pragma once

#include <QObject>
#include <QProcess>
#include <QVariantList>
#include <QVariantMap>

class QNetworkAccessManager;
class QNetworkReply;
class AppSettings;

class SetupManager final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool setupComplete READ setupComplete WRITE setSetupComplete NOTIFY setupChanged)
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
    Q_PROPERTY(QString statusText READ statusText NOTIFY statusTextChanged)
    Q_PROPERTY(QString logText READ logText NOTIFY logTextChanged)
    Q_PROPERTY(QVariantMap hardware READ hardware NOTIFY probeFinished)
    Q_PROPERTY(QVariantList recommendations READ recommendations NOTIFY probeFinished)
    Q_PROPERTY(QVariantMap updateInfo READ updateInfo NOTIFY updateInfoChanged)
    Q_PROPERTY(QString appVersion READ appVersion CONSTANT)
    Q_PROPERTY(int versionCode READ versionCode CONSTANT)
    Q_PROPERTY(QString githubUrl READ githubUrl CONSTANT)
    Q_PROPERTY(QString releasesUrl READ releasesUrl CONSTANT)

public:
    explicit SetupManager(QObject* parent = nullptr);

    bool setupComplete() const;
    bool busy() const;
    QString statusText() const;
    QString logText() const;
    QVariantMap hardware() const;
    QVariantList recommendations() const;
    QVariantMap updateInfo() const;
    QString appVersion() const;
    int versionCode() const;
    QString githubUrl() const;
    QString releasesUrl() const;

    void setAppSettings(AppSettings* settings);

    Q_INVOKABLE void probeHardware();
    Q_INVOKABLE void runSetup(const QStringList& ollamaModels, bool installPdf2zh, bool installPythonDeps);
    Q_INVOKABLE void checkForUpdates();
    Q_INVOKABLE void openUpdateDownload();
    Q_INVOKABLE void openOllamaDownload();
    Q_INVOKABLE void openGitHubReleases();
    Q_INVOKABLE void openGitHubRepo();
    Q_INVOKABLE void markSetupComplete();

    void setSetupComplete(bool value);

signals:
    void setupChanged();
    void busyChanged();
    void statusTextChanged();
    void logTextChanged();
    void probeFinished();
    void setupFinished(bool ok);
    void updateInfoChanged();

private:
    void appendLog(const QString& line);
    void setBusy(bool value);
    void setStatusText(const QString& text);
    QString bootstrapScriptPath() const;
    QString pythonExecutable() const;
    QString githubRepo() const;
    QString updateJsonUrl() const;
    void loadManifestOnce();

    AppSettings* m_appSettings = nullptr;
    QProcess* m_process = nullptr;
    QNetworkAccessManager* m_net = nullptr;
    bool m_setupComplete = false;
    bool m_busy = false;
    QString m_statusText;
    QString m_logText;
    QVariantMap m_hardware;
    QVariantList m_recommendations;
    QVariantMap m_updateInfo;
    bool m_manifestLoaded = false;
    QString m_githubRepo;
    QString m_githubUrl;
    QString m_releasesUrl;
    QString m_updateJsonUrl;
};

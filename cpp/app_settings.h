#pragma once

#include <QObject>
#include <QSettings>
#include <QString>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>

class AppSettings final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString appUiLanguage READ appUiLanguage WRITE setAppUiLanguage NOTIFY changed)
    Q_PROPERTY(QStringList enabledLanguageCodes READ enabledLanguageCodes NOTIFY changed)
    Q_PROPERTY(QStringList availableLocalModels READ availableLocalModels NOTIFY modelsChanged)
    Q_PROPERTY(QStringList availableCloudModels READ availableCloudModels NOTIFY modelsChanged)
    Q_PROPERTY(bool localRuntimeAvailable READ localRuntimeAvailable NOTIFY modelsChanged)
    Q_PROPERTY(bool cloudRuntimeAvailable READ cloudRuntimeAvailable NOTIFY modelsChanged)
    Q_PROPERTY(QString selectedLocalModel READ selectedLocalModel WRITE setSelectedLocalModel NOTIFY changed)
    Q_PROPERTY(QString selectedCloudModel READ selectedCloudModel WRITE setSelectedCloudModel NOTIFY changed)
    Q_PROPERTY(QString ollamaBaseUrl READ ollamaBaseUrl WRITE setOllamaBaseUrl NOTIFY changed)
    Q_PROPERTY(bool pdfLayoutAuto READ pdfLayoutAuto WRITE setPdfLayoutAuto NOTIFY changed)
    Q_PROPERTY(bool pdfLayoutPreserveTables READ pdfLayoutPreserveTables WRITE setPdfLayoutPreserveTables NOTIFY changed)
    Q_PROPERTY(QString pdfEngine READ pdfEngine WRITE setPdfEngine NOTIFY changed)
    Q_PROPERTY(QString polyglotPdfUrl READ polyglotPdfUrl WRITE setPolyglotPdfUrl NOTIFY changed)
    Q_PROPERTY(QString retainPdfUrl READ retainPdfUrl WRITE setRetainPdfUrl NOTIFY changed)
    Q_PROPERTY(QString retainPdfApiKey READ retainPdfApiKey WRITE setRetainPdfApiKey NOTIFY changed)
    Q_PROPERTY(int translateConcurrent READ translateConcurrent WRITE setTranslateConcurrent NOTIFY changed)
    Q_PROPERTY(QString glossaryText READ glossaryText WRITE setGlossaryText NOTIFY changed)
    Q_PROPERTY(bool glossaryEnabled READ glossaryEnabled WRITE setGlossaryEnabled NOTIFY changed)
    Q_PROPERTY(QString localAiMode READ localAiMode WRITE setLocalAiMode NOTIFY changed)
    Q_PROPERTY(int preferredGpuIndex READ preferredGpuIndex WRITE setPreferredGpuIndex NOTIFY changed)

public:
    explicit AppSettings(QObject* parent = nullptr);

    QString appUiLanguage() const;
    QStringList enabledLanguageCodes() const;
    QStringList availableLocalModels() const;
    QStringList availableCloudModels() const;
    bool localRuntimeAvailable() const;
    bool cloudRuntimeAvailable() const;
    QString selectedLocalModel() const;
    QString selectedCloudModel() const;
    QString ollamaBaseUrl() const;
    bool pdfLayoutAuto() const;
    bool pdfLayoutPreserveTables() const;
    QString pdfEngine() const;
    QString polyglotPdfUrl() const;
    QString retainPdfUrl() const;
    QString retainPdfApiKey() const;
    int translateConcurrent() const;
    QString glossaryText() const;
    bool glossaryEnabled() const;
    QString localAiMode() const;
    int preferredGpuIndex() const;

    Q_INVOKABLE void setAppUiLanguage(const QString& code);
    Q_INVOKABLE void setLanguageEnabled(const QString& code, bool enabled);
    Q_INVOKABLE bool isLanguageEnabled(const QString& code) const;
    Q_INVOKABLE QVariantList allLanguages() const;
    Q_INVOKABLE QVariantList enabledLanguages(bool includeAuto) const;
    Q_INVOKABLE void setSelectedLocalModel(const QString& id);
    Q_INVOKABLE void setSelectedCloudModel(const QString& id);
    Q_INVOKABLE void setOllamaBaseUrl(const QString& url);
    Q_INVOKABLE void setPdfLayoutAuto(bool value);
    Q_INVOKABLE void setPdfLayoutPreserveTables(bool value);
    Q_INVOKABLE void setPdfEngine(const QString& id);
    Q_INVOKABLE void setPolyglotPdfUrl(const QString& url);
    Q_INVOKABLE void setRetainPdfUrl(const QString& url);
    Q_INVOKABLE void setRetainPdfApiKey(const QString& key);
    Q_INVOKABLE void setTranslateConcurrent(int value);
    Q_INVOKABLE void setGlossaryText(const QString& text);
    Q_INVOKABLE void setGlossaryEnabled(bool value);
    Q_INVOKABLE void setLocalAiMode(const QString& mode);
    Q_INVOKABLE void setPreferredGpuIndex(int index);
    Q_INVOKABLE QString uiText(const QString& key, const QString& langCode = QString()) const;
    Q_INVOKABLE QVariantList appUiLanguageOptions() const;
    Q_INVOKABLE int appUiLanguageOptionIndex() const;
    Q_INVOKABLE QVariantMap cloudProvider(const QString& providerId) const;
    Q_INVOKABLE void setCloudProvider(const QString& providerId,
                                      const QString& baseUrl,
                                      const QString& apiKey,
                                      const QString& modelId,
                                      bool enabled);
    Q_INVOKABLE QVariantList cloudProviders() const;
    Q_INVOKABLE QString modelForRuntime(const QString& runtime) const;
    Q_INVOKABLE QString cloudBaseUrlForModel(const QString& modelId) const;
    Q_INVOKABLE QString cloudApiKeyForModel(const QString& modelId) const;
    Q_INVOKABLE void refreshAvailableModels();
    Q_INVOKABLE QString openFileFilter() const;
    Q_INVOKABLE QVariantList pdfEngineCatalog() const;
    Q_INVOKABLE QVariantMap probePdfEnginesStatus() const;
    Q_INVOKABLE void openExternalUrl(const QString& url) const;

signals:
    void changed();
    void modelsChanged();

private:
    void load();
    void save();
    static QString normalizeOllamaUrl(const QString& url);

    QSettings m_store;
    QString m_appUiLanguage = QStringLiteral("ru");
    QStringList m_enabledLanguages;
    QStringList m_availableLocal;
    QStringList m_availableCloud;
    QString m_selectedLocal;
    QString m_selectedCloud;
    QString m_ollamaBaseUrl = QStringLiteral("http://127.0.0.1:11434");
    bool m_pdfLayoutAuto = true;
    bool m_pdfLayoutPreserveTables = true;
    QString m_pdfEngine = QStringLiteral("etemenanki");
    QString m_polyglotPdfUrl = QStringLiteral("http://127.0.0.1:12226");
    QString m_retainPdfUrl = QStringLiteral("http://127.0.0.1:41000");
    QString m_retainPdfApiKey;
    int m_translateConcurrent = 1;
    QString m_glossaryText;
    bool m_glossaryEnabled = false;
    QString m_localAiMode = QStringLiteral("auto");
    int m_preferredGpuIndex = -1;
};

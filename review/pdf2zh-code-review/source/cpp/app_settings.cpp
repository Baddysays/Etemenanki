#include "app_settings.h"

#include "document_loader.h"
#include "translation_workflow.h"

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
#include <QUrl>

namespace {

QString catalogPath()
{
    QDir dir(QCoreApplication::applicationDirPath());
    for (int i = 0; i < 8; ++i) {
        const QString path = dir.filePath(QStringLiteral("assets/models_catalog.json"));
        if (QFile::exists(path))
            return path;
        if (!dir.cdUp())
            break;
    }
    return {};
}

QJsonArray loadCatalogModels()
{
    QFile f(catalogPath());
    if (!f.open(QIODevice::ReadOnly))
        return {};
    const QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
    return doc.object().value(QStringLiteral("models")).toArray();
}

QString providerForModelId(const QString& modelId)
{
    if (modelId.startsWith(QStringLiteral("gpt"), Qt::CaseInsensitive)
        || modelId.contains(QStringLiteral("openai"), Qt::CaseInsensitive))
        return QStringLiteral("openai");
    if (modelId.contains(QStringLiteral("deepseek"), Qt::CaseInsensitive))
        return QStringLiteral("deepseek");
    return QStringLiteral("custom");
}

} // namespace

AppSettings::AppSettings(QObject* parent)
    : QObject(parent)
{
    load();
    refreshAvailableModels();
}

QString AppSettings::normalizeOllamaUrl(const QString& url)
{
    QString base = url.trimmed();
    base.replace(QLatin1Char('\\'), QLatin1Char('/'));
    if (base.isEmpty())
        base = QStringLiteral("http://127.0.0.1:11434");
    if (!base.startsWith(QStringLiteral("http://"), Qt::CaseInsensitive)
        && !base.startsWith(QStringLiteral("https://"), Qt::CaseInsensitive)) {
        while (base.startsWith(QLatin1Char('/')))
            base.remove(0, 1);
        base = QStringLiteral("http://") + base;
    }
    while (base.endsWith(QLatin1Char('/')))
        base.chop(1);
    return base;
}

void AppSettings::load()
{
    m_appUiLanguage = m_store.value(QStringLiteral("appUiLanguage"), QStringLiteral("ru")).toString();
    m_ollamaBaseUrl = normalizeOllamaUrl(m_store.value(QStringLiteral("ollamaBaseUrl")).toString());
    m_selectedLocal =
        m_store.value(QStringLiteral("selectedLocalModel"), QStringLiteral("translategemma:4b")).toString();
    m_selectedCloud =
        m_store.value(QStringLiteral("selectedCloudModel"), QStringLiteral("deepseek-chat")).toString();
    m_pdfLayoutAuto = m_store.value(QStringLiteral("pdfLayoutAuto"), true).toBool();
    m_pdfLayoutPreserveTables = m_store.value(QStringLiteral("pdfLayoutPreserveTables"), true).toBool();
    m_pdfEngine = m_store.value(QStringLiteral("pdfEngine"), QStringLiteral("etemenanki")).toString();
    if (m_pdfEngine.isEmpty()
        || m_pdfEngine == QStringLiteral("polyglotpdf")
        || m_pdfEngine == QStringLiteral("retainpdf"))
        m_pdfEngine = QStringLiteral("etemenanki");
    m_polyglotPdfUrl = m_store.value(QStringLiteral("polyglotPdfUrl"), QStringLiteral("http://127.0.0.1:12226")).toString();
    m_retainPdfUrl = m_store.value(QStringLiteral("retainPdfUrl"), QStringLiteral("http://127.0.0.1:41000")).toString();
    m_retainPdfApiKey = m_store.value(QStringLiteral("retainPdfApiKey")).toString();
    m_translateConcurrent = qBound(1, m_store.value(QStringLiteral("translateConcurrent"), 2).toInt(), 6);
    m_glossaryText = m_store.value(QStringLiteral("glossaryText")).toString();
    m_glossaryEnabled = m_store.value(QStringLiteral("glossaryEnabled"), false).toBool();

    const QVariantList stored = m_store.value(QStringLiteral("enabledLanguages")).toList();
    m_enabledLanguages.clear();
    for (const QVariant& v : stored) {
        const QString code = v.toString().trimmed();
        if (!code.isEmpty())
            m_enabledLanguages << code;
    }
    if (m_enabledLanguages.isEmpty()) {
        m_enabledLanguages = {
            QStringLiteral("en"), QStringLiteral("ru"), QStringLiteral("de"), QStringLiteral("fr"),
            QStringLiteral("es"), QStringLiteral("it"), QStringLiteral("pt"), QStringLiteral("pl"),
            QStringLiteral("nl"), QStringLiteral("sv"), QStringLiteral("uk"), QStringLiteral("zh"),
            QStringLiteral("ja"), QStringLiteral("ko"), QStringLiteral("ar"), QStringLiteral("tr"),
        };
    }
}

void AppSettings::save()
{
    m_store.setValue(QStringLiteral("appUiLanguage"), m_appUiLanguage);
    m_store.setValue(QStringLiteral("ollamaBaseUrl"), m_ollamaBaseUrl);
    m_store.setValue(QStringLiteral("selectedLocalModel"), m_selectedLocal);
    m_store.setValue(QStringLiteral("selectedCloudModel"), m_selectedCloud);
    m_store.setValue(QStringLiteral("pdfLayoutAuto"), m_pdfLayoutAuto);
    m_store.setValue(QStringLiteral("pdfLayoutPreserveTables"), m_pdfLayoutPreserveTables);
    m_store.setValue(QStringLiteral("pdfEngine"), m_pdfEngine);
    m_store.setValue(QStringLiteral("polyglotPdfUrl"), m_polyglotPdfUrl);
    m_store.setValue(QStringLiteral("retainPdfUrl"), m_retainPdfUrl);
    m_store.setValue(QStringLiteral("retainPdfApiKey"), m_retainPdfApiKey);
    m_store.setValue(QStringLiteral("translateConcurrent"), m_translateConcurrent);
    m_store.setValue(QStringLiteral("glossaryText"), m_glossaryText);
    m_store.setValue(QStringLiteral("glossaryEnabled"), m_glossaryEnabled);
    QVariantList langs;
    for (const QString& code : m_enabledLanguages)
        langs << code;
    m_store.setValue(QStringLiteral("enabledLanguages"), langs);
    m_store.sync();
}

QString AppSettings::appUiLanguage() const { return m_appUiLanguage; }
QStringList AppSettings::enabledLanguageCodes() const { return m_enabledLanguages; }
QStringList AppSettings::availableLocalModels() const { return m_availableLocal; }
QStringList AppSettings::availableCloudModels() const { return m_availableCloud; }
bool AppSettings::localRuntimeAvailable() const { return !m_availableLocal.isEmpty(); }
bool AppSettings::cloudRuntimeAvailable() const { return !m_availableCloud.isEmpty(); }
QString AppSettings::selectedLocalModel() const { return m_selectedLocal; }
QString AppSettings::selectedCloudModel() const { return m_selectedCloud; }
QString AppSettings::ollamaBaseUrl() const { return m_ollamaBaseUrl; }
bool AppSettings::pdfLayoutAuto() const { return m_pdfLayoutAuto; }
bool AppSettings::pdfLayoutPreserveTables() const { return m_pdfLayoutPreserveTables; }
QString AppSettings::pdfEngine() const { return m_pdfEngine; }
QString AppSettings::polyglotPdfUrl() const { return m_polyglotPdfUrl; }
QString AppSettings::retainPdfUrl() const { return m_retainPdfUrl; }
QString AppSettings::retainPdfApiKey() const { return m_retainPdfApiKey; }
int AppSettings::translateConcurrent() const { return m_translateConcurrent; }
QString AppSettings::glossaryText() const { return m_glossaryText; }
bool AppSettings::glossaryEnabled() const { return m_glossaryEnabled; }

void AppSettings::setAppUiLanguage(const QString& code)
{
    const QString lang = code.trimmed().isEmpty() ? QStringLiteral("ru") : code.trimmed();
    if (m_appUiLanguage == lang)
        return;
    m_appUiLanguage = lang;
    save();
    emit changed();
}

void AppSettings::setLanguageEnabled(const QString& code, bool enabled)
{
    if (code == QStringLiteral("auto"))
        return;
    const bool has = m_enabledLanguages.contains(code);
    if (enabled && !has)
        m_enabledLanguages << code;
    else if (!enabled && has)
        m_enabledLanguages.removeAll(code);
    else
        return;
    save();
    emit changed();
}

bool AppSettings::isLanguageEnabled(const QString& code) const
{
    return m_enabledLanguages.contains(code);
}

QVariantList AppSettings::allLanguages() const
{
    return {
        QVariantMap{{QStringLiteral("code"), QStringLiteral("auto")},
                    {QStringLiteral("label"), uiText(QStringLiteral("lang_auto"))},
                    {QStringLiteral("flag"), QStringLiteral("🌐")}},
        QVariantMap{{QStringLiteral("code"), QStringLiteral("en")},
                    {QStringLiteral("label"), QStringLiteral("English (EN)")},
                    {QStringLiteral("flag"), QStringLiteral("🇺🇸")}},
        QVariantMap{{QStringLiteral("code"), QStringLiteral("ru")},
                    {QStringLiteral("label"), QStringLiteral("Русский (RU)")},
                    {QStringLiteral("flag"), QStringLiteral("🇷🇺")}},
        QVariantMap{{QStringLiteral("code"), QStringLiteral("de")},
                    {QStringLiteral("label"), QStringLiteral("Deutsch (DE)")},
                    {QStringLiteral("flag"), QStringLiteral("🇩🇪")}},
        QVariantMap{{QStringLiteral("code"), QStringLiteral("fr")},
                    {QStringLiteral("label"), QStringLiteral("Français (FR)")},
                    {QStringLiteral("flag"), QStringLiteral("🇫🇷")}},
        QVariantMap{{QStringLiteral("code"), QStringLiteral("es")},
                    {QStringLiteral("label"), QStringLiteral("Español (ES)")},
                    {QStringLiteral("flag"), QStringLiteral("🇪🇸")}},
        QVariantMap{{QStringLiteral("code"), QStringLiteral("it")},
                    {QStringLiteral("label"), QStringLiteral("Italiano (IT)")},
                    {QStringLiteral("flag"), QStringLiteral("🇮🇹")}},
        QVariantMap{{QStringLiteral("code"), QStringLiteral("pt")},
                    {QStringLiteral("label"), QStringLiteral("Português (PT)")},
                    {QStringLiteral("flag"), QStringLiteral("🇵🇹")}},
        QVariantMap{{QStringLiteral("code"), QStringLiteral("pl")},
                    {QStringLiteral("label"), QStringLiteral("Polski (PL)")},
                    {QStringLiteral("flag"), QStringLiteral("🇵🇱")}},
        QVariantMap{{QStringLiteral("code"), QStringLiteral("nl")},
                    {QStringLiteral("label"), QStringLiteral("Nederlands (NL)")},
                    {QStringLiteral("flag"), QStringLiteral("🇳🇱")}},
        QVariantMap{{QStringLiteral("code"), QStringLiteral("sv")},
                    {QStringLiteral("label"), QStringLiteral("Svenska (SV)")},
                    {QStringLiteral("flag"), QStringLiteral("🇸🇪")}},
        QVariantMap{{QStringLiteral("code"), QStringLiteral("uk")},
                    {QStringLiteral("label"), QStringLiteral("Українська (UK)")},
                    {QStringLiteral("flag"), QStringLiteral("🇺🇦")}},
        QVariantMap{{QStringLiteral("code"), QStringLiteral("zh")},
                    {QStringLiteral("label"), QStringLiteral("中文 (ZH)")},
                    {QStringLiteral("flag"), QStringLiteral("🇨🇳")}},
        QVariantMap{{QStringLiteral("code"), QStringLiteral("ja")},
                    {QStringLiteral("label"), QStringLiteral("日本語 (JA)")},
                    {QStringLiteral("flag"), QStringLiteral("🇯🇵")}},
        QVariantMap{{QStringLiteral("code"), QStringLiteral("ko")},
                    {QStringLiteral("label"), QStringLiteral("한국어 (KO)")},
                    {QStringLiteral("flag"), QStringLiteral("🇰🇷")}},
        QVariantMap{{QStringLiteral("code"), QStringLiteral("ar")},
                    {QStringLiteral("label"), QStringLiteral("العربية (AR)")},
                    {QStringLiteral("flag"), QStringLiteral("🇸🇦")}},
        QVariantMap{{QStringLiteral("code"), QStringLiteral("tr")},
                    {QStringLiteral("label"), QStringLiteral("Türkçe (TR)")},
                    {QStringLiteral("flag"), QStringLiteral("🇹🇷")}},
        QVariantMap{{QStringLiteral("code"), QStringLiteral("cs")},
                    {QStringLiteral("label"), QStringLiteral("Čeština (CS)")},
                    {QStringLiteral("flag"), QStringLiteral("🇨🇿")}},
        QVariantMap{{QStringLiteral("code"), QStringLiteral("ro")},
                    {QStringLiteral("label"), QStringLiteral("Română (RO)")},
                    {QStringLiteral("flag"), QStringLiteral("🇷🇴")}},
        QVariantMap{{QStringLiteral("code"), QStringLiteral("hu")},
                    {QStringLiteral("label"), QStringLiteral("Magyar (HU)")},
                    {QStringLiteral("flag"), QStringLiteral("🇭🇺")}},
        QVariantMap{{QStringLiteral("code"), QStringLiteral("fi")},
                    {QStringLiteral("label"), QStringLiteral("Suomi (FI)")},
                    {QStringLiteral("flag"), QStringLiteral("🇫🇮")}},
        QVariantMap{{QStringLiteral("code"), QStringLiteral("no")},
                    {QStringLiteral("label"), QStringLiteral("Norsk (NO)")},
                    {QStringLiteral("flag"), QStringLiteral("🇳🇴")}},
        QVariantMap{{QStringLiteral("code"), QStringLiteral("da")},
                    {QStringLiteral("label"), QStringLiteral("Dansk (DA)")},
                    {QStringLiteral("flag"), QStringLiteral("🇩🇰")}},
        QVariantMap{{QStringLiteral("code"), QStringLiteral("el")},
                    {QStringLiteral("label"), QStringLiteral("Ελληνικά (EL)")},
                    {QStringLiteral("flag"), QStringLiteral("🇬🇷")}},
        QVariantMap{{QStringLiteral("code"), QStringLiteral("he")},
                    {QStringLiteral("label"), QStringLiteral("עברית (HE)")},
                    {QStringLiteral("flag"), QStringLiteral("🇮🇱")}},
        QVariantMap{{QStringLiteral("code"), QStringLiteral("hi")},
                    {QStringLiteral("label"), QStringLiteral("हिन्दी (HI)")},
                    {QStringLiteral("flag"), QStringLiteral("🇮🇳")}},
        QVariantMap{{QStringLiteral("code"), QStringLiteral("vi")},
                    {QStringLiteral("label"), QStringLiteral("Tiếng Việt (VI)")},
                    {QStringLiteral("flag"), QStringLiteral("🇻🇳")}},
        QVariantMap{{QStringLiteral("code"), QStringLiteral("th")},
                    {QStringLiteral("label"), QStringLiteral("ไทย (TH)")},
                    {QStringLiteral("flag"), QStringLiteral("🇹🇭")}},
        QVariantMap{{QStringLiteral("code"), QStringLiteral("id")},
                    {QStringLiteral("label"), QStringLiteral("Bahasa Indonesia (ID)")},
                    {QStringLiteral("flag"), QStringLiteral("🇮🇩")}},
    };
}

QVariantList AppSettings::enabledLanguages(bool includeAuto) const
{
    QVariantList out;
    for (const QVariant& v : allLanguages()) {
        const QVariantMap m = v.toMap();
        const QString code = m.value(QStringLiteral("code")).toString();
        if (code == QStringLiteral("auto")) {
            if (includeAuto)
                out << m;
            continue;
        }
        if (m_enabledLanguages.contains(code))
            out << m;
    }
    return out;
}

void AppSettings::setSelectedLocalModel(const QString& id)
{
    if (id.isEmpty() || m_selectedLocal == id)
        return;
    m_selectedLocal = id;
    save();
    emit changed();
}

void AppSettings::setSelectedCloudModel(const QString& id)
{
    if (id.isEmpty() || m_selectedCloud == id)
        return;
    m_selectedCloud = id;
    save();
    emit changed();
}

void AppSettings::setOllamaBaseUrl(const QString& url)
{
    const QString normalized = normalizeOllamaUrl(url);
    if (m_ollamaBaseUrl == normalized)
        return;
    m_ollamaBaseUrl = normalized;
    save();
    emit changed();
    refreshAvailableModels();
}

void AppSettings::setPdfLayoutAuto(bool value)
{
    if (m_pdfLayoutAuto == value)
        return;
    m_pdfLayoutAuto = value;
    save();
    emit changed();
}

void AppSettings::setPdfLayoutPreserveTables(bool value)
{
    if (m_pdfLayoutPreserveTables == value)
        return;
    m_pdfLayoutPreserveTables = value;
    save();
    emit changed();
}

void AppSettings::setPdfEngine(const QString& id)
{
    const QString engine = id.trimmed().isEmpty() ? QStringLiteral("etemenanki") : id.trimmed();
    if (m_pdfEngine == engine)
        return;
    m_pdfEngine = engine;
    save();
    emit changed();
}

void AppSettings::setPolyglotPdfUrl(const QString& url)
{
    QString base = url.trimmed();
    if (base.isEmpty())
        base = QStringLiteral("http://127.0.0.1:12226");
    while (base.endsWith(QLatin1Char('/')))
        base.chop(1);
    if (m_polyglotPdfUrl == base)
        return;
    m_polyglotPdfUrl = base;
    save();
    emit changed();
}

void AppSettings::setRetainPdfUrl(const QString& url)
{
    QString base = url.trimmed();
    if (base.isEmpty())
        base = QStringLiteral("http://127.0.0.1:41000");
    while (base.endsWith(QLatin1Char('/')))
        base.chop(1);
    if (m_retainPdfUrl == base)
        return;
    m_retainPdfUrl = base;
    save();
    emit changed();
}

void AppSettings::setRetainPdfApiKey(const QString& key)
{
    if (m_retainPdfApiKey == key)
        return;
    m_retainPdfApiKey = key;
    save();
    emit changed();
}

void AppSettings::setTranslateConcurrent(int value)
{
    const int bounded = qBound(1, value, 6);
    if (m_translateConcurrent == bounded)
        return;
    m_translateConcurrent = bounded;
    save();
    emit changed();
}

void AppSettings::setGlossaryText(const QString& text)
{
    if (m_glossaryText == text)
        return;
    m_glossaryText = text;
    save();
    emit changed();
}

void AppSettings::setGlossaryEnabled(bool value)
{
    if (m_glossaryEnabled == value)
        return;
    m_glossaryEnabled = value;
    save();
    emit changed();
}

QString AppSettings::uiText(const QString& key) const
{
    const bool en = m_appUiLanguage == QStringLiteral("en");
    if (key == QStringLiteral("lang_auto"))
        return en ? QStringLiteral("Auto") : QStringLiteral("Авто");
    if (key == QStringLiteral("load_file"))
        return en ? QStringLiteral("Open file") : QStringLiteral("Загрузить файл");
    if (key == QStringLiteral("translate"))
        return en ? QStringLiteral("Translate") : QStringLiteral("Перевести");
    if (key == QStringLiteral("save"))
        return en ? QStringLiteral("Save") : QStringLiteral("Сохранить");
    if (key == QStringLiteral("cancel"))
        return en ? QStringLiteral("Cancel") : QStringLiteral("Отмена");
    if (key == QStringLiteral("settings"))
        return en ? QStringLiteral("Settings") : QStringLiteral("Настройки");
    if (key == QStringLiteral("help"))
        return en ? QStringLiteral("Help") : QStringLiteral("Справка");
    if (key == QStringLiteral("source_lang"))
        return en ? QStringLiteral("Source language") : QStringLiteral("Исходный язык");
    if (key == QStringLiteral("target_lang"))
        return en ? QStringLiteral("Target language") : QStringLiteral("Язык перевода");
    if (key == QStringLiteral("local"))
        return en ? QStringLiteral("Local") : QStringLiteral("Локально");
    if (key == QStringLiteral("cloud"))
        return en ? QStringLiteral("Cloud") : QStringLiteral("Облако");
    if (key == QStringLiteral("subtitle"))
        return en ? QStringLiteral("AI document translator") : QStringLiteral("AI-переводчик документов");
    if (key == QStringLiteral("no_local_models"))
        return en ? QStringLiteral("No local models — install in Ollama")
                  : QStringLiteral("Нет локальных моделей — установите в Ollama");
    if (key == QStringLiteral("no_cloud_models"))
        return en ? QStringLiteral("No cloud models — add API keys in Settings")
                  : QStringLiteral("Нет облачных моделей — укажите API в настройках");
    if (key == QStringLiteral("select_language"))
        return en ? QStringLiteral("Select language") : QStringLiteral("Выберите язык");
    if (key == QStringLiteral("hub_title"))
        return en ? QStringLiteral("Translation hub") : QStringLiteral("Центр перевода");
    if (key == QStringLiteral("hub_pipeline"))
        return en ? QStringLiteral("Pipeline") : QStringLiteral("Пайплайн");
    if (key == QStringLiteral("hub_export"))
        return en ? QStringLiteral("Export") : QStringLiteral("Экспорт");
    if (key == QStringLiteral("hub_formats"))
        return en ? QStringLiteral("Formats")
                  : QStringLiteral("PDF, DOCX, XLSX, SRT, JSON, EPUB, TXT, MD, HTML");
    return key;
}

QVariantMap AppSettings::cloudProvider(const QString& providerId) const
{
    const QString prefix = QStringLiteral("cloud/%1/").arg(providerId);
    return QVariantMap{
        {QStringLiteral("id"), providerId},
        {QStringLiteral("baseUrl"),
         m_store.value(prefix + QStringLiteral("baseUrl")).toString()},
        {QStringLiteral("apiKey"), m_store.value(prefix + QStringLiteral("apiKey")).toString()},
        {QStringLiteral("modelId"), m_store.value(prefix + QStringLiteral("modelId")).toString()},
        {QStringLiteral("enabled"), m_store.value(prefix + QStringLiteral("enabled"), false).toBool()},
    };
}

void AppSettings::setCloudProvider(const QString& providerId,
                                   const QString& baseUrl,
                                   const QString& apiKey,
                                   const QString& modelId,
                                   bool enabled)
{
    const QString prefix = QStringLiteral("cloud/%1/").arg(providerId);
    m_store.setValue(prefix + QStringLiteral("baseUrl"), baseUrl.trimmed());
    m_store.setValue(prefix + QStringLiteral("apiKey"), apiKey.trimmed());
    m_store.setValue(prefix + QStringLiteral("modelId"), modelId.trimmed());
    m_store.setValue(prefix + QStringLiteral("enabled"), enabled);
    m_store.sync();
    refreshAvailableModels();
    emit changed();
}

QVariantList AppSettings::cloudProviders() const
{
    return {
        QVariantMap{{QStringLiteral("id"), QStringLiteral("deepseek")},
                    {QStringLiteral("title"), QStringLiteral("DeepSeek")},
                    {QStringLiteral("defaultUrl"), QStringLiteral("https://api.deepseek.com")},
                    {QStringLiteral("defaultModel"), QStringLiteral("deepseek-chat")}},
        QVariantMap{{QStringLiteral("id"), QStringLiteral("openai")},
                    {QStringLiteral("title"), QStringLiteral("OpenAI")},
                    {QStringLiteral("defaultUrl"), QStringLiteral("https://api.openai.com/v1")},
                    {QStringLiteral("defaultModel"), QStringLiteral("gpt-4.1-mini")}},
    };
}

QString AppSettings::modelForRuntime(const QString& runtime) const
{
    if (runtime == QStringLiteral("cloud"))
        return m_availableCloud.contains(m_selectedCloud) ? m_selectedCloud
                                                          : (m_availableCloud.isEmpty() ? m_selectedCloud
                                                                                        : m_availableCloud.first());
    return m_availableLocal.contains(m_selectedLocal) ? m_selectedLocal
                                                      : (m_availableLocal.isEmpty() ? m_selectedLocal
                                                                                    : m_availableLocal.first());
}

QString AppSettings::cloudBaseUrlForModel(const QString& modelId) const
{
    const QVariantMap p = cloudProvider(providerForModelId(modelId));
    QString url = p.value(QStringLiteral("baseUrl")).toString().trimmed();
    if (url.isEmpty()) {
        if (providerForModelId(modelId) == QStringLiteral("openai"))
            url = QStringLiteral("https://api.openai.com/v1");
        else
            url = QStringLiteral("https://api.deepseek.com");
    }
    return url;
}

QString AppSettings::cloudApiKeyForModel(const QString& modelId) const
{
    return cloudProvider(providerForModelId(modelId)).value(QStringLiteral("apiKey")).toString().trimmed();
}

void AppSettings::refreshAvailableModels()
{
    const QJsonArray catalog = loadCatalogModels();
    QStringList localCatalog;
    QStringList cloudCatalog;
    for (const QJsonValue& v : catalog) {
        const QJsonObject obj = v.toObject();
        const QString id = obj.value(QStringLiteral("id")).toString();
        const QString provider = obj.value(QStringLiteral("provider")).toString();
        if (id.isEmpty())
            continue;
        if (provider == QStringLiteral("ollama"))
            localCatalog << id;
        else if (provider == QStringLiteral("cloud"))
            cloudCatalog << id;
    }

    auto net = new QNetworkAccessManager(this);
    QNetworkRequest req(QUrl(m_ollamaBaseUrl + QStringLiteral("/api/tags")));
    req.setTransferTimeout(5000);
    QNetworkReply* reply = net->get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply, net, localCatalog, cloudCatalog]() {
        QStringList ollamaInstalled;
        if (reply->error() == QNetworkReply::NoError) {
            const QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
            for (const QJsonValue& v : doc.object().value(QStringLiteral("models")).toArray()) {
                const QString name = v.toObject().value(QStringLiteral("name")).toString();
                if (!name.isEmpty())
                    ollamaInstalled << name;
            }
        }
        reply->deleteLater();
        net->deleteLater();

        QStringList local;
        for (const QString& id : localCatalog) {
            for (const QString& installed : ollamaInstalled) {
                if (installed == id || installed.startsWith(id + QLatin1Char(':'))
                    || id.startsWith(installed + QLatin1Char(':'))) {
                    local << id;
                    break;
                }
            }
        }

        QStringList cloud;
        for (const QString& id : cloudCatalog) {
            const QString provider = providerForModelId(id);
            const QVariantMap p = cloudProvider(provider);
            if (!p.value(QStringLiteral("enabled")).toBool())
                continue;
            const QString key = p.value(QStringLiteral("apiKey")).toString().trimmed();
            const QString configuredModel = p.value(QStringLiteral("modelId")).toString().trimmed();
            if (key.isEmpty())
                continue;
            if (configuredModel.isEmpty() || configuredModel == id)
                cloud << id;
        }

        m_availableLocal = local;
        m_availableCloud = cloud;
        emit modelsChanged();
    });
}

QString AppSettings::openFileFilter() const
{
    return TranslationWorkflow::openFileFilter(m_appUiLanguage == QStringLiteral("en"));
}

QVariantList AppSettings::pdfEngineCatalog() const
{
    const bool en = m_appUiLanguage == QStringLiteral("en");
    return {
        QVariantMap{
            {QStringLiteral("id"), QStringLiteral("etemenanki")},
            {QStringLiteral("name"), QStringLiteral("Etemenanki (PyMuPDF)")},
            {QStringLiteral("url"), QString()},
            {QStringLiteral("authors"), QStringLiteral("Etemenanki")},
            {QStringLiteral("license"), QStringLiteral("AGPL-3.0 (PyMuPDF)")},
            {QStringLiteral("desc"),
             en ? QStringLiteral("Built-in engine: block translation via your selected model, PDF rebuild with embedded Python + PyMuPDF.")
                : QStringLiteral("Встроенный движок: перевод блоков вашей моделью, сборка PDF через встроенный Python + PyMuPDF.")},
            {QStringLiteral("requires"),
             en ? QStringLiteral("Bundled Python in engines/python/ or dev .venv (see engines/python/README.md).")
                : QStringLiteral("Python в engines/python/ или dev .venv (см. engines/python/README.md).")},
        },
        QVariantMap{
            {QStringLiteral("id"), QStringLiteral("pdfmathtranslate")},
            {QStringLiteral("name"), QStringLiteral("PDFMathTranslate (pdf2zh)")},
            {QStringLiteral("url"), QStringLiteral("https://github.com/Byaidu/PDFMathTranslate")},
            {QStringLiteral("authors"), QStringLiteral("Byaidu, awwaawwa, reycn and contributors")},
            {QStringLiteral("license"), QStringLiteral("AGPL-3.0")},
            {QStringLiteral("desc"),
             en ? QStringLiteral("Scientific PDF translation with formulas, tables and layout preserved (EMNLP 2025 Demo). Portable .exe bundle — no Python install.")
                : QStringLiteral("Перевод научных PDF с формулами и вёрсткой (EMNLP 2025 Demo). Портативный .exe — Python не нужен.")},
            {QStringLiteral("requires"),
             en ? QStringLiteral("Run tools/setup_pdf2zh.ps1 or place pdf2zh.exe in engines/pdf2zh/. Uses Ollama from Etemenanki settings.")
                : QStringLiteral("Запустите tools/setup_pdf2zh.ps1 или положите pdf2zh.exe в engines/pdf2zh/. Ollama из настроек Etemenanki.")},
        },
    };
}

QVariantMap AppSettings::probePdfEnginesStatus() const
{
    return DocumentLoader::probePdfEngines();
}

void AppSettings::openExternalUrl(const QString& url) const
{
    const QString trimmed = url.trimmed();
    if (!trimmed.isEmpty())
        QDesktopServices::openUrl(QUrl(trimmed));
}

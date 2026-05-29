#include "app_settings.h"

#include "app_ui_strings.h"
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


QString regionalFlag(const char* iso2)
{
    if (!iso2 || !iso2[0] || !iso2[1])
        return QString::fromUtf8(u8"\xF0\x9F\x8C\x90");
    const auto cp = [](char c) -> char32_t {
        return 0x1F1E6u + static_cast<unsigned char>(c) - static_cast<unsigned char>('A');
    };
    const char32_t chars[2] = {cp(iso2[0]), cp(iso2[1])};
    return QString::fromUcs4(chars, 2);
}

QVariantMap languageRow(const char* code, const QString& label, const char* iso2)
{
    return QVariantMap{
        {QStringLiteral("code"), QString::fromUtf8(code)},
        {QStringLiteral("label"), label},
        {QStringLiteral("flag"), regionalFlag(iso2)},
    };
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
    static const QStringList kSupported = {
        QStringLiteral("ru"), QStringLiteral("en"), QStringLiteral("de"), QStringLiteral("fr"),
        QStringLiteral("es"), QStringLiteral("uk"), QStringLiteral("zh"), QStringLiteral("pt"),
        QStringLiteral("el"), QStringLiteral("la"),
    };
    QString lang = code.trimmed();
    if (lang.isEmpty() || !kSupported.contains(lang))
        lang = QStringLiteral("ru");
    if (m_appUiLanguage == lang)
        return;
    m_appUiLanguage = lang;
    save();
    emit changed();
}

QVariantList AppSettings::appUiLanguageOptions() const
{
    return AppUiStrings::uiLanguageOptions();
}

int AppSettings::appUiLanguageOptionIndex() const
{
    const QVariantList opts = appUiLanguageOptions();
    for (int i = 0; i < opts.size(); ++i) {
        if (opts.at(i).toMap().value(QStringLiteral("code")).toString() == m_appUiLanguage)
            return i;
    }
    return 0;
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
        languageRow("auto", uiText(QStringLiteral("lang_auto")), ""),
        languageRow("en", QString::fromUtf8(u8"English (EN)"), "US"),
        languageRow("ru", QString::fromUtf8(u8"Русский (RU)"), "RU"),
        languageRow("de", QString::fromUtf8(u8"Deutsch (DE)"), "DE"),
        languageRow("fr", QString::fromUtf8(u8"Français (FR)"), "FR"),
        languageRow("es", QString::fromUtf8(u8"Español (ES)"), "ES"),
        languageRow("it", QString::fromUtf8(u8"Italiano (IT)"), "IT"),
        languageRow("pt", QString::fromUtf8(u8"Português (PT)"), "PT"),
        languageRow("pl", QString::fromUtf8(u8"Polski (PL)"), "PL"),
        languageRow("nl", QString::fromUtf8(u8"Nederlands (NL)"), "NL"),
        languageRow("sv", QString::fromUtf8(u8"Svenska (SV)"), "SE"),
        languageRow("uk", QString::fromUtf8(u8"Українська (UK)"), "UA"),
        languageRow("zh", QString::fromUtf8(u8"中文 (ZH)"), "CN"),
        languageRow("ja", QString::fromUtf8(u8"日本語 (JA)"), "JP"),
        languageRow("ko", QString::fromUtf8(u8"한국어 (KO)"), "KR"),
        languageRow("ar", QString::fromUtf8(u8"العربية (AR)"), "SA"),
        languageRow("tr", QString::fromUtf8(u8"Türkçe (TR)"), "TR"),
        languageRow("cs", QString::fromUtf8(u8"Čeština (CS)"), "CZ"),
        languageRow("ro", QString::fromUtf8(u8"Română (RO)"), "RO"),
        languageRow("hu", QString::fromUtf8(u8"Magyar (HU)"), "HU"),
        languageRow("fi", QString::fromUtf8(u8"Suomi (FI)"), "FI"),
        languageRow("no", QString::fromUtf8(u8"Norsk (NO)"), "NO"),
        languageRow("da", QString::fromUtf8(u8"Dansk (DA)"), "DK"),
        languageRow("el", QString::fromUtf8(u8"Ελληνικά (EL)"), "GR"),
        languageRow("he", QString::fromUtf8(u8"עברית (HE)"), "IL"),
        languageRow("hi", QString::fromUtf8(u8"हिन्दी (HI)"), "IN"),
        languageRow("vi", QString::fromUtf8(u8"Tiếng Việt (VI)"), "VN"),
        languageRow("th", QString::fromUtf8(u8"ไทย (TH)"), "TH"),
        languageRow("id", QString::fromUtf8(u8"Bahasa Indonesia (ID)"), "ID"),
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

QString AppSettings::uiText(const QString& key, const QString& langCode) const
{
    const QString lang = langCode.trimmed().isEmpty() ? m_appUiLanguage : langCode.trimmed();
    return AppUiStrings::text(key, lang);
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
    return TranslationWorkflow::openFileFilter(m_appUiLanguage != QStringLiteral("ru"));
}

QVariantList AppSettings::pdfEngineCatalog() const
{
    const QString lang = m_appUiLanguage;
    return {
        QVariantMap{
            {QStringLiteral("id"), QStringLiteral("etemenanki")},
            {QStringLiteral("name"), QStringLiteral("Etemenanki (PyMuPDF)")},
            {QStringLiteral("url"), QString()},
            {QStringLiteral("authors"), QStringLiteral("Etemenanki")},
            {QStringLiteral("license"), QStringLiteral("AGPL-3.0 (PyMuPDF)")},
            {QStringLiteral("desc"), AppUiStrings::text(QStringLiteral("pdf_etemenanki_desc"), lang)},
            {QStringLiteral("requires"), AppUiStrings::text(QStringLiteral("pdf_etemenanki_requires"), lang)},
        },
        QVariantMap{
            {QStringLiteral("id"), QStringLiteral("pdfmathtranslate")},
            {QStringLiteral("name"), QStringLiteral("PDFMathTranslate (pdf2zh)")},
            {QStringLiteral("url"), QStringLiteral("https://github.com/Byaidu/PDFMathTranslate")},
            {QStringLiteral("authors"), QStringLiteral("Byaidu, awwaawwa, reycn and contributors")},
            {QStringLiteral("license"), QStringLiteral("AGPL-3.0")},
            {QStringLiteral("desc"), AppUiStrings::text(QStringLiteral("pdf_pdf2zh_desc"), lang)},
            {QStringLiteral("requires"), AppUiStrings::text(QStringLiteral("pdf_pdf2zh_requires"), lang)},
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

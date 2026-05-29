#include "translator_backend.h"

#include "app_settings.h"
#include "document_format.h"
#include "document_loader.h"
#include "translation_workflow.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QHash>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QFutureWatcher>
#include <QElapsedTimer>
#include <QRegularExpression>
#include <QSettings>
#include <QStandardPaths>
#include <QTimer>
#include <QUuid>
#include <QtConcurrent>

#include <algorithm>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

namespace {

QString catalogPath()
{
    QStringList candidates;
    QDir dir(QCoreApplication::applicationDirPath());
    for (int i = 0; i < 8; ++i) {
        const QString path = dir.filePath(QStringLiteral("assets/models_catalog.json"));
        if (QFile::exists(path))
            return path;
        candidates << path;
        if (!dir.cdUp())
            break;
    }
    return candidates.isEmpty() ? QString() : candidates.first();
}

} // namespace

TranslatorBackend::TranslatorBackend(QObject* parent)
    : QObject(parent)
{
    loadCatalog();
    refreshHardware();
    updateModelInfo(m_model);
    probeExtractRuntimeAsync();
}

void TranslatorBackend::probeExtractRuntimeAsync()
{
    setStatus(QStringLiteral("Проверка Python..."));
    QTimer::singleShot(0, this, [this]() {
        DocumentLoader::ensureReady();
        const ExtractRuntimeInfo info = DocumentLoader::probe();
        if (info.ready)
            setStatus(QStringLiteral("Готово — центр перевода: PDF, DOCX, SRT, XLSX, JSON…"));
        else if (!DocumentLoader::activePythonPath().isEmpty())
            setStatus(QStringLiteral("Python: %1").arg(DocumentLoader::activePythonPath()));
        else
            setStatus(info.message);
    });
}

QString TranslatorBackend::sourceText() const { return m_sourceText; }
QString TranslatorBackend::translatedText() const { return m_translatedText; }
QString TranslatorBackend::status() const { return m_status; }
int TranslatorBackend::progress() const { return m_progress; }
int TranslatorBackend::estimatedRemainingSec() const { return m_estimatedRemainingSec; }
bool TranslatorBackend::documentFormatted() const { return m_documentFormatted; }
bool TranslatorBackend::contentIsStructured() const { return m_structuredDocument; }
QStringList TranslatorBackend::words() const { return m_words; }
bool TranslatorBackend::busy() const { return m_busy; }
QString TranslatorBackend::fileName() const { return m_fileName; }
QString TranslatorBackend::filePath() const { return m_filePath; }
QUrl TranslatorBackend::fileUrl() const
{
    if (m_filePath.isEmpty())
        return {};
    return QUrl::fromLocalFile(QDir::fromNativeSeparators(m_filePath));
}

QUrl TranslatorBackend::pdfPreviewUrl() const
{
    if (!m_isPdf)
        return fileUrl();
    if (m_pdfWorkCopyPath.isEmpty() || !QFile::exists(m_pdfWorkCopyPath))
        return {};
    return QUrl::fromLocalFile(QDir::fromNativeSeparators(m_pdfWorkCopyPath));
}

bool TranslatorBackend::isPdf() const { return m_isPdf; }
bool TranslatorBackend::hasTranslatedPdf() const
{
    return !m_translatedPdfPath.isEmpty() && QFile::exists(m_translatedPdfPath);
}
QUrl TranslatorBackend::translatedPdfUrl() const
{
    if (m_translatedPdfPath.isEmpty())
        return {};
    return QUrl::fromLocalFile(QDir::fromNativeSeparators(m_translatedPdfPath));
}
int TranslatorBackend::pageCount() const { return m_pageCount; }
QString TranslatorBackend::workflowTitle() const { return m_workflowTitle; }
QString TranslatorBackend::workflowPipeline() const { return m_workflowPipeline; }
QString TranslatorBackend::workflowExport() const { return m_workflowExport; }
bool TranslatorBackend::workflowRoundTrip() const { return m_workflowRoundTrip; }
QString TranslatorBackend::ramLabel() const { return m_ramLabel; }
QString TranslatorBackend::vramLabel() const { return m_vramLabel; }
bool TranslatorBackend::hwCompatible() const { return m_hwCompatible; }
QString TranslatorBackend::modelSpeed() const { return m_modelSpeed; }
QString TranslatorBackend::modelQuality() const { return m_modelQuality; }
QString TranslatorBackend::modelRamNeed() const { return m_modelRamNeed; }
QString TranslatorBackend::modelVramNeed() const { return m_modelVramNeed; }
QStringList TranslatorBackend::modelIds() const { return m_modelIds; }

void TranslatorBackend::loadCatalog()
{
    QFile f(catalogPath());
    if (!f.open(QIODevice::ReadOnly)) {
        m_modelIds = {QStringLiteral("translategemma:4b"), QStringLiteral("deepseek-chat")};
        return;
    }
    const QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
    f.close();
    m_models = doc.object().value(QStringLiteral("models")).toArray();
    m_modelIds.clear();
    for (const QJsonValue& v : m_models) {
        const QString id = v.toObject().value(QStringLiteral("id")).toString();
        if (!id.isEmpty()) m_modelIds << id;
    }
}

void TranslatorBackend::refreshHardware()
{
#ifdef Q_OS_WIN
    MEMORYSTATUSEX memInfo{};
    memInfo.dwLength = sizeof(memInfo);
    if (GlobalMemoryStatusEx(&memInfo)) {
        m_ramTotalGb = memInfo.ullTotalPhys / (1024.0 * 1024.0 * 1024.0);
        const double used = (memInfo.ullTotalPhys - memInfo.ullAvailPhys) / (1024.0 * 1024.0 * 1024.0);
        m_ramLabel = QStringLiteral("RAM: %1 / %2 GB")
                         .arg(QString::number(used, 'f', 1), QString::number(m_ramTotalGb, 'f', 1));
    } else {
        m_ramLabel = QStringLiteral("RAM: —");
    }
#else
    m_ramLabel = QStringLiteral("RAM: —");
#endif
    m_vramLabel = QStringLiteral("VRAM: —");
    m_vramTotalGb = 0;
    emit hardwareChanged();
}

void TranslatorBackend::updateModelInfo(const QString& modelId)
{
    m_model = modelId;
    QJsonObject selected;
    for (const QJsonValue& v : m_models) {
        const QJsonObject obj = v.toObject();
        if (obj.value(QStringLiteral("id")).toString() == modelId) {
            selected = obj;
            break;
        }
    }

    const int reqRam = selected.value(QStringLiteral("required_ram_gb")).toInt(8);
    const QJsonValue reqVramVal = selected.value(QStringLiteral("required_vram_gb"));
    const bool hasVramReq = !reqVramVal.isNull() && reqVramVal.isDouble();
    const int reqVram = hasVramReq ? reqVramVal.toInt() : 0;

    const QString speedFromCatalog = selected.value(QStringLiteral("speed_label")).toString();
    const QString qualityFromCatalog = selected.value(QStringLiteral("quality_label")).toString();
    if (!speedFromCatalog.isEmpty()) {
        m_modelSpeed = speedFromCatalog;
    } else if (modelId.contains(QStringLiteral("12b"))) {
        m_modelSpeed = QStringLiteral("42 ток/с");
    } else if (modelId.contains(QStringLiteral("4b"))) {
        m_modelSpeed = QStringLiteral("85 ток/с");
    } else if (modelId.contains(QStringLiteral("deepseek"))) {
        m_modelSpeed = QStringLiteral("120 ток/с");
    } else {
        m_modelSpeed = QStringLiteral("95 ток/с");
    }

    if (!qualityFromCatalog.isEmpty()) {
        m_modelQuality = qualityFromCatalog;
    } else if (modelId.contains(QStringLiteral("12b")) || modelId.contains(QStringLiteral("deepseek"))) {
        m_modelQuality = QStringLiteral("Очень высокое");
    } else {
        m_modelQuality = QStringLiteral("Высокое");
    }

    m_modelRamNeed = QStringLiteral("~%1 GB").arg(reqRam);
    m_modelVramNeed = hasVramReq ? QStringLiteral("~%1 GB").arg(reqVram) : QStringLiteral("—");
    m_hwCompatible = m_ramTotalGb <= 0 || m_ramTotalGb >= reqRam;
    if (hasVramReq && m_vramTotalGb > 0) {
        m_hwCompatible = m_hwCompatible && m_vramTotalGb >= reqVram;
    }

    if (m_ramTotalGb > 0) {
        m_ramLabel = QStringLiteral("RAM: %1 / %2 GB")
                         .arg(QString::number(qMin(m_ramTotalGb, static_cast<double>(reqRam)), 'f', 1),
                              QString::number(m_ramTotalGb, 'f', 1));
    }
    if (hasVramReq) {
        if (m_vramTotalGb > 0) {
            m_vramLabel = QStringLiteral("VRAM: %1 / %2 GB")
                              .arg(QString::number(qMin(m_vramTotalGb, static_cast<double>(reqVram)), 'f', 1),
                                   QString::number(m_vramTotalGb, 'f', 1));
        } else {
            m_vramLabel = QStringLiteral("VRAM: треб. ~%1 GB").arg(reqVram);
        }
    }

    emit modelInfoChanged();
    emit hardwareChanged();
}

int TranslatorBackend::estimatePageCount(const QString& text) const
{
    const int blocks = text.split(QStringLiteral("\n\n"), Qt::SkipEmptyParts).size();
    return qMax(1, qMin(120, blocks));
}

QStringList TranslatorBackend::chunkText(const QString& input, int maxChars) const
{
    QStringList chunks;
    int start = 0;
    while (start < input.size()) {
        int end = qMin(start + maxChars, input.size());
        if (end < input.size()) {
            const int nl = input.lastIndexOf('\n', end);
            if (nl > start + 200) end = nl;
        }
        chunks << input.mid(start, end - start);
        start = end;
    }
    if (chunks.isEmpty()) chunks << QString();
    return chunks;
}

QString TranslatorBackend::combinedSourceText() const
{
    if (!m_sourceText.trimmed().isEmpty())
        return m_sourceText.trimmed();

    QStringList parts;
    parts.reserve(m_sourcePages.size());
    for (const QString& page : m_sourcePages) {
        const QString trimmed = page.trimmed();
        if (!trimmed.isEmpty())
            parts << trimmed;
    }
    return parts.join(QStringLiteral("\n\n"));
}

void TranslatorBackend::updateWorkflowInfo(const QString& suffix)
{
    const WorkflowInfo info = TranslationWorkflow::forSuffix(suffix);
    const bool english = m_appSettings && m_appSettings->appUiLanguage() == QStringLiteral("en");
    m_workflowId = info.id;
    m_workflowTitle = english ? info.titleEn : info.titleRu;
    m_workflowPipeline = english ? info.pipelineEn : info.pipelineRu;
    m_workflowExport = english ? info.exportEn : info.exportRu;
    m_workflowRoundTrip = info.roundTripExport;
    emit workflowChanged();
}

QString TranslatorBackend::glossaryPromptSuffix() const
{
    if (!m_appSettings || !m_appSettings->glossaryEnabled())
        return {};
    const QString glossary = m_appSettings->glossaryText().trimmed();
    if (glossary.isEmpty())
        return {};
    return QStringLiteral("\n\nUse this glossary consistently (source=target, one pair per line):\n")
        + glossary;
}

QString TranslatorBackend::buildPromptForChunk(const QString& chunk,
                                               bool layoutChunk,
                                               bool structuredChunk,
                                               bool segmentChunk) const
{
    const QString glossary = glossaryPromptSuffix();
    if (layoutChunk || segmentChunk) {
        return QStringLiteral("Translate each line from %1 to %2.\n"
                              "Input format: ID|text\n"
                              "Output format: ID|translated text (same IDs, one line each).\n"
                              "Translate the entire line into %2 only; do not leave %1 words.\n"
                              "Keep every ID unchanged and in the same order.\n"
                              "Do not merge or split lines. Do not add notes.%4\n\n%3")
            .arg(m_sourceLang, m_targetLang, chunk, glossary);
    }
    if (structuredChunk) {
        return QStringLiteral("Translate this JSON document page from %1 to %2.\n"
                              "Each block has kind: heading, paragraph, table, or list.\n"
                              "Translate only human-readable strings in \"text\" and table cells.\n"
                              "Keep kind, level, indent, and table row/column structure unchanged.\n"
                              "Return ONLY valid JSON: {\"blocks\":[...]}%4\n\n%3")
            .arg(m_sourceLang, m_targetLang, chunk, glossary);
    }
    return QStringLiteral("You are a professional document translator. Translate from %1 to %2.\n"
                          "Preserve the exact formatting of the source:\n"
                          "- Keep markdown headings, lists, tables (| columns |), line breaks, numbering\n"
                          "- Do NOT flatten tables or merge table rows into one line\n"
                          "- Keep spacing and paragraph breaks\n"
                          "Return ONLY the translated text without notes.%4\n\n"
                          "Text:\n%3")
        .arg(m_sourceLang, m_targetLang, chunk, glossary);
}

void TranslatorBackend::buildTranslationUnits()
{
    m_chunks.clear();
    m_chunkPageIndex.clear();

    if (m_pdfLayoutMode && !m_pdfLayout.isEmpty()) {
        const QJsonArray pages = m_pdfLayout.value(QStringLiteral("pages")).toArray();
        for (int page = 0; page < pages.size(); ++page) {
            const QJsonObject pageObj = pages.at(page).toObject();
            QStringList lines;
            for (const QJsonValue& value : pageObj.value(QStringLiteral("items")).toArray()) {
                const QJsonObject item = value.toObject();
                const QString id = item.value(QStringLiteral("id")).toString();
                const QString text = item.value(QStringLiteral("text")).toString().trimmed();
                if (id.isEmpty() || text.isEmpty())
                    continue;
                lines << id + QLatin1Char('|') + text;
            }
            for (const QJsonValue& tableValue : pageObj.value(QStringLiteral("tables")).toArray()) {
                for (const QJsonValue& cellValue :
                     tableValue.toObject().value(QStringLiteral("cells")).toArray()) {
                    const QJsonObject cell = cellValue.toObject();
                    const QString id = cell.value(QStringLiteral("id")).toString();
                    const QString text = cell.value(QStringLiteral("text")).toString().trimmed();
                    if (id.isEmpty() || text.isEmpty())
                        continue;
                    lines << id + QLatin1Char('|') + text;
                }
            }
            if (lines.isEmpty())
                continue;
            m_chunks << lines.join(QLatin1Char('\n'));
            m_chunkPageIndex << page;
        }
        return;
    }

    if (m_segmentMode && !m_sourcePages.isEmpty()) {
        for (int page = 0; page < m_sourcePages.size(); ++page) {
            const QString pageText = m_sourcePages.at(page).trimmed();
            if (pageText.isEmpty())
                continue;
            m_chunks << pageText;
            m_chunkPageIndex << page;
        }
        return;
    }

    if (m_structuredDocument && !m_sourcePageBlocks.isEmpty()) {
        for (int page = 0; page < m_sourcePageBlocks.size(); ++page) {
            const QJsonArray& blocks = m_sourcePageBlocks.at(page);
            if (blocks.isEmpty())
                continue;
            QJsonObject root;
            root.insert(QStringLiteral("blocks"), blocks);
            m_chunks << QString::fromUtf8(
                QJsonDocument(root).toJson(QJsonDocument::Compact));
            m_chunkPageIndex << page;
        }
        return;
    }

    const bool hasPages = m_sourcePages.size() > 1
        || (m_sourcePages.size() == 1 && m_pageCount > 1);

    if (hasPages && !m_sourcePages.isEmpty()) {
        for (int page = 0; page < m_sourcePages.size(); ++page) {
            const QString pageText = m_sourcePages.at(page).trimmed();
            if (pageText.isEmpty())
                continue;
            const QStringList parts = chunkText(pageText);
            for (const QString& part : parts) {
                m_chunks << part;
                m_chunkPageIndex << page;
            }
        }
        return;
    }

    const QStringList parts = chunkText(combinedSourceText());
    for (const QString& part : parts) {
        m_chunks << part;
        m_chunkPageIndex << -1;
    }
}

void TranslatorBackend::setStatus(const QString& text)
{
    if (m_status == text) return;
    m_status = text;
    emit statusChanged();
}

void TranslatorBackend::setBusy(bool value)
{
    if (m_busy == value) return;
    m_busy = value;
    if (!value)
        updateTranslationEta();
    emit busyChanged();
}

int TranslatorBackend::parseTokensPerSec() const
{
    static const QRegularExpression re(QStringLiteral("(\\d+)"));
    const QRegularExpressionMatch match = re.match(m_modelSpeed);
    if (match.hasMatch()) {
        const int value = match.captured(1).toInt();
        if (value > 0)
            return value;
    }
    return 60;
}

void TranslatorBackend::updateTranslationEta()
{
    const int previous = m_estimatedRemainingSec;
    if (!m_busy) {
        m_estimatedRemainingSec = 0;
    } else {
        const int total = m_chunks.size();
        const int done = m_currentChunk;
        if (done > 0 && m_translateTimer.isValid()) {
            const qint64 elapsedSec = qMax<qint64>(1, m_translateTimer.elapsed() / 1000);
            const int avgPerChunk = qMax(1, static_cast<int>(elapsedSec / done));
            m_estimatedRemainingSec = avgPerChunk * qMax(0, total - done);
        } else {
            int chars = 0;
            for (const QString& chunk : m_chunks)
                chars += chunk.size();
            const int tokensPerSec = parseTokensPerSec();
            int estimate = qMax(3, static_cast<int>(chars / qMax(1, tokensPerSec * 3)));
            if (m_translateTimer.isValid()) {
                const int elapsed = static_cast<int>(m_translateTimer.elapsed() / 1000);
                estimate = qMax(0, estimate - elapsed);
            }
            m_estimatedRemainingSec = estimate;
        }
    }

    if (previous != m_estimatedRemainingSec)
        emit estimatedRemainingSecChanged();
}

QString TranslatorBackend::normalizeLocalFilePath(const QString& path)
{
    const QString trimmed = path.trimmed();
    if (trimmed.isEmpty())
        return {};
    if (trimmed.startsWith(QStringLiteral("file:"), Qt::CaseInsensitive)) {
        const QUrl url = QUrl::fromUserInput(trimmed);
        if (url.isValid() && url.isLocalFile())
            return QDir::toNativeSeparators(url.toLocalFile());
    }
    return QDir::toNativeSeparators(trimmed);
}

void TranslatorBackend::setAppSettings(AppSettings* settings)
{
    m_appSettings = settings;
}

QString TranslatorBackend::ollamaApiUrl(const QString& path) const
{
    const QString base = m_appSettings ? m_appSettings->ollamaBaseUrl()
                                       : QStringLiteral("http://127.0.0.1:11434");
    return base + path;
}

QUrl TranslatorBackend::suggestedSaveUrl() const
{
    QString baseName = QStringLiteral("translation_ru");
    QString suffix = QStringLiteral(".txt");
    if (!m_fileName.isEmpty()) {
        const QFileInfo srcName(m_fileName);
        baseName = srcName.completeBaseName() + QStringLiteral("_ru");
        if (m_isPdf && hasTranslatedPdf())
            suffix = QStringLiteral(".pdf");
        else if (!srcName.suffix().isEmpty())
            suffix = QStringLiteral(".") + srcName.suffix();
    } else if (m_isPdf && hasTranslatedPdf()) {
        suffix = QStringLiteral(".pdf");
    }

    QString dir;
    if (!m_filePath.isEmpty())
        dir = QFileInfo(m_filePath).absolutePath();
    if (dir.isEmpty())
        dir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);

    return QUrl::fromLocalFile(QDir(dir).filePath(baseName + suffix));
}

QString TranslatorBackend::extractRuntimeStatus() const
{
    const ExtractRuntimeInfo info = DocumentLoader::probe();
    if (info.ready)
        return QStringLiteral("Python: %1").arg(info.pythonPath);
    if (!info.message.isEmpty())
        return info.message;
    return QStringLiteral("Python не настроен");
}

void TranslatorBackend::loadFileFromUrl(const QUrl& url)
{
    if (url.isEmpty())
        return;
    if (url.isLocalFile())
        loadFile(QDir::toNativeSeparators(url.toLocalFile()));
    else
        loadFile(url.toString(QUrl::FullyEncoded));
}

void TranslatorBackend::setSourceText(const QString& text)
{
    if (m_sourceText == text) return;
    m_sourceText = text;
    m_pageCount = estimatePageCount(m_sourceText);
    emit sourceTextChanged();
    emit pageCountChanged();
}

QString TranslatorBackend::sourcePageText(int page) const
{
    if (page < 1 || page > m_sourcePages.size()) return QString();
    return m_sourcePages.at(page - 1);
}

QString TranslatorBackend::translatedPageText(int page) const
{
    if (page < 1 || page > m_translatedPages.size()) return QString();
    return m_translatedPages.at(page - 1);
}

QString TranslatorBackend::translatedPageHtml(int page) const
{
    if (page < 1 || page > m_translatedPageHtml.size()) return QString();
    return m_translatedPageHtml.at(page - 1);
}

bool TranslatorBackend::parseTranslatedBlocks(const QString& raw, QJsonArray* blocksOut) const
{
    if (!blocksOut)
        return false;

    QString jsonText = raw.trimmed();
    if (jsonText.startsWith(QStringLiteral("```"))) {
        const int firstLine = jsonText.indexOf(QLatin1Char('\n'));
        const int closing = jsonText.lastIndexOf(QStringLiteral("```"));
        if (firstLine > 0 && closing > firstLine)
            jsonText = jsonText.mid(firstLine + 1, closing - firstLine - 1).trimmed();
    }

    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(jsonText.toUtf8(), &parseError);
    if (doc.isObject()) {
        const QJsonArray blocks = doc.object().value(QStringLiteral("blocks")).toArray();
        if (!blocks.isEmpty()) {
            *blocksOut = blocks;
            return true;
        }
    }

    return false;
}

void TranslatorBackend::applyTranslatedBlocks(int pageIndex, const QJsonArray& blocks)
{
    while (m_translatedPageBlocks.size() <= pageIndex)
        m_translatedPageBlocks << QJsonArray();
    while (m_translatedPageHtml.size() <= pageIndex)
        m_translatedPageHtml << QString();
    while (m_translatedPages.size() <= pageIndex)
        m_translatedPages << QString();

    m_translatedPageBlocks[pageIndex] = blocks;
    m_translatedPageHtml[pageIndex] = DocumentFormat::blocksToHtml(blocks);
    m_translatedPages[pageIndex] = DocumentFormat::blocksToPlainText(blocks);
    emit translatedTextChanged();
}

void TranslatorBackend::applyLayoutPageTranslations(int pageIndex, const QString& response)
{
    QJsonArray pages = m_pdfLayout.value(QStringLiteral("pages")).toArray();
    if (pageIndex < 0 || pageIndex >= pages.size())
        return;

    QHash<QString, QString> translatedById;
    const QStringList lines = response.split(QLatin1Char('\n'), Qt::SkipEmptyParts);
    for (const QString& line : lines) {
        const int pipe = line.indexOf(QLatin1Char('|'));
        if (pipe <= 0)
            continue;
        const QString id = line.left(pipe).trimmed();
        const QString text = line.mid(pipe + 1).trimmed();
        if (!id.isEmpty() && !text.isEmpty())
            translatedById.insert(id, text);
    }

    QJsonObject page = pages.at(pageIndex).toObject();
    QJsonArray items = page.value(QStringLiteral("items")).toArray();
    QJsonArray updatedItems;
    QJsonArray tables = page.value(QStringLiteral("tables")).toArray();
    QJsonArray updatedTables;

    auto applyToObject = [&](QJsonObject obj) {
        const QString id = obj.value(QStringLiteral("id")).toString();
        const QString source = obj.value(QStringLiteral("text")).toString().trimmed();
        if (translatedById.contains(id))
            obj.insert(QStringLiteral("text_translated"), translatedById.value(id));
        else if (!source.isEmpty() && !obj.contains(QStringLiteral("text_translated")))
            obj.insert(QStringLiteral("text_translated"), obj.value(QStringLiteral("text")));
        return obj;
    };

    if (translatedById.isEmpty()) {
        QStringList plainLines;
        for (const QString& line : lines) {
            const QString trimmed = line.trimmed();
            if (!trimmed.isEmpty())
                plainLines << trimmed;
        }
        int lineIdx = 0;
        for (const QJsonValue& value : items) {
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
        for (const QJsonValue& tableValue : tables) {
            QJsonObject table = tableValue.toObject();
            QJsonArray cells = table.value(QStringLiteral("cells")).toArray();
            QJsonArray updatedCells;
            for (const QJsonValue& cellValue : cells) {
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
    } else {
        for (const QJsonValue& value : items)
            updatedItems.append(applyToObject(value.toObject()));
        for (const QJsonValue& tableValue : tables) {
            QJsonObject table = tableValue.toObject();
            QJsonArray cells = table.value(QStringLiteral("cells")).toArray();
            QJsonArray updatedCells;
            for (const QJsonValue& cellValue : cells)
                updatedCells.append(applyToObject(cellValue.toObject()));
            table.insert(QStringLiteral("cells"), updatedCells);
            updatedTables.append(table);
        }
    }

    page.insert(QStringLiteral("items"), updatedItems);
    page.insert(QStringLiteral("tables"), updatedTables);
    pages[pageIndex] = page;
    m_pdfLayout.insert(QStringLiteral("pages"), pages);
}

void TranslatorBackend::applySegmentTranslations(const QString& response)
{
    if (m_workflowMeta.isEmpty())
        return;

    QHash<QString, QString> translatedById;
    const QStringList lines = response.split(QLatin1Char('\n'), Qt::SkipEmptyParts);
    for (const QString& line : lines) {
        const int pipe = line.indexOf(QLatin1Char('|'));
        if (pipe <= 0)
            continue;
        const QString id = line.left(pipe).trimmed();
        const QString text = line.mid(pipe + 1).trimmed();
        if (!id.isEmpty() && !text.isEmpty())
            translatedById.insert(id, text);
    }
    if (translatedById.isEmpty())
        return;

    QJsonArray segments = m_workflowMeta.value(QStringLiteral("segments")).toArray();
    QJsonArray updated;
    for (const QJsonValue& value : segments) {
        QJsonObject item = value.toObject();
        const QString id = item.value(QStringLiteral("id")).toString();
        if (translatedById.contains(id))
            item.insert(QStringLiteral("text_translated"), translatedById.value(id));
        updated.append(item);
    }
    m_workflowMeta.insert(QStringLiteral("segments"), updated);
}

void TranslatorBackend::syncTranslatedTextFromSegments()
{
    const QJsonArray segments = m_workflowMeta.value(QStringLiteral("segments")).toArray();
    if (segments.isEmpty())
        return;

    QStringList parts;
    parts.reserve(segments.size());
    for (const QJsonValue& value : segments) {
        const QJsonObject item = value.toObject();
        const QString translated = item.value(QStringLiteral("text_translated")).toString().trimmed();
        const QString source = item.value(QStringLiteral("text")).toString().trimmed();
        if (!translated.isEmpty())
            parts << translated;
        else if (!source.isEmpty())
            parts << source;
    }
    m_translatedText = parts.join(QStringLiteral("\n\n"));
    if (m_translatedPages.isEmpty())
        m_translatedPages << m_translatedText;
    else
        m_translatedPages[0] = m_translatedText;
    emit translatedTextChanged();
}

void TranslatorBackend::syncTranslatedPagesFromLayout()
{
    if (m_pdfLayout.isEmpty())
        return;

    const QJsonArray pages = m_pdfLayout.value(QStringLiteral("pages")).toArray();
    if (pages.isEmpty())
        return;

    QStringList pageTexts;
    pageTexts.reserve(pages.size());
    for (const QJsonValue& pageValue : pages) {
        const QJsonObject page = pageValue.toObject();
        QStringList lines;
        for (const QJsonValue& itemValue : page.value(QStringLiteral("items")).toArray()) {
            const QJsonObject item = itemValue.toObject();
            const QString translated =
                item.value(QStringLiteral("text_translated")).toString().trimmed();
            const QString source = item.value(QStringLiteral("text")).toString().trimmed();
            if (!translated.isEmpty())
                lines << translated;
            else if (!source.isEmpty())
                lines << source;
        }
        for (const QJsonValue& tableValue : page.value(QStringLiteral("tables")).toArray()) {
            for (const QJsonValue& cellValue :
                 tableValue.toObject().value(QStringLiteral("cells")).toArray()) {
                const QJsonObject cell = cellValue.toObject();
                const QString translated =
                    cell.value(QStringLiteral("text_translated")).toString().trimmed();
                const QString source = cell.value(QStringLiteral("text")).toString().trimmed();
                if (!translated.isEmpty())
                    lines << translated;
                else if (!source.isEmpty())
                    lines << source;
            }
        }
        pageTexts << lines.join(QStringLiteral("\n"));
    }

    m_translatedPages = pageTexts;
    m_pageCount = qMax(m_pageCount, pageTexts.size());
    m_translatedText = pageTexts.join(QStringLiteral("\n\n"));
    emit pageCountChanged();
    emit translatedTextChanged();
}

void TranslatorBackend::clearPdfWorkCopy()
{
    if (m_pdfWorkCopyPath.isEmpty())
        return;
    QFile::remove(m_pdfWorkCopyPath);
    m_pdfWorkCopyPath.clear();
    emit pdfPreviewUrlChanged();
}

bool TranslatorBackend::ensurePdfWorkCopy()
{
    if (!m_isPdf || m_filePath.isEmpty())
        return false;
    if (!m_pdfWorkCopyPath.isEmpty() && QFile::exists(m_pdfWorkCopyPath))
        return true;

    const QString copy = DocumentLoader::copyPdfForPythonWork(m_filePath);
    if (copy.isEmpty())
        return false;

    m_pdfWorkCopyPath = copy;
    emit pdfPreviewUrlChanged();
    return true;
}

void TranslatorBackend::buildTranslatedPdfAsync()
{
    DocumentLoader::ensureReady();

    if (!ensurePdfWorkCopy()) {
        if (!m_translatedText.isEmpty())
            animateWords(m_translatedText);
        m_progress = 100;
        emit progressChanged();
        setStatus(QStringLiteral("Перевод готов; PDF: не удалось подготовить копию файла"));
        if (m_etaTimer)
            m_etaTimer->stop();
        setBusy(false);
        m_activeReplies.clear();
        return;
    }

    const QString tempDir =
        QStandardPaths::writableLocation(QStandardPaths::TempLocation) + QStringLiteral("/Etemenanki");
    QDir().mkpath(tempDir);
    const QString outputPath =
        tempDir + QLatin1Char('/') + QUuid::createUuid().toString(QUuid::WithoutBraces) + QStringLiteral("_ru.pdf");

    setStatus(QStringLiteral("Сборка переведённого PDF..."));

    const QString sourcePath = m_filePath;
    const QJsonObject layout = m_pdfLayout;
    const QStringList translatedPages = m_translatedPages;
    const QString workCopyPath = m_pdfWorkCopyPath;

    QTimer::singleShot(0, this, [this, sourcePath, layout, translatedPages, outputPath, workCopyPath]() {
        const PdfBuildResult buildResult = DocumentLoader::buildTranslatedPdfFromPages(
            sourcePath, layout, translatedPages, outputPath, workCopyPath);

        if (buildResult.ok) {
            if (!buildResult.layout.isEmpty())
                m_pdfLayout = buildResult.layout;
            m_translatedPdfPath = buildResult.outputPath;
            syncTranslatedPagesFromLayout();
            emit translatedPdfPathChanged();
            m_progress = 100;
            emit progressChanged();
            setStatus(QStringLiteral("Перевод завершен — PDF готов (%1 стр.)").arg(m_pageCount));
            animateWords(m_translatedText);
        } else {
            if (!m_translatedText.isEmpty())
                animateWords(m_translatedText);
            m_progress = 100;
            emit progressChanged();
            setStatus(buildResult.error.isEmpty()
                          ? QStringLiteral("Перевод готов, но PDF не собран")
                          : QStringLiteral("Перевод готов; PDF: %1").arg(buildResult.error.left(380)));
        }

        if (m_etaTimer)
            m_etaTimer->stop();
        setBusy(false);
        m_activeReplies.clear();
    });
}

void TranslatorBackend::retryTranslatedPdfBuild()
{
    if (!m_isPdf || m_filePath.isEmpty() || m_busy)
        return;
    if (m_translatedPages.isEmpty() && m_translatedText.trimmed().isEmpty())
        return;
    setBusy(true);
    buildTranslatedPdfAsync();
}

void TranslatorBackend::ensurePageCount(int count)
{
    const int n = qMax(1, qMin(count, 120));
    if (n <= m_pageCount)
        return;

    // Do not pad empty page slots over text that was already extracted.
    if (!combinedSourceText().isEmpty() && m_sourcePages.size() >= m_pageCount)
        return;

    m_pageCount = n;
    while (m_sourcePages.size() < n) {
        m_sourcePages << QString();
    }
    while (m_translatedPages.size() < n) {
        m_translatedPages << QString();
    }
    emit pageCountChanged();
}

void TranslatorBackend::loadFile(const QString& path)
{
    const QString filePath = normalizeLocalFilePath(path);
    const QFileInfo info(filePath);
    if (!info.exists() || !info.isFile()) {
        setStatus(QStringLiteral("Файл не найден: %1").arg(filePath));
        return;
    }

    const int generation = ++m_loadGeneration;
    setStatus(QStringLiteral("Загрузка и извлечение текста..."));
    setBusy(true);

    const QFileInfo fileInfo = info;
    auto* watcher = new QFutureWatcher<DocumentLoadResult>(this);
    connect(watcher, &QFutureWatcher<DocumentLoadResult>::finished, this,
            [this, watcher, fileInfo, generation]() {
                watcher->deleteLater();
                if (generation != m_loadGeneration)
                    return;
                applyLoadedDocument(watcher->result(), fileInfo);
                setBusy(false);
            });
    watcher->setFuture(QtConcurrent::run([filePath]() { return DocumentLoader::load(filePath); }));
}

void TranslatorBackend::applyLoadedDocument(const DocumentLoadResult& doc, const QFileInfo& fileInfo)
{
    const QString filePath = fileInfo.absoluteFilePath();
    const bool isPdf = fileInfo.suffix().compare(QStringLiteral("pdf"), Qt::CaseInsensitive) == 0;
    const QString suffix = fileInfo.suffix().toLower();
    updateWorkflowInfo(suffix);

    m_workflowMeta = doc.workflowMeta;
    if (!doc.workflowId.isEmpty())
        m_workflowId = doc.workflowId;

    const bool hasSegments = m_workflowMeta.contains(QStringLiteral("segments"))
        && !m_workflowMeta.value(QStringLiteral("segments")).toArray().isEmpty();
    const bool segmentMode = hasSegments
        && (m_workflowId == QStringLiteral("subtitle")
            || m_workflowId == QStringLiteral("spreadsheet")
            || m_workflowId == QStringLiteral("json")
            || m_workflowId == QStringLiteral("docx"));
    if (m_segmentMode != segmentMode) {
        m_segmentMode = segmentMode;
    }

    const bool hasPdfLayout = doc.ok && !doc.pdfLayout.isEmpty();
    if (m_pdfLayoutMode != hasPdfLayout) {
        m_pdfLayoutMode = hasPdfLayout;
    }
    m_pdfLayout = doc.pdfLayout;
    m_translatedPdfPath.clear();
    emit translatedPdfPathChanged();

    const bool structured = doc.ok && !doc.pageBlocks.isEmpty() && !hasPdfLayout;
    if (m_structuredDocument != structured) {
        m_structuredDocument = structured;
        emit contentIsStructuredChanged();
    }
    m_sourcePageBlocks = doc.pageBlocks;

    const bool formatted = doc.ok
        && (hasPdfLayout || structured || segmentMode || suffix == QStringLiteral("pdf")
            || suffix == QStringLiteral("docx")
            || suffix == QStringLiteral("xlsx")
            || suffix == QStringLiteral("csv")
            || suffix == QStringLiteral("html")
            || suffix == QStringLiteral("htm")
            || doc.encoding.contains(QStringLiteral("markdown"), Qt::CaseInsensitive)
            || doc.encoding == QStringLiteral("pymupdf-md"));
    if (m_documentFormatted != formatted) {
        m_documentFormatted = formatted;
        emit documentFormattedChanged();
    }

    m_fileName = fileInfo.fileName();
    const QString absolutePath = fileInfo.absoluteFilePath();
    clearPdfWorkCopy();
    m_filePath = absolutePath;
    m_isPdf = isPdf;

    QString pdfCopyWarning;
    if (isPdf && doc.ok) {
        m_pdfWorkCopyPath = DocumentLoader::copyPdfForPythonWork(absolutePath);
        if (m_pdfWorkCopyPath.isEmpty()) {
            pdfCopyWarning =
                QStringLiteral(" (копия не создана — см. %1)").arg(DocumentLoader::pythonDebugLogPath());
        } else {
            emit pdfPreviewUrlChanged();
        }
    }

    if (!doc.ok) {
        if (!isPdf) {
            m_fileName.clear();
            m_filePath.clear();
            m_isPdf = false;
            setStatus(doc.error);
            return;
        }
        m_sourcePages = {QString()};
        m_pageCount = 1;
        m_sourceText.clear();
        const QString pyHint = DocumentLoader::activePythonPath().isEmpty()
            ? QString()
            : QStringLiteral("\nPython: %1").arg(DocumentLoader::activePythonPath());
        setStatus(doc.error + pyHint
                  + QStringLiteral("\nПревью PDF доступно; текст для перевода не извлечён."));
    } else {
        m_sourcePages = doc.pages;
        m_pageCount = qMax(qMax(1, doc.pageCount), m_sourcePages.size());
        m_sourceText = doc.text;
        if (m_sourceText.trimmed().isEmpty()) {
            QStringList parts;
            for (const QString& page : std::as_const(m_sourcePages)) {
                const QString trimmed = page.trimmed();
                if (!trimmed.isEmpty())
                    parts << trimmed;
            }
            m_sourceText = parts.join(QStringLiteral("\n\n"));
        }
        if (m_sourceText.trimmed().isEmpty()) {
            if (!isPdf) {
                m_fileName.clear();
                m_filePath.clear();
                m_isPdf = false;
            }
            setStatus(QStringLiteral("Текст не найден в документе (скан без текстового слоя?)"));
        } else {
            const QString enc = doc.encoding.isEmpty() ? QString() : QStringLiteral(" (%1)").arg(doc.encoding);
            const QString mode = hasPdfLayout ? QStringLiteral(" [PDF layout]")
                : (m_structuredDocument ? QStringLiteral(" [структура]")
                                         : (m_segmentMode ? QStringLiteral(" [сегменты]") : QString()));
            setStatus(QStringLiteral("%1 — %2 (%3 стр.)%4")
                          .arg(m_workflowTitle, m_workflowPipeline)
                          .arg(m_pageCount)
                          .arg(pdfCopyWarning));
        }
    }

    m_translatedText.clear();
    m_translatedPages.clear();
    m_translatedPageBlocks.clear();
    m_translatedPageHtml.clear();
    for (int i = 0; i < m_pageCount; ++i) {
        m_translatedPages << QString();
        m_translatedPageHtml << QString();
    }

    emit sourceTextChanged();
    emit translatedTextChanged();
    emit fileNameChanged();
    emit filePathChanged();
    emit pageCountChanged();
}

void TranslatorBackend::saveResult(const QString& path)
{
    QString filePath = normalizeLocalFilePath(path);
    if (filePath.isEmpty()) {
        setStatus(QStringLiteral("Не выбран файл для сохранения"));
        return;
    }

    if (m_isPdf && hasTranslatedPdf()) {
        if (!filePath.endsWith(QStringLiteral(".pdf"), Qt::CaseInsensitive))
            filePath += QStringLiteral(".pdf");
        if (QFile::exists(filePath))
            QFile::remove(filePath);
        if (QFile::copy(m_translatedPdfPath, filePath)) {
            setStatus(QStringLiteral("PDF сохранён: %1").arg(QFileInfo(filePath).fileName()));
            return;
        }
        setStatus(QStringLiteral("Не удалось сохранить PDF"));
        return;
    }

    if (m_workflowRoundTrip && !m_workflowMeta.isEmpty() && !m_filePath.isEmpty()) {
        const ExportDocumentResult exported = DocumentLoader::exportDocument(
            m_filePath, filePath, m_workflowId, m_workflowMeta);
        if (exported.ok) {
            setStatus(QStringLiteral("Файл сохранён: %1").arg(QFileInfo(exported.outputPath).fileName()));
            return;
        }
        if (!exported.error.isEmpty())
            setStatus(QStringLiteral("Экспорт: %1 — сохраняю как TXT").arg(exported.error.left(180)));
    }

    QFileInfo outInfo(filePath);
    if (!outInfo.suffix().isEmpty()
        && outInfo.suffix().compare(QStringLiteral("pdf"), Qt::CaseInsensitive) == 0) {
        filePath = QDir(outInfo.path()).filePath(outInfo.completeBaseName() + QStringLiteral(".txt"));
    } else if (outInfo.suffix().isEmpty()) {
        filePath += QStringLiteral(".txt");
    }

    QFile f(filePath);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        setStatus(QStringLiteral("Не удалось сохранить файл"));
        return;
    }
    f.write(m_translatedText.toUtf8());
    f.close();
    setStatus(QStringLiteral("Файл сохранён: %1").arg(QFileInfo(filePath).fileName()));
}

void TranslatorBackend::cancelTranslate()
{
    if (!m_busy) return;
    m_cancelled = true;
    for (QNetworkReply* reply : std::as_const(m_activeReplies)) {
        if (reply)
            reply->abort();
    }
    m_activeReplies.clear();
    stopWordAnimation();
    if (m_etaTimer)
        m_etaTimer->stop();
    setBusy(false);
    setStatus(QStringLiteral("Перевод отменен"));
}

void TranslatorBackend::startTranslate(const QString& runtime,
                                       const QString& model,
                                       const QString& sourceLang,
                                       const QString& targetLang)
{
    const QString baseUrl =
        m_appSettings ? m_appSettings->cloudBaseUrlForModel(model) : m_baseUrl;
    const QString apiKey = m_appSettings ? m_appSettings->cloudApiKeyForModel(model) : m_apiKey;
    if (m_busy) return;
    if (combinedSourceText().isEmpty()) {
        if (!m_fileName.isEmpty() && (m_isPdf || m_fileName.endsWith(QStringLiteral(".docx"), Qt::CaseInsensitive)))
            setStatus(QStringLiteral("Текст не извлечён — загрузите файл снова или установите pymupdf: pip install pymupdf"));
        else
            setStatus(QStringLiteral("Нет текста для перевода"));
        return;
    }
    if (m_sourceText.trimmed().isEmpty())
        m_sourceText = combinedSourceText();

    m_pendingRuntime = runtime;
    m_pendingModel = model;
    m_pendingSourceLang = sourceLang;
    m_pendingTargetLang = targetLang;
    m_pendingBaseUrl = baseUrl;
    m_pendingApiKey = apiKey;

    if (runtime == QStringLiteral("local")) {
        setStatus(QStringLiteral("Проверка Ollama..."));
        QNetworkRequest pingReq(QUrl(ollamaApiUrl(QStringLiteral("/api/tags"))));
        pingReq.setTransferTimeout(4000);
        QNetworkReply* pingReply = m_net.get(pingReq);
        connect(pingReply, &QNetworkReply::finished, this, [this, pingReply]() {
            const bool ollamaUp = pingReply->error() == QNetworkReply::NoError;
            pingReply->deleteLater();
            if (!ollamaUp) {
                setStatus(QStringLiteral("Запустите Ollama: ollama serve"));
                return;
            }
            proceedWithTranslate();
        });
        return;
    }

    if (apiKey.trimmed().isEmpty()) {
        setStatus(QStringLiteral("Укажите API Key для облачного перевода"));
        return;
    }

    proceedWithTranslate();
}

void TranslatorBackend::proceedWithTranslate()
{
    m_runtime = m_pendingRuntime;
    m_model = m_pendingModel;
    m_sourceLang = m_pendingSourceLang;
    m_targetLang = m_pendingTargetLang;
    m_baseUrl = m_pendingBaseUrl;
    m_apiKey = m_pendingApiKey;

    const QString pdfEngine =
        m_appSettings ? m_appSettings->pdfEngine() : QStringLiteral("etemenanki");
    if (m_isPdf && pdfEngine != QStringLiteral("etemenanki")) {
        runExternalPdfEngine(pdfEngine);
        return;
    }

    const bool layoutAuto = !m_appSettings || m_appSettings->pdfLayoutAuto();
    if (m_isPdf && layoutAuto && m_pdfLayout.isEmpty() && !m_filePath.isEmpty()) {
        setBusy(true);
        setStatus(QStringLiteral("Извлечение структуры PDF (таблицы, лого)..."));

        QTimer::singleShot(0, this, [this]() {
            if (!ensurePdfWorkCopy()) {
                setStatus(QStringLiteral("PDF: не удалось подготовить копию для извлечения структуры"));
                runTranslateJob();
                return;
            }

            const DocumentLoadResult layoutResult =
                DocumentLoader::extractPdfLayout(m_filePath, m_pdfWorkCopyPath);

            if (layoutResult.ok && !layoutResult.pdfLayout.isEmpty()) {
                m_pdfLayout = layoutResult.pdfLayout;
                m_pdfLayoutMode = true;
                if (!layoutResult.pages.isEmpty()) {
                    m_sourcePages = layoutResult.pages;
                    m_pageCount = qMax(m_pageCount, layoutResult.pageCount);
                    emit pageCountChanged();
                }
            } else if (!layoutResult.error.isEmpty()) {
                setStatus(QStringLiteral("PDF layout: %1").arg(layoutResult.error.left(160)));
            }

            runTranslateJob();
        });
        return;
    }

    runTranslateJob();
}

void TranslatorBackend::runExternalPdfEngine(const QString& engine)
{
    m_translatedPdfPath.clear();
    emit translatedPdfPathChanged();
    m_progress = 0;
    emit progressChanged();
    setBusy(true);

    QString engineLabel = engine;
    if (engine == QStringLiteral("pdfmathtranslate"))
        engineLabel = QStringLiteral("PDFMathTranslate");
    else if (engine == QStringLiteral("polyglotpdf"))
        engineLabel = QStringLiteral("PolyglotPDF");
    else if (engine == QStringLiteral("retainpdf"))
        engineLabel = QStringLiteral("RetainPDF");

    setStatus(QStringLiteral("PDF: %1…").arg(engineLabel));

    if (!ensurePdfWorkCopy()) {
        setStatus(QStringLiteral("PDF: не удалось подготовить копию файла"));
        setBusy(false);
        return;
    }

    const QString tempDir =
        QStandardPaths::writableLocation(QStandardPaths::TempLocation) + QStringLiteral("/Etemenanki");
    QDir().mkpath(tempDir);
    const QString outputPath =
        tempDir + QLatin1Char('/') + QUuid::createUuid().toString(QUuid::WithoutBraces) + QStringLiteral("_ext.pdf");

    ExternalPdfTranslateRequest req;
    req.engine = engine;
    req.inputPath = m_pdfWorkCopyPath.isEmpty() ? m_filePath : m_pdfWorkCopyPath;
    req.outputPath = outputPath;
    req.srcLang = m_sourceLang;
    req.dstLang = m_targetLang;
    req.runtime = m_runtime;
    req.model = m_model;
    if (m_appSettings) {
        req.ollamaUrl = m_appSettings->ollamaBaseUrl();
    }
    req.cloudBase = m_baseUrl;
    req.cloudKey = m_apiKey;

    if (!m_etaTimer) {
        m_etaTimer = new QTimer(this);
        m_etaTimer->setInterval(1000);
        connect(m_etaTimer, &QTimer::timeout, this, &TranslatorBackend::updateTranslationEta);
    }
    m_translateTimer.restart();
    m_etaTimer->start();

    QTimer::singleShot(0, this, [this, req, engineLabel]() {
        const ExternalPdfTranslateResult result = DocumentLoader::translatePdfExternal(req);
        if (result.ok) {
            m_translatedPdfPath = result.outputPath;
            emit translatedPdfPathChanged();
            m_translatedText = QStringLiteral("PDF переведён движком %1. Откройте «Исходный формат».").arg(engineLabel);
            emit translatedTextChanged();
            m_progress = 100;
            emit progressChanged();
            setStatus(QStringLiteral("PDF готов — %1").arg(engineLabel));
            animateWords(m_translatedText);
        } else {
            m_progress = 0;
            emit progressChanged();
            setStatus(result.error.isEmpty()
                          ? QStringLiteral("Внешний PDF-движок завершился с ошибкой")
                          : result.error.left(420));
        }

        if (m_etaTimer)
            m_etaTimer->stop();
        setBusy(false);
        m_activeReplies.clear();
    });
}

void TranslatorBackend::runTranslateJob()
{
    m_translatedPdfPath.clear();
    emit translatedPdfPathChanged();

    buildTranslationUnits();
    if (m_chunks.isEmpty()
        || std::all_of(m_chunks.cbegin(), m_chunks.cend(),
                       [](const QString& s) { return s.trimmed().isEmpty(); })) {
        setStatus(QStringLiteral("Нет текста для перевода"));
        setBusy(false);
        return;
    }

    m_translatedChunks = QStringList(m_chunks.size());
    m_currentChunk = 0;
    m_inFlight = 0;
    m_completedChunks = 0;
    m_nextDispatchIndex = 0;
    m_activeReplies.clear();
    m_translatedText.clear();
    m_translatedPageBlocks.clear();
    m_translatedPageHtml.clear();
    m_translatedPages = QStringList(qMax(1, m_pageCount), QString());
    m_translatedPageHtml = QStringList(qMax(1, m_pageCount), QString());
    m_words.clear();
    m_progress = 1;
    m_cancelled = false;
    m_translateTimer.start();
    if (!m_etaTimer) {
        m_etaTimer = new QTimer(this);
        m_etaTimer->setInterval(1000);
        connect(m_etaTimer, &QTimer::timeout, this, &TranslatorBackend::updateTranslationEta);
    }
    m_etaTimer->start();
    emit wordsChanged();
    emit translatedTextChanged();
    emit progressChanged();
    updateTranslationEta();
    setStatus(QStringLiteral("Перевод %1 стр...").arg(m_pageCount));
    setBusy(true);
    updateModelInfo(m_model);

    dispatchTranslationChunks();
}

void TranslatorBackend::finishTranslation()
{
    if (m_segmentMode)
        syncTranslatedTextFromSegments();

    if (m_isPdf && !m_filePath.isEmpty()) {
        if (!m_translatedPages.isEmpty()) {
            QStringList pageTexts;
            pageTexts.reserve(m_translatedPages.size());
            for (const QString& page : std::as_const(m_translatedPages))
                pageTexts << page.trimmed();
            m_translatedText = pageTexts.join(QStringLiteral("\n\n"));
            emit translatedTextChanged();
        } else if (!m_translatedChunks.isEmpty()) {
            m_translatedText = m_translatedChunks.join(QStringLiteral("\n\n"));
            emit translatedTextChanged();
        }
        buildTranslatedPdfAsync();
        return;
    }

    if (!m_translatedPages.isEmpty()) {
        QStringList pageTexts;
        pageTexts.reserve(m_translatedPages.size());
        for (const QString& page : std::as_const(m_translatedPages)) {
            pageTexts << page.trimmed();
        }
        m_translatedText = pageTexts.join(QStringLiteral("\n\n"));
    } else {
        QStringList ordered;
        ordered.reserve(m_translatedChunks.size());
        for (const QString& chunk : m_translatedChunks) {
            if (!chunk.trimmed().isEmpty())
                ordered << chunk.trimmed();
        }
        m_translatedText = ordered.join(QStringLiteral("\n\n"));
        if (m_translatedPages.isEmpty() || m_translatedPages.size() == 1)
            m_translatedPages = {m_translatedText};
    }

    emit translatedTextChanged();
    animateWords(m_translatedText);
    m_progress = 100;
    emit progressChanged();
    updateTranslationEta();
    if (m_etaTimer)
        m_etaTimer->stop();
    setStatus(QStringLiteral("Перевод завершен (%1 стр.)").arg(m_translatedPages.size()));
    setBusy(false);
    m_activeReplies.clear();
}

void TranslatorBackend::dispatchTranslationChunks()
{
    if (m_cancelled)
        return;

    const int maxConcurrent =
        m_appSettings ? qBound(1, m_appSettings->translateConcurrent(), 6) : 1;

    while (m_inFlight < maxConcurrent && m_nextDispatchIndex < m_chunks.size()) {
        while (m_nextDispatchIndex < m_chunks.size()
               && m_chunks.at(m_nextDispatchIndex).trimmed().isEmpty()) {
            m_translatedChunks[m_nextDispatchIndex] = QString();
            ++m_completedChunks;
            ++m_nextDispatchIndex;
        }
        if (m_nextDispatchIndex >= m_chunks.size())
            break;
        startChunkAt(m_nextDispatchIndex);
        ++m_nextDispatchIndex;
    }

    if (m_completedChunks >= m_chunks.size() && m_inFlight == 0)
        finishTranslation();
}

void TranslatorBackend::startChunkAt(int chunkIndex)
{
    if (m_cancelled || chunkIndex < 0 || chunkIndex >= m_chunks.size())
        return;

    const QString chunk = m_chunks.at(chunkIndex);
    const bool layoutChunk = m_pdfLayoutMode;
    const bool segmentChunk = m_segmentMode;
    const bool structuredChunk =
        !layoutChunk && !segmentChunk && m_structuredDocument
        && chunk.trimmed().startsWith(QLatin1Char('{'));

    const QString prompt =
        buildPromptForChunk(chunk, layoutChunk, structuredChunk, segmentChunk);

    QNetworkRequest req;
    QJsonObject payload;
    QUrl url;
    if (m_runtime == QStringLiteral("local")) {
        url = QUrl(ollamaApiUrl(QStringLiteral("/api/chat")));
        payload[QStringLiteral("model")] = m_model;
        payload[QStringLiteral("stream")] = false;
        payload[QStringLiteral("messages")] = QJsonArray{
            QJsonObject{{QStringLiteral("role"), QStringLiteral("user")},
                        {QStringLiteral("content"), prompt}}};
    } else {
        url = QUrl(m_baseUrl + QStringLiteral("/chat/completions"));
        req.setRawHeader("Authorization", QByteArray("Bearer ") + m_apiKey.toUtf8());
        payload[QStringLiteral("model")] = m_model;
        payload[QStringLiteral("temperature")] = 0.2;
        payload[QStringLiteral("messages")] = QJsonArray{
            QJsonObject{{QStringLiteral("role"), QStringLiteral("user")},
                        {QStringLiteral("content"), prompt}}};
    }
    req.setUrl(url);
    req.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));

    QNetworkReply* reply = m_net.post(req, QJsonDocument(payload).toJson(QJsonDocument::Compact));
    m_activeReplies << reply;
    ++m_inFlight;

    connect(reply, &QNetworkReply::finished, this,
            [this, reply, chunkIndex, layoutChunk, structuredChunk, segmentChunk]() {
                m_activeReplies.removeAll(reply);

                if (m_cancelled) {
                    reply->deleteLater();
                    --m_inFlight;
                    return;
                }

                const QByteArray raw = reply->readAll();
                const auto err = reply->error();
                const QString errText = reply->errorString();
                reply->deleteLater();
                --m_inFlight;

                if (err != QNetworkReply::NoError) {
                    setStatus(QStringLiteral("Ошибка сети: ") + errText);
                    setBusy(false);
                    m_activeReplies.clear();
                    return;
                }

                const QJsonDocument doc = QJsonDocument::fromJson(raw);
                QString translated;
                if (m_runtime == QStringLiteral("local")) {
                    translated = doc.object()
                                     .value(QStringLiteral("message"))
                                     .toObject()
                                     .value(QStringLiteral("content"))
                                     .toString()
                                     .trimmed();
                } else {
                    const QJsonArray choices = doc.object().value(QStringLiteral("choices")).toArray();
                    const QJsonObject firstChoice =
                        choices.isEmpty() ? QJsonObject() : choices.at(0).toObject();
                    translated = firstChoice.value(QStringLiteral("message"))
                                     .toObject()
                                     .value(QStringLiteral("content"))
                                     .toString()
                                     .trimmed();
                }

                handleChunkFinished(chunkIndex, translated, layoutChunk, structuredChunk, segmentChunk);
            });
}

void TranslatorBackend::handleChunkFinished(int chunkIndex,
                                            const QString& translated,
                                            bool layoutChunk,
                                            bool structuredChunk,
                                            bool segmentChunk)
{
    if (chunkIndex >= 0 && chunkIndex < m_translatedChunks.size())
        m_translatedChunks[chunkIndex] = translated;

    if (layoutChunk && chunkIndex < m_chunkPageIndex.size())
        applyLayoutPageTranslations(m_chunkPageIndex.at(chunkIndex), translated);
    else if (segmentChunk)
        applySegmentTranslations(translated);
    else if (chunkIndex < m_chunkPageIndex.size()) {
        const int pageIdx = m_chunkPageIndex.at(chunkIndex);
        if (structuredChunk && pageIdx >= 0) {
            QJsonArray blocks;
            if (parseTranslatedBlocks(translated, &blocks)) {
                applyTranslatedBlocks(pageIdx, blocks);
            } else {
                QJsonArray fallback;
                QJsonObject paragraph;
                paragraph.insert(QStringLiteral("kind"), QStringLiteral("paragraph"));
                paragraph.insert(QStringLiteral("indent"), 0);
                paragraph.insert(QStringLiteral("text"), translated);
                fallback.append(paragraph);
                applyTranslatedBlocks(pageIdx, fallback);
            }
        } else if (pageIdx >= 0 && pageIdx < m_translatedPages.size()) {
            if (!m_translatedPages[pageIdx].isEmpty())
                m_translatedPages[pageIdx] += QStringLiteral("\n\n");
            m_translatedPages[pageIdx] += translated;
            emit translatedTextChanged();
        }
    }

    ++m_completedChunks;
    m_progress = qMax(1, static_cast<int>((m_completedChunks * 100.0) / qMax(1, m_chunks.size())));
    emit progressChanged();
    updateTranslationEta();
    const int pageNum = chunkIndex < m_chunkPageIndex.size() ? m_chunkPageIndex.at(chunkIndex) + 1
                                                             : m_completedChunks;
    setStatus(QStringLiteral("Перевод: %1% (стр. %2/%3)")
                  .arg(m_progress)
                  .arg(pageNum)
                  .arg(m_pageCount));

    dispatchTranslationChunks();
}

void TranslatorBackend::processNextChunk()
{
    dispatchTranslationChunks();
}

void TranslatorBackend::stopWordAnimation()
{
    if (!m_wordsTimer)
        return;
    m_wordsTimer->stop();
    m_wordsTimer->deleteLater();
    m_wordsTimer = nullptr;
}

void TranslatorBackend::animateWords(const QString& text)
{
    const QStringList allWords = text.split(QRegularExpression(QStringLiteral("\\s+")), Qt::SkipEmptyParts);
    if (allWords.isEmpty())
        return;

    stopWordAnimation();
    m_words.clear();
    emit wordsChanged();

    auto* idx = new int(0);
    m_wordsTimer = new QTimer(this);
    m_wordsTimer->setInterval(35);
    connect(m_wordsTimer, &QTimer::timeout, this, [this, idx, allWords]() {
        if (*idx >= allWords.size()) {
            stopWordAnimation();
            delete idx;
            return;
        }
        m_words.append(allWords.at(*idx));
        emit wordsChanged();
        ++(*idx);
    });
    m_wordsTimer->start();
}

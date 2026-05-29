#include "translation_workflow.h"

namespace {

WorkflowInfo make(const char* id,
                  const char* titleRu,
                  const char* titleEn,
                  const char* pipelineRu,
                  const char* pipelineEn,
                  const char* exportRu,
                  const char* exportEn,
                  bool layoutPreserving = false,
                  bool roundTripExport = false)
{
    WorkflowInfo info;
    info.id = QString::fromUtf8(id);
    info.titleRu = QString::fromUtf8(titleRu);
    info.titleEn = QString::fromUtf8(titleEn);
    info.pipelineRu = QString::fromUtf8(pipelineRu);
    info.pipelineEn = QString::fromUtf8(pipelineEn);
    info.exportRu = QString::fromUtf8(exportRu);
    info.exportEn = QString::fromUtf8(exportEn);
    info.layoutPreserving = layoutPreserving;
    info.roundTripExport = roundTripExport;
    return info;
}

} // namespace

WorkflowInfo TranslationWorkflow::forSuffix(const QString& suffix)
{
    const QString s = suffix.toLower();

    if (s == QStringLiteral("pdf"))
        return make("pdf_layout", "PDF", "PDF",
                    "Извлечение layout → перевод по блокам → сборка PDF",
                    "Layout extract → block translation → PDF rebuild",
                    "PDF с сохранением вёрстки", "Layout-preserving PDF",
                    true, true);

    if (s == QStringLiteral("docx"))
        return make("docx", "Word", "Word",
                    "Параграфы → перевод → DOCX/TXT",
                    "Paragraphs → translate → DOCX/TXT",
                    "DOCX или TXT", "DOCX or TXT",
                    false, true);

    if (s == QStringLiteral("xlsx") || s == QStringLiteral("csv"))
        return make("spreadsheet", "Таблица", "Spreadsheet",
                    "Ячейки → перевод → XLSX/CSV",
                    "Cells → translate → XLSX/CSV",
                    "XLSX/CSV", "XLSX/CSV",
                    false, true);

    if (s == QStringLiteral("srt") || s == QStringLiteral("ass") || s == QStringLiteral("vtt"))
        return make("subtitle", "Субтитры", "Subtitles",
                    "Реплики → перевод → SRT/ASS/VTT",
                    "Cues → translate → SRT/ASS/VTT",
                    "Тот же формат субтитров", "Same subtitle format",
                    false, true);

    if (s == QStringLiteral("json"))
        return make("json", "JSON", "JSON",
                    "Строковые поля → перевод → JSON",
                    "String fields → translate → JSON",
                    "JSON", "JSON",
                    false, true);

    if (s == QStringLiteral("html") || s == QStringLiteral("htm"))
        return make("html", "HTML", "HTML",
                    "Текст страницы → перевод → HTML/TXT",
                    "Page text → translate → HTML/TXT",
                    "HTML или TXT", "HTML or TXT",
                    false, true);

    if (s == QStringLiteral("epub"))
        return make("epub", "EPUB", "EPUB",
                    "Главы → перевод → EPUB/TXT",
                    "Chapters → translate → EPUB/TXT",
                    "EPUB или TXT", "EPUB or TXT",
                    false, true);

    if (s == QStringLiteral("md"))
        return make("markdown", "Markdown", "Markdown",
                    "Markdown → перевод → MD/TXT",
                    "Markdown → translate → MD/TXT",
                    "MD или TXT", "MD or TXT");

    if (s == QStringLiteral("txt"))
        return make("text", "Текст", "Text",
                    "Текст → перевод → TXT",
                    "Text → translate → TXT",
                    "TXT", "TXT");

    return make("unknown", "Документ", "Document",
                "Универсальный перевод текста",
                "Universal text translation",
                "TXT", "TXT");
}

bool TranslationWorkflow::isHubExtractSuffix(const QString& suffix)
{
    const QString s = suffix.toLower();
    static const QStringList hub = {
        QStringLiteral("docx"),
        QStringLiteral("xlsx"),
        QStringLiteral("csv"),
        QStringLiteral("srt"),
        QStringLiteral("ass"),
        QStringLiteral("vtt"),
        QStringLiteral("json"),
        QStringLiteral("html"),
        QStringLiteral("htm"),
        QStringLiteral("epub"),
    };
    return hub.contains(s);
}

QStringList TranslationWorkflow::supportedSuffixes()
{
    return {
        QStringLiteral("txt"),
        QStringLiteral("md"),
        QStringLiteral("docx"),
        QStringLiteral("pdf"),
        QStringLiteral("xlsx"),
        QStringLiteral("csv"),
        QStringLiteral("srt"),
        QStringLiteral("ass"),
        QStringLiteral("vtt"),
        QStringLiteral("json"),
        QStringLiteral("html"),
        QStringLiteral("htm"),
        QStringLiteral("epub"),
    };
}

QString TranslationWorkflow::openFileFilter(bool englishUi)
{
    if (englishUi) {
        return QStringLiteral(
            "All supported (*.txt *.md *.docx *.pdf *.xlsx *.csv *.srt *.ass *.vtt *.json *.html *.htm *.epub);;"
            "Documents (*.txt *.md *.docx *.pdf);;"
            "Subtitles (*.srt *.ass *.vtt);;"
            "Spreadsheets (*.xlsx *.csv);;"
            "Data (*.json);;"
            "Web (*.html *.htm);;"
            "Books (*.epub);;"
            "All files (*)");
    }
    return QStringLiteral(
        "Все форматы (*.txt *.md *.docx *.pdf *.xlsx *.csv *.srt *.ass *.vtt *.json *.html *.htm *.epub);;"
        "Документы (*.txt *.md *.docx *.pdf);;"
        "Субтитры (*.srt *.ass *.vtt);;"
        "Таблицы (*.xlsx *.csv);;"
        "JSON (*.json);;"
        "Веб (*.html *.htm);;"
        "Книги (*.epub);;"
        "Все файлы (*)");
}

QString TranslationWorkflow::saveFileFilter(bool englishUi,
                                            bool pdfWithTranslation,
                                            bool roundTrip)
{
    if (pdfWithTranslation) {
        if (englishUi)
            return QStringLiteral("PDF (*.pdf);;Text (*.txt);;All files (*)");
        return QStringLiteral("PDF (*.pdf);;Текст (*.txt);;Все файлы (*)");
    }
    if (roundTrip) {
        if (englishUi)
            return QStringLiteral("Original format (*.*);;Text (*.txt);;All files (*)");
        return QStringLiteral("Формат оригинала (*.*);;Текст (*.txt);;Все файлы (*)");
    }
    if (englishUi)
        return QStringLiteral("Text (*.txt);;Word (*.docx);;Markdown (*.md);;All files (*)");
    return QStringLiteral("Текст (*.txt);;Word (*.docx);;Markdown (*.md);;Все файлы (*)");
}

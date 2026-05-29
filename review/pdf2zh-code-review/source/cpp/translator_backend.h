#pragma once



#include <QJsonArray>

#include <QObject>

#include <QList>

#include <QNetworkAccessManager>

#include <QElapsedTimer>

#include <QTimer>

#include <QUrl>

#include <QString>

#include <QStringList>

#include <QFileInfo>

#include "document_loader.h"
#include "translation_workflow.h"

class AppSettings;



class QNetworkReply;



class TranslatorBackend final : public QObject

{

    Q_OBJECT

    Q_PROPERTY(QString sourceText READ sourceText WRITE setSourceText NOTIFY sourceTextChanged)

    Q_PROPERTY(QString translatedText READ translatedText NOTIFY translatedTextChanged)

    Q_PROPERTY(QString status READ status NOTIFY statusChanged)

    Q_PROPERTY(int progress READ progress NOTIFY progressChanged)

    Q_PROPERTY(int estimatedRemainingSec READ estimatedRemainingSec NOTIFY estimatedRemainingSecChanged)

    Q_PROPERTY(bool documentFormatted READ documentFormatted NOTIFY documentFormattedChanged)

    Q_PROPERTY(bool contentIsStructured READ contentIsStructured NOTIFY contentIsStructuredChanged)

    Q_PROPERTY(QStringList words READ words NOTIFY wordsChanged)

    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)

    Q_PROPERTY(QString fileName READ fileName NOTIFY fileNameChanged)

    Q_PROPERTY(QString filePath READ filePath NOTIFY filePathChanged)

    Q_PROPERTY(QUrl fileUrl READ fileUrl NOTIFY filePathChanged)

    Q_PROPERTY(QUrl pdfPreviewUrl READ pdfPreviewUrl NOTIFY pdfPreviewUrlChanged)

    Q_PROPERTY(bool isPdf READ isPdf NOTIFY filePathChanged)

    Q_PROPERTY(bool hasTranslatedPdf READ hasTranslatedPdf NOTIFY translatedPdfPathChanged)

    Q_PROPERTY(QUrl translatedPdfUrl READ translatedPdfUrl NOTIFY translatedPdfPathChanged)

    Q_PROPERTY(int pageCount READ pageCount NOTIFY pageCountChanged)
    Q_PROPERTY(QString workflowTitle READ workflowTitle NOTIFY workflowChanged)
    Q_PROPERTY(QString workflowPipeline READ workflowPipeline NOTIFY workflowChanged)
    Q_PROPERTY(QString workflowExport READ workflowExport NOTIFY workflowChanged)
    Q_PROPERTY(bool workflowRoundTrip READ workflowRoundTrip NOTIFY workflowChanged)

    Q_PROPERTY(QString ramLabel READ ramLabel NOTIFY hardwareChanged)

    Q_PROPERTY(QString vramLabel READ vramLabel NOTIFY hardwareChanged)

    Q_PROPERTY(bool hwCompatible READ hwCompatible NOTIFY hardwareChanged)

    Q_PROPERTY(QString modelSpeed READ modelSpeed NOTIFY modelInfoChanged)

    Q_PROPERTY(QString modelQuality READ modelQuality NOTIFY modelInfoChanged)

    Q_PROPERTY(QString modelRamNeed READ modelRamNeed NOTIFY modelInfoChanged)

    Q_PROPERTY(QString modelVramNeed READ modelVramNeed NOTIFY modelInfoChanged)

    Q_PROPERTY(QStringList modelIds READ modelIds CONSTANT)




public:

    explicit TranslatorBackend(QObject* parent = nullptr);



    QString sourceText() const;

    QString translatedText() const;

    QString status() const;

    int progress() const;

    int estimatedRemainingSec() const;

    bool documentFormatted() const;

    bool contentIsStructured() const;

    QStringList words() const;

    bool busy() const;

    QString fileName() const;

    QString filePath() const;

    QUrl fileUrl() const;

    QUrl pdfPreviewUrl() const;

    bool isPdf() const;

    bool hasTranslatedPdf() const;

    QUrl translatedPdfUrl() const;

    int pageCount() const;
    QString workflowTitle() const;
    QString workflowPipeline() const;
    QString workflowExport() const;
    bool workflowRoundTrip() const;

    QString ramLabel() const;

    QString vramLabel() const;

    bool hwCompatible() const;

    QString modelSpeed() const;

    QString modelQuality() const;

    QString modelRamNeed() const;

    QString modelVramNeed() const;

    QStringList modelIds() const;

    Q_INVOKABLE void setSourceText(const QString& text);

    Q_INVOKABLE void loadFile(const QString& path);

    Q_INVOKABLE void loadFileFromUrl(const QUrl& url);

    Q_INVOKABLE void saveResult(const QString& path);

    Q_INVOKABLE void startTranslate(const QString& runtime,

                                    const QString& model,

                                    const QString& sourceLang,

                                    const QString& targetLang);

    void setAppSettings(AppSettings* settings);

    Q_INVOKABLE void cancelTranslate();

    Q_INVOKABLE void updateModelInfo(const QString& modelId);

    Q_INVOKABLE QString sourcePageText(int page) const;

    Q_INVOKABLE QString translatedPageText(int page) const;

    Q_INVOKABLE QString translatedPageHtml(int page) const;

    Q_INVOKABLE void ensurePageCount(int count);

    Q_INVOKABLE void retryTranslatedPdfBuild();

    Q_INVOKABLE QUrl suggestedSaveUrl() const;

    Q_INVOKABLE QString extractRuntimeStatus() const;

signals:

    void sourceTextChanged();

    void translatedTextChanged();

    void statusChanged();

    void progressChanged();

    void estimatedRemainingSecChanged();

    void documentFormattedChanged();

    void contentIsStructuredChanged();

    void wordsChanged();

    void busyChanged();

    void fileNameChanged();

    void filePathChanged();

    void pdfPreviewUrlChanged();

    void translatedPdfPathChanged();

    void pageCountChanged();

    void workflowChanged();

    void hardwareChanged();

    void modelInfoChanged();

private:

    void loadCatalog();

    void refreshHardware();

    void probeExtractRuntimeAsync();

    int estimatePageCount(const QString& text) const;

    QStringList chunkText(const QString& input, int maxChars = 2200) const;

    QString combinedSourceText() const;

    void buildTranslationUnits();

    void setStatus(const QString& text);

    void setBusy(bool value);

    void proceedWithTranslate();

    void runTranslateJob();

    void runExternalPdfEngine(const QString& engine);

    void processNextChunk();

    void dispatchTranslationChunks();

    void startChunkAt(int chunkIndex);

    void handleChunkFinished(int chunkIndex,
                             const QString& translated,
                             bool layoutChunk,
                             bool structuredChunk,
                             bool segmentChunk);

    void finishTranslation();

    void animateWords(const QString& text);

    void stopWordAnimation();

    void updateTranslationEta();

    int parseTokensPerSec() const;

    bool parseTranslatedBlocks(const QString& raw, QJsonArray* blocksOut) const;

    void applyTranslatedBlocks(int pageIndex, const QJsonArray& blocks);

    void applyLoadedDocument(const DocumentLoadResult& doc, const QFileInfo& fileInfo);

    void applyLayoutPageTranslations(int pageIndex, const QString& response);

    void applySegmentTranslations(const QString& response);

    void syncTranslatedPagesFromLayout();

    void syncTranslatedTextFromSegments();

    void updateWorkflowInfo(const QString& suffix);

    QString glossaryPromptSuffix() const;

    QString buildPromptForChunk(const QString& chunk,
                                bool layoutChunk,
                                bool structuredChunk,
                                bool segmentChunk) const;

    void buildTranslatedPdfAsync();

    bool ensurePdfWorkCopy();

    void clearPdfWorkCopy();

    static QString normalizeLocalFilePath(const QString& path);

    QString ollamaApiUrl(const QString& path) const;

    AppSettings* m_appSettings = nullptr;

    QNetworkAccessManager m_net;

    QList<QNetworkReply*> m_activeReplies;

    QTimer* m_wordsTimer = nullptr;

    QTimer* m_etaTimer = nullptr;

    QElapsedTimer m_translateTimer;

    int m_loadGeneration = 0;

    QJsonArray m_models;

    QStringList m_modelIds;

    QString m_sourceText;

    QString m_translatedText;

    QString m_status = QStringLiteral("Готово");

    QString m_fileName;

    QString m_filePath;

    QString m_pdfWorkCopyPath;

    QString m_translatedPdfPath;

    bool m_isPdf = false;

    QString m_runtime = QStringLiteral("local");

    QString m_model = QStringLiteral("translategemma:4b");

    QString m_sourceLang = QStringLiteral("en");

    QString m_targetLang = QStringLiteral("ru");

    QString m_baseUrl = QStringLiteral("https://api.deepseek.com");

    QString m_apiKey;

    QString m_pendingRuntime;

    QString m_pendingModel;

    QString m_pendingSourceLang;

    QString m_pendingTargetLang;

    QString m_pendingBaseUrl;

    QString m_pendingApiKey;

    QStringList m_chunks;

    QStringList m_translatedChunks;

    QList<int> m_chunkPageIndex;

    int m_currentChunk = 0;

    bool m_busy = false;

    bool m_cancelled = false;

    int m_progress = 0;

    int m_estimatedRemainingSec = 0;

    bool m_documentFormatted = false;

    bool m_structuredDocument = false;

    bool m_pdfLayoutMode = false;

    bool m_segmentMode = false;

    int m_pageCount = 0;

    int m_inFlight = 0;

    int m_completedChunks = 0;

    int m_nextDispatchIndex = 0;

    QString m_workflowId;

    QString m_workflowTitle;

    QString m_workflowPipeline;

    QString m_workflowExport;

    bool m_workflowRoundTrip = false;

    QJsonObject m_workflowMeta;

    QStringList m_sourcePages;

    QList<QJsonArray> m_sourcePageBlocks;

    QJsonObject m_pdfLayout;

    QList<QJsonArray> m_translatedPageBlocks;

    QStringList m_translatedPageHtml;

    QStringList m_translatedPages;

    QStringList m_words;

    QString m_ramLabel;

    QString m_vramLabel;

    bool m_hwCompatible = true;

    double m_ramTotalGb = 0;

    double m_vramTotalGb = 0;

    QString m_modelSpeed;

    QString m_modelQuality;

    QString m_modelRamNeed;

    QString m_modelVramNeed;

};



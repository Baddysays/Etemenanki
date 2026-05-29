#pragma once

#include <QString>
#include <QStringList>

struct WorkflowInfo
{
    QString id;
    QString titleRu;
    QString titleEn;
    QString pipelineRu;
    QString pipelineEn;
    QString exportRu;
    QString exportEn;
    bool layoutPreserving = false;
    bool roundTripExport = false;
};

class TranslationWorkflow
{
public:
    static WorkflowInfo forSuffix(const QString& suffix);
    static bool isHubExtractSuffix(const QString& suffix);
    static QString openFileFilter(bool englishUi);
    static QString saveFileFilter(bool englishUi, bool pdfWithTranslation, bool roundTrip);
    static QStringList supportedSuffixes();
};

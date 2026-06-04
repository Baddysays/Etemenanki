#pragma once

#include "document_loader.h"

#include <QAtomicInt>
#include <QObject>
#include <QProcess>

class ExternalPdfTranslateWorker : public QObject
{
    Q_OBJECT

public:
    explicit ExternalPdfTranslateWorker(ExternalPdfTranslateRequest request,
                                        QObject* parent = nullptr);

public slots:
    void start();
    void cancel();

signals:
    void finished(ExternalPdfTranslateResult result);
    void progressUpdated(QString message);

private slots:
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onReadyReadStandardError();

private:
    ExternalPdfTranslateRequest m_request;
    QProcess* m_proc = nullptr;
    QByteArray m_stdoutBuf;
    QByteArray m_stderrBuf;
    QAtomicInt m_cancelled{0};
};

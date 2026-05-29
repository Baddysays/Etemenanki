#pragma once

#include <QJsonArray>
#include <QString>

namespace DocumentFormat {

QString escapeHtml(const QString& text);
QString blocksToPlainText(const QJsonArray& blocks);
QString blocksToHtml(const QJsonArray& blocks);
QString wrapHtmlPage(const QString& bodyHtml);

} // namespace DocumentFormat

#pragma once

#include <QHash>
#include <QString>
#include <QVariantList>

namespace AppUiStrings {

using LangTable = QHash<QString, QString>;
using KeyTable = QHash<QString, LangTable>;

const KeyTable& table();
QString text(const QString& key, const QString& lang);
QString textArgs(const QString& key, const QString& lang, const QStringList& args);
QString translateMessage(const QString& message, const QString& lang);
QVariantList uiLanguageOptions();

} // namespace AppUiStrings

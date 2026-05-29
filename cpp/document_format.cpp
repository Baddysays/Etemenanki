#include "document_format.h"

#include <QJsonObject>

namespace DocumentFormat {

QString escapeHtml(const QString& text)
{
    QString out;
    out.reserve(text.size() + 16);
    for (const QChar ch : text) {
        switch (ch.unicode()) {
        case '&': out += QStringLiteral("&amp;"); break;
        case '<': out += QStringLiteral("&lt;"); break;
        case '>': out += QStringLiteral("&gt;"); break;
        case '"': out += QStringLiteral("&quot;"); break;
        default: out += ch; break;
        }
    }
    return out;
}

QString blocksToPlainText(const QJsonArray& blocks)
{
    QStringList parts;
    for (const QJsonValue& value : blocks) {
        const QJsonObject block = value.toObject();
        const QString kind = block.value(QStringLiteral("kind")).toString();
        if (kind == QStringLiteral("table")) {
            const QJsonArray rows = block.value(QStringLiteral("rows")).toArray();
            for (const QJsonValue& rowValue : rows) {
                QStringList cells;
                for (const QJsonValue& cell : rowValue.toArray())
                    cells << cell.toString().trimmed();
                parts << cells.join(QStringLiteral(" | "));
            }
        } else if (kind == QStringLiteral("list")) {
            for (const QJsonValue& item : block.value(QStringLiteral("items")).toArray())
                parts << item.toString();
        } else {
            parts << block.value(QStringLiteral("text")).toString();
        }
    }
    return parts.join(QStringLiteral("\n\n"));
}

QString wrapHtmlPage(const QString& bodyHtml)
{
    return QStringLiteral(
               "<html><head><meta charset='utf-8'><style>"
               "body{font-family:'Segoe UI',Arial,sans-serif;font-size:11pt;color:#1f2b3f;"
               "line-height:1.4;margin:12px 16px;}"
               "h1{font-size:20pt;font-weight:700;margin:14px 0 8px;color:#1f2b3f;}"
               "h2{font-size:16pt;font-weight:700;margin:12px 0 6px;color:#1f2b3f;}"
               "h3{font-size:13pt;font-weight:600;margin:10px 0 4px;color:#1f2b3f;}"
               "p{margin:5px 0;}"
               "table{border-collapse:collapse;margin:12px 0;width:100%;}"
               "td,th{border:1px solid #9db3d9;padding:6px 8px;vertical-align:top;font-size:10pt;}"
               "th{background:#eef3fb;font-weight:600;}"
               "ul{margin:6px 0 8px 22px;padding:0;}"
               "li{margin:2px 0;}"
               "</style></head><body>")
           + bodyHtml + QStringLiteral("</body></html>");
}

QString blocksToHtml(const QJsonArray& blocks)
{
    QString body;
    for (const QJsonValue& value : blocks) {
        const QJsonObject block = value.toObject();
        const QString kind = block.value(QStringLiteral("kind")).toString();
        if (kind == QStringLiteral("heading")) {
            const int level = qBound(1, block.value(QStringLiteral("level")).toInt(2), 3);
            body += QStringLiteral("<h%1>%2</h%1>")
                         .arg(level)
                         .arg(escapeHtml(block.value(QStringLiteral("text")).toString()));
        } else if (kind == QStringLiteral("table")) {
            const QJsonArray rows = block.value(QStringLiteral("rows")).toArray();
            if (rows.isEmpty())
                continue;
            body += QStringLiteral("<table>");
            bool header = true;
            for (const QJsonValue& rowValue : rows) {
                const QJsonArray row = rowValue.toArray();
                body += QStringLiteral("<tr>");
                for (const QJsonValue& cell : row) {
                    const QString tag = header ? QStringLiteral("th") : QStringLiteral("td");
                    body += QStringLiteral("<%1>%2</%1>")
                                 .arg(tag, escapeHtml(cell.toString()));
                }
                body += QStringLiteral("</tr>");
                header = false;
            }
            body += QStringLiteral("</table>");
        } else if (kind == QStringLiteral("list")) {
            body += QStringLiteral("<ul>");
            for (const QJsonValue& item : block.value(QStringLiteral("items")).toArray()) {
                body += QStringLiteral("<li>%1</li>").arg(escapeHtml(item.toString()));
            }
            body += QStringLiteral("</ul>");
        } else {
            const int indent = qMax(0, block.value(QStringLiteral("indent")).toInt());
            body += QStringLiteral("<p style='margin-left:%1px'>%2</p>")
                         .arg(indent * 14)
                         .arg(escapeHtml(block.value(QStringLiteral("text")).toString()));
        }
    }
    return wrapHtmlPage(body);
}

} // namespace DocumentFormat

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Window {
    id: helpWin

    EtePalette { id: pal }
    title: settings.uiText("help", settings.appUiLanguage)
    width: 600
    height: 640
    minimumWidth: 480
    minimumHeight: 400
    color: pal.bg
    flags: Qt.Window | Qt.WindowTitleHint | Qt.WindowSystemMenuHint
           | Qt.WindowMinMaxButtonsHint | Qt.WindowCloseButtonHint

    property bool helpEn: settings.appUiLanguage !== "ru"

    readonly property string contactEmail: "hello@baddysays.ru"
    readonly property string contactTelegramUrl: "https://t.me/baddysays"
    readonly property string contactTelegramLabel: "@baddysays"

    function open() {
        show()
        raise()
        requestActivate()
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 0

        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            ScrollBar.vertical.policy: ScrollBar.AsNeeded

            ColumnLayout {
                width: helpWin.width - 56
                spacing: 16

                Label {
                    Layout.fillWidth: true
                    wrapMode: Text.WordWrap
                    text: helpEn
                          ? "Etemenanki translates documents while preserving structure where possible."
                          : "Etemenanki переводит документы с сохранением структуры там, где это возможно."
                    color: pal.muted
                    font.pixelSize: pal.fontCaption
                }

                Rectangle { Layout.fillWidth: true; height: 1; color: pal.border }

                Label {
                    text: helpEn ? "Quick start" : "Быстрый старт"
                    font.pixelSize: 16
                    font.weight: Font.Bold
                    color: pal.text
                }
                Label {
                    Layout.fillWidth: true
                    wrapMode: Text.WordWrap
                    lineHeight: 1.4
                    text: helpEn
                          ? "1. Upload a document.\n2. Choose languages and model.\n3. Translate.\n4. Save the result.\n5. For PDF, open «Original format»."
                          : "1. Загрузите документ.\n2. Выберите языки и модель.\n3. Переведите.\n4. Сохраните.\n5. Для PDF откройте «Исходный формат»."
                    color: pal.text
                }

                Rectangle { Layout.fillWidth: true; height: 1; color: pal.border }

                Label {
                    text: helpEn ? "Supported formats" : "Форматы"
                    font.pixelSize: 16
                    font.weight: Font.Bold
                    color: pal.text
                }

                GridLayout {
                    Layout.fillWidth: true
                    columns: 2
                    columnSpacing: 16
                    rowSpacing: 8

                    Repeater {
                        model: helpEn ? [
                            ["PDF", "Layout-preserving translation"],
                            ["DOCX", "Word → DOCX or TXT"],
                            ["XLSX / CSV", "Cells → same format"],
                            ["SRT / VTT", "Subtitles"],
                            ["JSON", "String fields"],
                            ["HTML / EPUB", "Structured text"],
                            ["TXT / MD", "Plain text"]
                        ] : [
                            ["PDF", "Перевод с вёрсткой"],
                            ["DOCX", "Word → DOCX или TXT"],
                            ["XLSX / CSV", "Ячейки → тот же формат"],
                            ["SRT / VTT", "Субтитры"],
                            ["JSON", "Строковые поля"],
                            ["HTML / EPUB", "Структурированный текст"],
                            ["TXT / MD", "Обычный текст"]
                        ]

                        RowLayout {
                            required property var modelData
                            Layout.columnSpan: 2
                            Layout.fillWidth: true
                            spacing: 16
                            Label {
                                Layout.preferredWidth: 100
                                text: modelData[0]
                                font.weight: Font.DemiBold
                                color: pal.accent
                                font.pixelSize: pal.fontCaption
                            }
                            Label {
                                Layout.fillWidth: true
                                wrapMode: Text.WordWrap
                                text: modelData[1]
                                color: pal.text
                                font.pixelSize: pal.fontCaption
                            }
                        }
                    }
                }

                Rectangle { Layout.fillWidth: true; height: 1; color: pal.border }

                Label {
                    text: helpEn ? "About the product" : "О продукте"
                    font.pixelSize: 16
                    font.weight: Font.Bold
                    color: pal.text
                }

                Label {
                    Layout.fillWidth: true
                    wrapMode: Text.WordWrap
                    lineHeight: 1.4
                    text: helpEn
                          ? "Etemenanki is developed by baddysays.\nQuestions, feedback and support:"
                          : "Etemenanki разработано baddysays.\nВопросы, отзывы и поддержка:"
                    color: pal.text
                }

                GridLayout {
                    Layout.fillWidth: true
                    columns: 2
                    columnSpacing: 16
                    rowSpacing: 10

                    Label {
                        text: helpEn ? "Email" : "Почта"
                        font.weight: Font.DemiBold
                        color: pal.muted
                        font.pixelSize: pal.fontCaption
                    }
                    Label {
                        Layout.fillWidth: true
                        text: helpWin.contactEmail
                        color: pal.accentText
                        font.pixelSize: pal.fontCaption
                        font.underline: true
                        TapHandler {
                            cursorShape: Qt.PointingHandCursor
                            onTapped: Qt.openUrlExternally("mailto:" + helpWin.contactEmail)
                        }
                    }

                    Label {
                        text: "Telegram"
                        font.weight: Font.DemiBold
                        color: pal.muted
                        font.pixelSize: pal.fontCaption
                    }
                    Label {
                        Layout.fillWidth: true
                        text: helpWin.contactTelegramLabel
                        color: pal.accentText
                        font.pixelSize: pal.fontCaption
                        font.underline: true
                        TapHandler {
                            cursorShape: Qt.PointingHandCursor
                            onTapped: Qt.openUrlExternally(helpWin.contactTelegramUrl)
                        }
                    }
                }
            }
        }

        Button {
            Layout.alignment: Qt.AlignRight
            implicitHeight: 40
            text: helpEn ? "Close" : "Закрыть"
            onClicked: helpWin.close()
            background: Rectangle {
                radius: pal.radiusSm
                color: parent.hovered ? pal.surface : pal.card
                border.color: pal.border
            }
            contentItem: Label {
                text: parent.text
                color: pal.text
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }
        }
    }
}

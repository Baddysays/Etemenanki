import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

ApplicationWindow {
    width: 1460
    height: 900
    visible: true
    title: "Etemenanki - Переводчик документов"
    color: "#f3f5f9"

    property string runtime: "local"

    FileDialog {
        id: openDialog
        title: "Выбрать файл"
        nameFilters: ["Documents (*.pdf *.docx *.txt *.md)", "All files (*)"]
        onAccepted: backend.chooseFile(selectedFile.toString())
    }

    FileDialog {
        id: saveDialog
        title: "Сохранить перевод"
        nameFilters: ["Word (*.docx)", "Text (*.txt)"]
        onAccepted: backend.saveResult(selectedFile.toString())
    }

    Rectangle {
        anchors.fill: parent
        anchors.margins: 14
        color: "transparent"

        ColumnLayout {
            anchors.fill: parent
            spacing: 8

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 84
                radius: 14
                color: "white"
                border.color: "#d9e1ef"

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 12

                    ColumnLayout {
                        spacing: 0
                        Label { text: "Etemenanki"; font.pixelSize: 34; font.weight: Font.DemiBold; color: "#1f2b3f" }
                        Label { text: "AI-переводчик документов"; color: "#6a7892"; font.pixelSize: 14 }
                    }
                    Item { Layout.fillWidth: true }
                    Rectangle {
                        radius: 10
                        color: "#eef8ef"
                        border.color: "#bfe0c4"
                        Layout.preferredWidth: 180
                        Layout.preferredHeight: 52
                        Label {
                            anchors.centerIn: parent
                            text: backend.specsBadge
                            color: "#295f31"
                            font.pixelSize: 12
                        }
                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 112
                radius: 14
                color: "white"
                border.color: "#d9e1ef"

                GridLayout {
                    anchors.fill: parent
                    anchors.margins: 10
                    rowSpacing: 8
                    columnSpacing: 8
                    columns: 8

                    Label { text: "UI" }
                    ComboBox { id: uiLang; model: ["ru", "en"]; Layout.fillWidth: true }
                    Label { text: "Режим ИИ" }
                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 36
                        radius: 10
                        color: "#f0f4fb"
                        border.color: "#cad7ec"
                        RowLayout {
                            anchors.fill: parent
                            anchors.margins: 3
                            spacing: 4
                            Button {
                                text: "Local"
                                checkable: true
                                checked: runtime === "local"
                                onClicked: runtime = "local"
                                background: Rectangle { radius: 8; color: parent.checked ? "#e6efff" : "transparent"; border.color: parent.checked ? "#8fb0e5" : "transparent" }
                            }
                            Button {
                                text: "Cloud"
                                checkable: true
                                checked: runtime === "cloud"
                                onClicked: runtime = "cloud"
                                background: Rectangle { radius: 8; color: parent.checked ? "#e6efff" : "transparent"; border.color: parent.checked ? "#8fb0e5" : "transparent" }
                            }
                        }
                    }
                    Label { text: "Модель" }
                    ComboBox {
                        id: modelBox
                        model: backend.models
                        Layout.columnSpan: 3
                        Layout.fillWidth: true
                        onCurrentTextChanged: backend.refresh_compat(currentText)
                    }

                    Label { text: "Исходный язык" }
                    ComboBox { id: srcLang; model: ["auto", "en", "ru", "de", "fr", "es", "uk", "zh"]; currentIndex: 1; Layout.fillWidth: true }
                    Button {
                        text: "⇄"
                        onClicked: {
                            if (srcLang.currentText !== "auto") {
                                const oldSrc = srcLang.currentIndex
                                srcLang.currentIndex = dstLang.currentIndex + 1
                                dstLang.currentIndex = oldSrc - 1
                            }
                        }
                        background: Rectangle { radius: 16; color: "white"; border.color: "#cfd9eb" }
                    }
                    Label { text: "Язык перевода" }
                    ComboBox { id: dstLang; model: ["ru", "en", "de", "fr", "es", "uk", "zh"]; Layout.fillWidth: true }
                    Button { text: "Выбрать файл"; onClicked: openDialog.open() }
                    Button {
                        text: "Перевести"
                        onClicked: backend.startTranslate(runtime, modelBox.currentText, srcLang.currentText, dstLang.currentText, baseUrl.text, apiKey.text)
                        background: Rectangle { radius: 10; color: "#2f78ee"; border.color: "#2f78ee" }
                        contentItem: Text { text: parent.text; color: "white"; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                    }
                    Button { text: "Сохранить как"; onClicked: saveDialog.open() }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: runtime === "cloud" ? 70 : 0
                visible: runtime === "cloud"
                radius: 14
                color: "white"
                border.color: "#d9e1ef"
                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 12
                    spacing: 10
                    Label { text: "Base URL" }
                    TextField { id: baseUrl; text: "https://api.deepseek.com"; Layout.fillWidth: true }
                    Label { text: "API Key" }
                    TextField { id: apiKey; echoMode: TextInput.Password; Layout.preferredWidth: 260 }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 82
                radius: 14
                color: "white"
                border.color: "#d9e1ef"
                Label {
                    anchors.fill: parent
                    anchors.margins: 12
                    text: backend.compatText
                    wrapMode: Text.WordWrap
                    color: "#2a3a53"
                }
            }

            RowLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: 8

                Rectangle {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    radius: 14
                    color: "white"
                    border.color: "#d9e1ef"
                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 8
                        Label { text: "Просмотр исходного файла" }
                        RowLayout {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            ListView {
                                Layout.preferredWidth: 92
                                Layout.fillHeight: true
                                model: backend.pageCount
                                delegate: Rectangle {
                                    width: 72
                                    height: 34
                                    radius: 8
                                    border.color: "#d5e0f2"
                                    color: "#f7f9fd"
                                    Text { anchors.centerIn: parent; text: index + 1; color: "#4b5d7a" }
                                }
                            }
                            TextArea {
                                id: sourceText
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                text: backend.sourceText
                                wrapMode: TextArea.Wrap
                                onTextChanged: backend.setSourceText(text)
                                background: Rectangle { radius: 10; color: "#fbfcfe"; border.color: "#cfd9eb" }
                            }
                        }
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    radius: 14
                    color: "white"
                    border.color: "#d9e1ef"
                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 8
                        Label { text: "Просмотр переведенного файла" }
                        RowLayout {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            ListView {
                                Layout.preferredWidth: 92
                                Layout.fillHeight: true
                                model: backend.pageCount
                                delegate: Rectangle {
                                    width: 72
                                    height: 34
                                    radius: 8
                                    border.color: "#d5e0f2"
                                    color: "#f7f9fd"
                                    Text { anchors.centerIn: parent; text: index + 1; color: "#4b5d7a" }
                                }
                            }
                            TextArea {
                                id: resultText
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                text: backend.resultText
                                wrapMode: TextArea.Wrap
                                onTextChanged: backend.setResultText(text)
                                background: Rectangle { radius: 10; color: "#fbfcfe"; border.color: "#cfd9eb" }
                            }
                        }
                    }
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 8
                CheckBox { text: "PDF -> DOCX output"; checked: true }
                ProgressBar { Layout.fillWidth: true; value: backend.progress / 100.0 }
            }

            Label { text: backend.status; color: "#4a5f80" }
        }
    }
}

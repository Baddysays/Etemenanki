import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Dialog {
    id: dlg
    property color cText: "#1f2b3f"
    property color cMuted: "#6a7892"
    property color cBorder: "#d9e1ef"
    property color cCard: "#ffffff"
    property color cAccent: "#2f78ee"

    modal: true
    anchors.centerIn: parent
    width: 720
    height: 620
    padding: 0
    standardButtons: Dialog.NoButton

    background: Rectangle {
        radius: 14
        color: cCard
        border.color: cBorder
    }

    property int section: 0

    contentItem: ColumnLayout {
        spacing: 0

        RowLayout {
            Layout.fillWidth: true
            Layout.margins: 16
            spacing: 12
            Label {
                text: settings.uiText("settings")
                font.pixelSize: 20
                font.weight: Font.Bold
                color: cText
            }
            Item { Layout.fillWidth: true }
            Button {
                text: "✕"
                implicitWidth: 36
                implicitHeight: 36
                onClicked: dlg.close()
                background: Rectangle {
                    radius: 8
                    color: parent.hovered ? "#eef2f8" : "transparent"
                    border.color: "#e3eaf5"
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            height: 1
            color: cBorder
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

            ListView {
                id: nav
                Layout.preferredWidth: 200
                Layout.fillHeight: true
                clip: true
                model: [
                    { id: 0, title: settings.appUiLanguage === "en" ? "General" : "Общие" },
                    { id: 1, title: settings.appUiLanguage === "en" ? "Languages" : "Языки" },
                    { id: 2, title: settings.appUiLanguage === "en" ? "Models" : "Модели" },
                    { id: 3, title: settings.appUiLanguage === "en" ? "Cloud API" : "Облако API" },
                    { id: 4, title: settings.appUiLanguage === "en" ? "PDF layout" : "Вёрстка PDF" },
                    { id: 5, title: settings.appUiLanguage === "en" ? "Translation" : "Перевод" }
                ]
                currentIndex: dlg.section
                onCurrentIndexChanged: dlg.section = currentIndex

                delegate: ItemDelegate {
                    width: nav.width
                    height: 40
                    required property int index
                    required property var modelData
                    highlighted: dlg.section === modelData.id
                    contentItem: Label {
                        text: modelData.title
                        color: highlighted ? "#224d93" : cText
                        font.weight: highlighted ? Font.DemiBold : Font.Normal
                    }
                    background: Rectangle {
                        color: highlighted ? "#e6efff" : (parent.hovered ? "#f7f9fd" : "transparent")
                    }
                    onClicked: dlg.section = modelData.id
                }
            }

            Rectangle {
                Layout.preferredWidth: 1
                Layout.fillHeight: true
                color: cBorder
            }

            StackLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.margins: 16
                currentIndex: dlg.section

                // General
                ScrollView {
                    clip: true
                    ColumnLayout {
                        width: parent.width
                        spacing: 12
                        Label {
                            text: settings.appUiLanguage === "en" ? "Application language" : "Язык интерфейса"
                            color: cMuted
                            font.pixelSize: 12
                        }
                        ComboBox {
                            Layout.fillWidth: true
                            model: [
                                { code: "ru", label: "Русский" },
                                { code: "en", label: "English" }
                            ]
                            textRole: "label"
                            valueRole: "code"
                            currentIndex: settings.appUiLanguage === "en" ? 1 : 0
                            onActivated: {
                                const code = model[currentIndex].code
                                settings.setAppUiLanguage(code)
                            }
                        }
                        Label {
                            text: "Ollama"
                            color: cMuted
                            font.pixelSize: 12
                        }
                        TextField {
                            id: ollamaField
                            Layout.fillWidth: true
                            text: settings.ollamaBaseUrl
                            placeholderText: "http://127.0.0.1:11434"
                            onEditingFinished: settings.setOllamaBaseUrl(text)
                            background: Rectangle {
                                radius: 10
                                color: "#fbfcfe"
                                border.color: "#cfd9eb"
                            }
                        }
                        Label {
                            Layout.fillWidth: true
                            wrapMode: Text.WordWrap
                            text: backend.extractRuntimeStatus()
                            color: cText
                            font.pixelSize: 12
                        }
                    }
                }

                // Languages
                ScrollView {
                    clip: true
                    ColumnLayout {
                        width: parent.width
                        spacing: 8
                        Label {
                            text: settings.appUiLanguage === "en"
                                  ? "Languages available in translation pickers"
                                  : "Языки в списках перевода"
                            color: cMuted
                            font.pixelSize: 12
                        }
                        Repeater {
                            model: {
                                const all = settings.allLanguages()
                                const out = []
                                for (let i = 0; i < all.length; ++i) {
                                    if (all[i].code !== "auto")
                                        out.push(all[i])
                                }
                                return out
                            }
                            delegate: CheckBox {
                                required property var modelData
                                text: modelData.flag + "  " + modelData.label
                                checked: settings.isLanguageEnabled(modelData.code)
                                onToggled: settings.setLanguageEnabled(modelData.code, checked)
                            }
                        }
                    }
                }

                // Models
                ScrollView {
                    clip: true
                    ColumnLayout {
                        width: parent.width
                        spacing: 14
                        GroupBox {
                            Layout.fillWidth: true
                            title: settings.uiText("local")
                            label: Label {
                                color: cMuted
                                font.pixelSize: 12
                            }
                            ColumnLayout {
                                anchors.fill: parent
                                spacing: 8
                                Label {
                                    wrapMode: Text.WordWrap
                                    Layout.fillWidth: true
                                    text: settings.localRuntimeAvailable
                                          ? (settings.appUiLanguage === "en"
                                             ? "Installed in Ollama:"
                                             : "Установлены в Ollama:")
                                          : settings.uiText("no_local_models")
                                    color: settings.localRuntimeAvailable ? cText : "#c77b1d"
                                    font.pixelSize: 12
                                }
                                ComboBox {
                                    Layout.fillWidth: true
                                    enabled: settings.availableLocalModels.length > 0
                                    model: settings.availableLocalModels
                                    currentIndex: Math.max(0, settings.availableLocalModels.indexOf(settings.selectedLocalModel))
                                    onActivated: settings.setSelectedLocalModel(currentText)
                                }
                                Button {
                                    text: settings.appUiLanguage === "en" ? "Refresh list" : "Обновить список"
                                    onClicked: settings.refreshAvailableModels()
                                }
                            }
                        }
                        GroupBox {
                            Layout.fillWidth: true
                            title: settings.uiText("cloud")
                            label: Label { color: cMuted; font.pixelSize: 12 }
                            ColumnLayout {
                                anchors.fill: parent
                                spacing: 8
                                Label {
                                    wrapMode: Text.WordWrap
                                    Layout.fillWidth: true
                                    text: settings.cloudRuntimeAvailable
                                          ? (settings.appUiLanguage === "en"
                                             ? "Configured cloud models:"
                                             : "Настроенные облачные модели:")
                                          : settings.uiText("no_cloud_models")
                                    color: settings.cloudRuntimeAvailable ? cText : "#c77b1d"
                                    font.pixelSize: 12
                                }
                                ComboBox {
                                    Layout.fillWidth: true
                                    enabled: settings.availableCloudModels.length > 0
                                    model: settings.availableCloudModels
                                    currentIndex: Math.max(0, settings.availableCloudModels.indexOf(settings.selectedCloudModel))
                                    onActivated: settings.setSelectedCloudModel(currentText)
                                }
                            }
                        }
                    }
                }

                // Cloud API
                ScrollView {
                    clip: true
                    ColumnLayout {
                        width: parent.width
                        spacing: 16
                        Repeater {
                            model: settings.cloudProviders()
                            delegate: Rectangle {
                                required property var modelData
                                Layout.fillWidth: true
                                implicitHeight: cloudCol.implicitHeight + 24
                                radius: 12
                                color: "#fbfcfe"
                                border.color: cBorder

                                property var cfg: settings.cloudProvider(modelData.id)

                                ColumnLayout {
                                    id: cloudCol
                                    anchors.fill: parent
                                    anchors.margins: 12
                                    spacing: 8
                                    CheckBox {
                                        text: modelData.title
                                        checked: cfg.enabled
                                        onToggled: settings.setCloudProvider(
                                            modelData.id,
                                            urlField.text,
                                            keyField.text,
                                            modelField.text,
                                            checked)
                                    }
                                    Label { text: "Base URL"; color: cMuted; font.pixelSize: 11 }
                                    TextField {
                                        id: urlField
                                        Layout.fillWidth: true
                                        text: cfg.baseUrl || modelData.defaultUrl
                                        background: Rectangle {
                                            radius: 8
                                            color: "#ffffff"
                                            border.color: "#cfd9eb"
                                        }
                                    }
                                    Label { text: "API Key"; color: cMuted; font.pixelSize: 11 }
                                    TextField {
                                        id: keyField
                                        Layout.fillWidth: true
                                        text: cfg.apiKey
                                        echoMode: TextInput.Password
                                        placeholderText: "sk-..."
                                        background: Rectangle {
                                            radius: 8
                                            color: "#ffffff"
                                            border.color: "#cfd9eb"
                                        }
                                    }
                                    Label { text: "Model ID"; color: cMuted; font.pixelSize: 11 }
                                    TextField {
                                        id: modelField
                                        Layout.fillWidth: true
                                        text: cfg.modelId || modelData.defaultModel
                                        background: Rectangle {
                                            radius: 8
                                            color: "#ffffff"
                                            border.color: "#cfd9eb"
                                        }
                                    }
                                    Button {
                                        text: settings.appUiLanguage === "en" ? "Save" : "Сохранить"
                                        onClicked: settings.setCloudProvider(
                                            modelData.id,
                                            urlField.text,
                                            keyField.text,
                                            modelField.text,
                                            true)
                                    }
                                }
                            }
                        }
                    }
                }

                // PDF layout & engines
                ScrollView {
                    clip: true
                    ColumnLayout {
                        id: pdfSection
                        width: parent.width
                        spacing: 10

                        property var engineCatalog: settings.pdfEngineCatalog()
                        property var selectedEngine: {
                            for (let i = 0; i < engineCatalog.length; ++i) {
                                if (engineCatalog[i].id === settings.pdfEngine)
                                    return engineCatalog[i]
                            }
                            return engineCatalog.length > 0 ? engineCatalog[0] : ({})
                        }
                        property var engineProbe: ({})

                        function refreshProbe() {
                            engineProbe = settings.probePdfEnginesStatus()
                        }

                        Component.onCompleted: refreshProbe()

                        Label {
                            wrapMode: Text.WordWrap
                            Layout.fillWidth: true
                            text: settings.appUiLanguage === "en"
                                  ? "Choose a PDF layout engine. External engines run as separate open-source tools."
                                  : "Выберите движок PDF. Внешние движки — отдельные open-source инструменты."
                            color: cMuted
                            font.pixelSize: 12
                        }

                        Label {
                            text: settings.appUiLanguage === "en" ? "PDF engine" : "Движок PDF"
                            font.weight: Font.DemiBold
                            color: cText
                        }

                        ButtonGroup { id: pdfEngineGroup }

                        Repeater {
                            model: pdfSection.engineCatalog
                            delegate: RadioButton {
                                required property var modelData
                                ButtonGroup.group: pdfEngineGroup
                                Layout.fillWidth: true
                                text: modelData.name
                                checked: settings.pdfEngine === modelData.id
                                onClicked: {
                                    settings.setPdfEngine(modelData.id)
                                    pdfSection.refreshProbe()
                                }
                            }
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            Layout.preferredHeight: engineInfo.implicitHeight + 16
                            radius: 10
                            color: "#f7f9fd"
                            border.color: cBorder
                            ColumnLayout {
                                id: engineInfo
                                anchors.fill: parent
                                anchors.margins: 10
                                spacing: 4
                                Label {
                                    Layout.fillWidth: true
                                    wrapMode: Text.WordWrap
                                    text: pdfSection.selectedEngine.desc || ""
                                    color: cText
                                    font.pixelSize: 12
                                }
                                Label {
                                    Layout.fillWidth: true
                                    wrapMode: Text.WordWrap
                                    visible: (pdfSection.selectedEngine.requires || "").length > 0
                                    text: pdfSection.selectedEngine.requires || ""
                                    color: cMuted
                                    font.pixelSize: 11
                                }
                                RowLayout {
                                    Layout.fillWidth: true
                                    spacing: 8
                                    Label {
                                        Layout.fillWidth: true
                                        wrapMode: Text.WordWrap
                                        visible: (pdfSection.selectedEngine.authors || "").length > 0
                                        text: (settings.appUiLanguage === "en" ? "Authors: " : "Авторы: ")
                                              + (pdfSection.selectedEngine.authors || "")
                                        color: cMuted
                                        font.pixelSize: 11
                                    }
                                    Button {
                                        visible: (pdfSection.selectedEngine.url || "").length > 0
                                        text: "GitHub ↗"
                                        flat: true
                                        onClicked: settings.openExternalUrl(pdfSection.selectedEngine.url)
                                    }
                                }
                            }
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 8
                            Button {
                                text: settings.appUiLanguage === "en" ? "Check availability" : "Проверить доступность"
                                onClicked: pdfSection.refreshProbe()
                            }
                            Label {
                                Layout.fillWidth: true
                                wrapMode: Text.WordWrap
                                text: {
                                    const engines = pdfSection.engineProbe.engines
                                    if (!engines)
                                        return settings.appUiLanguage === "en" ? "Not checked" : "Не проверено"
                                    const info = engines[settings.pdfEngine]
                                    if (!info)
                                        return ""
                                    const mark = info.available ? "✓" : "✗"
                                    return mark + " " + (info.message || "")
                                }
                                color: {
                                    const engines = pdfSection.engineProbe.engines
                                    if (!engines || !engines[settings.pdfEngine])
                                        return cMuted
                                    return engines[settings.pdfEngine].available ? "#27a85a" : "#c77b1d"
                                }
                                font.pixelSize: 11
                            }
                        }

                        RowLayout {
                            visible: settings.pdfEngine === "pdfmathtranslate"
                            Layout.fillWidth: true
                            spacing: 8
                            Button {
                                text: settings.appUiLanguage === "en"
                                      ? "Download pdf2zh (releases)"
                                      : "Скачать pdf2zh (релизы)"
                                onClicked: settings.openExternalUrl(
                                    "https://github.com/Byaidu/PDFMathTranslate/releases")
                            }
                            Label {
                                Layout.fillWidth: true
                                wrapMode: Text.WordWrap
                                text: settings.appUiLanguage === "en"
                                      ? "Or run: powershell -File tools/setup_pdf2zh.ps1"
                                      : "Или: powershell -File tools/setup_pdf2zh.ps1"
                                color: cMuted
                                font.pixelSize: 11
                            }
                        }

                        Rectangle { Layout.fillWidth: true; height: 1; color: cBorder }

                        Label {
                            visible: settings.pdfEngine === "etemenanki"
                            wrapMode: Text.WordWrap
                            Layout.fillWidth: true
                            text: settings.appUiLanguage === "en"
                                  ? "Built-in engine options:"
                                  : "Параметры встроенного движка:"
                            color: cMuted
                            font.pixelSize: 12
                        }
                        CheckBox {
                            visible: settings.pdfEngine === "etemenanki"
                            text: settings.appUiLanguage === "en"
                                  ? "Auto layout extract (recommended)"
                                  : "Авто-извлечение layout (рекомендуется)"
                            checked: settings.pdfLayoutAuto
                            onToggled: settings.setPdfLayoutAuto(checked)
                        }
                        CheckBox {
                            visible: settings.pdfEngine === "etemenanki"
                            text: settings.appUiLanguage === "en"
                                  ? "Preserve tables when rebuilding PDF"
                                  : "Сохранять таблицы при сборке PDF"
                            checked: settings.pdfLayoutPreserveTables
                            onToggled: settings.setPdfLayoutPreserveTables(checked)
                        }

                        Rectangle { Layout.fillWidth: true; height: 1; color: cBorder }

                        Label {
                            wrapMode: Text.WordWrap
                            Layout.fillWidth: true
                            text: settings.appUiLanguage === "en"
                                  ? "PyMuPDF and pdf2zh are AGPL-3.0. See engines/THIRD_PARTY.md in the app folder."
                                  : "PyMuPDF и pdf2zh — AGPL-3.0. См. engines/THIRD_PARTY.md в папке приложения."
                            color: cMuted
                            font.pixelSize: 10
                        }

                        Label {
                            wrapMode: Text.WordWrap
                            Layout.fillWidth: true
                            text: settings.appUiLanguage === "en"
                                  ? "With gratitude to the open-source authors:"
                                  : "С благодарностью авторам open-source проектов:"
                            color: cText
                            font.weight: Font.DemiBold
                            font.pixelSize: 12
                        }

                        Repeater {
                            model: pdfSection.engineCatalog
                            delegate: RowLayout {
                                required property var modelData
                                visible: (modelData.url || "").length > 0
                                Layout.fillWidth: true
                                spacing: 6
                                Label {
                                    text: "• " + modelData.name
                                    color: cMuted
                                    font.pixelSize: 11
                                }
                                Button {
                                    text: modelData.url.replace("https://github.com/", "")
                                    flat: true
                                    onClicked: settings.openExternalUrl(modelData.url)
                                }
                            }
                        }
                    }
                }

                ScrollView {
                    clip: true
                    ColumnLayout {
                        width: parent.width
                        spacing: 10
                        Label {
                            wrapMode: Text.WordWrap
                            Layout.fillWidth: true
                            text: settings.appUiLanguage === "en"
                                  ? "Translation hub: glossary and parallel chunk requests."
                                  : "Центр перевода: глоссарий и параллельные запросы к модели."
                            color: cMuted
                            font.pixelSize: 12
                        }
                        CheckBox {
                            text: settings.appUiLanguage === "en"
                                  ? "Use glossary in prompts"
                                  : "Использовать глоссарий в промптах"
                            checked: settings.glossaryEnabled
                            onToggled: settings.setGlossaryEnabled(checked)
                        }
                        Label {
                            text: settings.appUiLanguage === "en"
                                  ? "Glossary (source=target, one pair per line)"
                                  : "Глоссарий (исходник=перевод, по одной паре на строку)"
                            color: cMuted
                            font.pixelSize: 12
                        }
                        TextArea {
                            id: glossaryField
                            Layout.fillWidth: true
                            Layout.preferredHeight: 120
                            wrapMode: TextArea.Wrap
                            text: settings.glossaryText
                            placeholderText: settings.appUiLanguage === "en"
                                ? "Etemenanki=Etemenanki\nAPI=API"
                                : "Etemenanki=Etemenanki\nдоговор=contract"
                            onEditingFinished: settings.setGlossaryText(text)
                            background: Rectangle {
                                radius: 10
                                color: "#fbfcfe"
                                border.color: "#cfd9eb"
                            }
                        }
                        RowLayout {
                            Layout.fillWidth: true
                            Label {
                                text: settings.appUiLanguage === "en"
                                      ? "Parallel chunk requests"
                                      : "Параллельных запросов"
                                color: cMuted
                                font.pixelSize: 12
                            }
                            SpinBox {
                                id: concurrentBox
                                from: 1
                                to: 6
                                value: settings.translateConcurrent
                                onValueModified: settings.setTranslateConcurrent(value)
                            }
                            Item { Layout.fillWidth: true }
                        }
                    }
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            height: 1
            color: cBorder
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.margins: 12
            Item { Layout.fillWidth: true }
            Button {
                text: settings.appUiLanguage === "en" ? "Close" : "Закрыть"
                onClicked: {
                    settings.setOllamaBaseUrl(ollamaField.text)
                    settings.setGlossaryText(glossaryField.text)
                    settings.refreshAvailableModels()
                    dlg.close()
                }
                background: Rectangle {
                    radius: 10
                    color: parent.hovered ? "#eef2f8" : "#ffffff"
                    border.color: "#cfd9eb"
                }
            }
        }
    }
}

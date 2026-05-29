import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Window {
    id: wizard

    EtePalette { id: pal }

    width: 760
    height: 620
    minimumWidth: 680
    minimumHeight: 520
    title: "Etemenanki — " + trRu("Setup", "Настройка")
    color: pal.bg
    modality: Qt.ApplicationModal
    flags: Qt.Dialog | Qt.WindowTitleHint | Qt.WindowCloseButtonHint
    font.family: "Segoe UI"
    font.pixelSize: pal.fontBody

    property int step: 0
    property var selectedModels: []

    readonly property var stepTitles: [
        trRu("Your PC", "Ваш ПК"),
        trRu("AI models", "Модели ИИ"),
        trRu("Components", "Компоненты")
    ]

    function trRu(en, ru) {
        return settings.appUiLanguage === "ru" ? ru : en
    }

    function refreshSelectionFromRecommendations() {
        const out = []
        for (let i = 0; i < setup.recommendations.length; ++i) {
            const row = setup.recommendations[i]
            if (row.recommended)
                out.push(row.id)
        }
        if (out.length === 0 && setup.recommendations.length > 0)
            out.push(setup.recommendations[0].id)
        selectedModels = out
    }

    onVisibleChanged: {
        if (visible && setup.recommendations.length === 0)
            setup.probeHardware()
    }

    Connections {
        target: setup
        function onProbeFinished() { wizard.refreshSelectionFromRecommendations() }
    }

    component WizardCard : Rectangle {
        id: card
        default property alias content: inner.data
        Layout.fillWidth: true
        implicitHeight: inner.implicitHeight + 28
        radius: pal.radiusMd
        color: pal.card
        border.color: pal.border
        border.width: 1
        ColumnLayout {
            id: inner
            anchors.fill: parent
            anchors.margins: 16
            spacing: 10
        }
    }

    component PrimaryBtn : Button {
        implicitHeight: 40
        padding: 14
        font.pixelSize: pal.fontBody
        font.weight: Font.DemiBold
        contentItem: Text {
            text: parent.text
            color: enabled ? "#ffffff" : pal.muted
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            font: parent.font
        }
        background: Rectangle {
            radius: pal.radiusSm
            color: parent.enabled ? (parent.down ? pal.accentHover : pal.accent) : pal.border
        }
    }

    component GhostBtn : Button {
        implicitHeight: 40
        padding: 14
        flat: true
        contentItem: Text {
            text: parent.text
            color: pal.accentText
            font.pixelSize: pal.fontBody
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }
        background: Rectangle {
            radius: pal.radiusSm
            color: parent.hovered ? pal.accentSoft : "transparent"
            border.color: parent.hovered ? pal.accent : "transparent"
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 24
        spacing: 16

        RowLayout {
            Layout.fillWidth: true
            spacing: 16
            Image {
                visible: brandLogoHeader.toString().length > 0
                source: brandLogoHeader
                Layout.preferredWidth: 56
                Layout.preferredHeight: 56
                fillMode: Image.PreserveAspectFit
                smooth: true
            }
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 4
                Label {
                    text: trRu("Welcome to Etemenanki", "Добро пожаловать в Etemenanki")
                    font.pixelSize: pal.fontTitle
                    font.weight: Font.DemiBold
                    color: pal.text
                }
                Label {
                    text: trRu("One-time setup — then translate documents locally or in the cloud.",
                               "Один раз настроили — дальше переводите документы локально или в облаке.")
                    wrapMode: Text.WordWrap
                    Layout.fillWidth: true
                    color: pal.muted
                    font.pixelSize: pal.fontBody
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 8
            Repeater {
                model: wizard.stepTitles.length
                delegate: RowLayout {
                    required property int index
                    spacing: 6
                    Rectangle {
                        width: 28
                        height: 28
                        radius: 14
                        color: wizard.step === index ? pal.accent : (wizard.step > index ? pal.ok : pal.surface)
                        border.color: wizard.step >= index ? "transparent" : pal.border
                        Label {
                            anchors.centerIn: parent
                            text: (index + 1).toString()
                            color: wizard.step >= index ? "#fff" : pal.muted
                            font.weight: Font.DemiBold
                        }
                    }
                    Label {
                        text: wizard.stepTitles[index]
                        color: wizard.step === index ? pal.accentText : pal.muted
                        font.weight: wizard.step === index ? Font.DemiBold : Font.Normal
                        visible: index === wizard.step
                    }
                }
            }
            Item { Layout.fillWidth: true }
            Label {
                text: "v" + setup.appVersion
                color: pal.muted
                font.pixelSize: pal.fontCaption
            }
        }

        StackLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: wizard.step

            // Step 0
            ColumnLayout {
                spacing: 12
                WizardCard {
                    Label {
                        text: trRu("We analyze RAM and GPU to recommend Ollama models.",
                                   "Анализируем ОЗУ и видеокарту, чтобы подобрать модели Ollama.")
                        wrapMode: Text.WordWrap
                        Layout.fillWidth: true
                        color: pal.text
                    }
                    Label {
                        text: setup.statusText
                        color: pal.muted
                        font.pixelSize: pal.fontCaption
                        wrapMode: Text.WordWrap
                        Layout.fillWidth: true
                    }
                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 100
                        radius: pal.radiusSm
                        color: pal.surface
                        border.color: pal.border
                        Label {
                            anchors.fill: parent
                            anchors.margins: 12
                            font.family: "Consolas"
                            font.pixelSize: pal.fontCaption
                            color: pal.text
                            text: {
                                const h = setup.hardware
                                if (!h || h.ram_gb === undefined)
                                    return trRu("Press «Scan» to detect hardware.", "Нажмите «Сканировать».")
                                const vram = (h.vram_gb !== undefined && h.vram_gb !== null)
                                    ? (h.vram_gb + " GB VRAM") : trRu("VRAM: unknown", "VRAM: неизвестно")
                                return "RAM: " + h.ram_gb + " GB\n" + vram + "\n"
                                    + trRu("Profile: ", "Профиль: ") + h.hardware_tier
                            }
                        }
                    }
                    GhostBtn {
                        text: trRu("Scan hardware", "Сканировать ПК")
                        enabled: !setup.busy
                        onClicked: setup.probeHardware()
                    }
                }
            }

            // Step 1
            ColumnLayout {
                spacing: 12
                WizardCard {
                    Layout.fillHeight: true
                    Label {
                        text: trRu("Select models to download (requires Ollama):",
                                   "Выберите модели для загрузки (нужен Ollama):")
                        font.weight: Font.DemiBold
                        color: pal.text
                    }
                    ListView {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clip: true
                        spacing: 4
                        model: setup.recommendations
                        delegate: ItemDelegate {
                            required property var modelData
                            checkable: true
                            width: ListView.view.width
                            height: 52
                            contentItem: ColumnLayout {
                                spacing: 2
                                Label {
                                    text: modelData.id
                                    font.weight: Font.DemiBold
                                    color: pal.text
                                }
                                Label {
                                    text: "★" + modelData.translation_quality + " · " + modelData.quality_label
                                        + (modelData.recommended ? (" · " + wizard.trRu("recommended", "рекомендуется")) : "")
                                    color: modelData.recommended ? pal.ok : pal.muted
                                    font.pixelSize: pal.fontCaption
                                }
                            }
                            checked: wizard.selectedModels.indexOf(modelData.id) >= 0
                            onClicked: {
                                const id = modelData.id
                                let list = wizard.selectedModels.slice()
                                const pos = list.indexOf(id)
                                if (pos >= 0)
                                    list.splice(pos, 1)
                                else
                                    list.push(id)
                                wizard.selectedModels = list
                            }
                            background: Rectangle {
                                radius: pal.radiusSm
                                color: parent.checked ? pal.accentSoft : (parent.hovered ? pal.surface : "transparent")
                                border.color: parent.checked ? pal.accent : pal.border
                                border.width: 1
                            }
                        }
                    }
                    RowLayout {
                        GhostBtn {
                            text: trRu("Download Ollama", "Скачать Ollama")
                            onClicked: setup.openOllamaDownload()
                        }
                        Label {
                            Layout.fillWidth: true
                            text: trRu("Then return here and click Install.", "Вернитесь сюда и нажмите «Установить».")
                            color: pal.muted
                            font.pixelSize: pal.fontCaption
                            wrapMode: Text.WordWrap
                        }
                    }
                }
            }

            // Step 2
            ColumnLayout {
                spacing: 12
                WizardCard {
                    CheckBox {
                        id: cbPdf2zh
                        text: trRu("PDF engine (pdf2zh) — layout-preserving PDF translation",
                                   "PDF-движок (pdf2zh) — перевод PDF с сохранением вёрстки")
                        checked: true
                    }
                    CheckBox {
                        id: cbPythonDeps
                        text: trRu("Python libraries — extract text from DOCX/PDF",
                                   "Библиотеки Python — извлечение текста из DOCX/PDF")
                        checked: true
                    }
                    ProgressBar {
                        Layout.fillWidth: true
                        indeterminate: true
                        visible: setup.busy
                    }
                    ScrollView {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 140
                        TextArea {
                            readOnly: true
                            text: setup.logText
                            font.family: "Consolas"
                            font.pixelSize: pal.fontCaption
                            color: pal.text
                            background: null
                        }
                    }
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 8
            GhostBtn {
                text: trRu("GitHub", "GitHub")
                onClicked: setup.openGitHubRepo()
            }
            Item { Layout.fillWidth: true }
            GhostBtn {
                text: trRu("Back", "Назад")
                visible: wizard.step > 0
                enabled: !setup.busy
                onClicked: wizard.step--
            }
            GhostBtn {
                text: trRu("Skip", "Пропустить")
                enabled: !setup.busy
                onClicked: {
                    setup.markSetupComplete()
                    wizard.close()
                }
            }
            PrimaryBtn {
                text: wizard.step < 2 ? trRu("Next", "Далее") : trRu("Install", "Установить")
                enabled: !setup.busy
                onClicked: {
                    if (wizard.step === 0) {
                        if (setup.recommendations.length === 0)
                            setup.probeHardware()
                        wizard.step = 1
                    } else if (wizard.step === 1) {
                        wizard.step = 2
                    } else {
                        setup.runSetup(wizard.selectedModels, cbPdf2zh.checked, cbPythonDeps.checked)
                    }
                }
            }
            PrimaryBtn {
                text: trRu("Done", "Готово")
                visible: wizard.step === 2 && setup.setupComplete
                onClicked: wizard.close()
            }
        }
    }

    Connections {
        target: setup
        function onSetupFinished(ok) {
            if (ok)
                wizard.step = 2
        }
    }
}

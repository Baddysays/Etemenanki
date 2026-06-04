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

    property int step: 0
    property var selectedModels: []
    property double hwRamGb: -1
    property double hwVramGb: -1
    property string hwTier: ""
    property string hwTierLabel: ""
    property string localAiMode: "auto"
    property int gpuPickIndex: -1

    readonly property var gpuList: {
        const h = setup.hardware
        return (h && h.gpus) ? h.gpus : []
    }

    readonly property var stepTitles: [
        trRu("Your PC", "Ваш ПК"),
        trRu("AI models", "Модели ИИ"),
        trRu("Components", "Компоненты")
    ]

    function trRu(en, ru) {
        return settings.appUiLanguage === "ru" ? ru : en
    }

    function modelSubtitle(row) {
        if (!row)
            return ""
        const q = trRu(row.quality_label_en || row.quality_label || "",
                       row.quality_label_ru || row.quality_label || "")
        const sp = trRu(row.speed_label_en || row.speed_label || "",
                        row.speed_label_ru || row.speed_label || "")
        let line = "★" + row.translation_quality + " · " + q
        if (sp.length > 0)
            line += " · " + sp
        if (row.recommended)
            line += " · " + trRu("recommended", "рекомендуется")
        else if (row.fits_hardware === false)
            line += " · " + trRu("too heavy for this PC", "тяжело для этого ПК")
        return line
    }

    function tierLabel(tier) {
        const labels = setup.hardware.tier_labels
        if (!labels || !tier)
            return tier || "—"
        const block = labels[tier]
        if (!block)
            return tier
        return settings.appUiLanguage === "ru"
            ? (block.label_ru || block.label_en || tier)
            : (block.label_en || block.label_ru || tier)
    }

    function pythonStatusLine() {
        const d = setup.depsStatus
        if (!d || d.python !== true)
            return trRu("Python: not found — set path in tools/python_path.txt or install from python.org",
                        "Python: не найден — укажите путь в tools/python_path.txt или установите с python.org")
        const ver = d.python_version ? String(d.python_version) : "?"
        const libsOk = d.python_libs === true
        const libs = libsOk
            ? trRu("libraries OK", "библиотеки OK")
            : trRu("libraries missing", "библиотеки не установлены")
        let path = d.python_path ? String(d.python_path) : ""
        if (path.length > 52)
            path = "…" + path.slice(-49)
        return "Python " + ver + " — " + libs + (path.length > 0 ? (" · " + path) : "")
    }

    function syncHardwareFromSetup() {
        const h = setup.hardware
        hwRamGb = (h && h.ram_gb !== undefined) ? Number(h.ram_gb) : -1
        hwVramGb = (h && h.vram_gb !== undefined && h.vram_gb !== null) ? Number(h.vram_gb) : -1
        hwTier = (h && h.hardware_tier) ? String(h.hardware_tier) : ""
        hwTierLabel = tierLabel(hwTier)
    }

    function refreshSelectionFromRecommendations() {
        const out = []
        for (let i = 0; i < setup.recommendations.length; ++i) {
            const row = setup.recommendations[i]
            if (wizard.localAiMode === "embedded" && row.provider !== "embedded")
                continue
            if (wizard.localAiMode !== "embedded" && row.provider === "embedded")
                continue
            if (row.recommended && !row.installed)
                out.push(row.id)
        }
        if (out.length === 0) {
            for (let j = 0; j < setup.recommendations.length; ++j) {
                const r = setup.recommendations[j]
                if (wizard.localAiMode === "embedded" && r.provider !== "embedded")
                    continue
                if (wizard.localAiMode !== "embedded" && r.provider === "embedded")
                    continue
                if (r.recommended)
                    out.push(r.id)
            }
        }
        if (out.length === 0 && setup.recommendations.length > 0)
            out.push(setup.recommendations[0].id)
        selectedModels = out
    }

    function applyWizardSettings() {
        settings.setLocalAiMode(localAiMode)
        let idx = gpuPickIndex
        if (idx < 0) {
            const h = setup.hardware
            if (h && h.recommended_gpu_index !== undefined)
                idx = Number(h.recommended_gpu_index)
        }
        if (idx >= 0)
            settings.setPreferredGpuIndex(idx)
    }

    function installSummary() {
        const parts = []
        if (selectedModels.length > 0)
            parts.push(trRu("download models: ", "скачать модели: ") + selectedModels.join(", "))
        if (cbPdf2zh.selected)
            parts.push(trRu("PDF engine", "PDF-движок"))
        if (wizard.localAiMode === "embedded")
            parts.push(trRu("built-in AI model ~1.7 GB", "встроенная модель ИИ ~1,7 ГБ"))
        if (cbPythonDeps.selected && setup.depsStatus.python === true)
            parts.push(trRu("Python libraries", "библиотеки Python"))
        if (parts.length === 0)
            return trRu("Nothing selected — click Done to continue.",
                        "Ничего не выбрано — нажмите «Готово» для продолжения.")
        return trRu("Will install: ", "Будет установлено: ") + parts.join("; ")
    }

    onVisibleChanged: {
        if (visible) {
            if (!setup.probeReady && !setup.busy)
                setup.probeHardware()
        }
    }

    Connections {
        target: setup
        function onProbeFinished(ok) {
            if (ok) {
                wizard.syncHardwareFromSetup()
                wizard.refreshSelectionFromRecommendations()
                const h = setup.hardware
                if (h && h.recommended_gpu_index !== undefined && wizard.gpuPickIndex < 0)
                    wizard.gpuPickIndex = Number(h.recommended_gpu_index)
            }
        }
        function onHardwareChanged() { wizard.syncHardwareFromSetup() }
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

    component OptionCard : Rectangle {
        id: opt
        property alias title: titleLbl.text
        property alias subtitle: subLbl.text
        property bool selected: true
        default property alias content: extra.data
        signal toggled(bool on)

        Layout.fillWidth: true
        implicitHeight: inner.implicitHeight + 24
        radius: pal.radiusSm
        color: !enabled ? pal.surface : (selected ? pal.accentSoft : (hover.hovered ? pal.surface : pal.card))
        border.color: !enabled ? pal.border : (selected ? pal.accent : pal.border)
        border.width: selected ? 1.5 : 1

        HoverHandler { id: hover; enabled: opt.enabled }

        MouseArea {
            anchors.fill: parent
            enabled: opt.enabled
            cursorShape: Qt.PointingHandCursor
            onClicked: {
                opt.selected = !opt.selected
                opt.toggled(opt.selected)
            }
        }

        RowLayout {
            id: inner
            anchors.fill: parent
            anchors.margins: 14
            spacing: 12

            Rectangle {
                Layout.alignment: Qt.AlignTop
                Layout.topMargin: 1
                width: 22
                height: 22
                radius: 6
                color: !opt.enabled ? pal.surface
                                    : (opt.selected ? pal.accent : pal.card)
                border.color: !opt.enabled ? pal.border
                                           : (opt.selected ? pal.accent : pal.border)
                border.width: opt.selected ? 0 : 1
                Text {
                    anchors.centerIn: parent
                    text: "✓"
                    color: "#ffffff"
                    font.pixelSize: 12
                    font.weight: Font.Bold
                    visible: opt.selected && opt.enabled
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 3
                Label {
                    id: titleLbl
                    Layout.fillWidth: true
                    wrapMode: Text.WordWrap
                    color: opt.enabled ? pal.text : pal.muted
                    font.pixelSize: pal.fontBody
                    font.weight: Font.DemiBold
                }
                Label {
                    id: subLbl
                    Layout.fillWidth: true
                    wrapMode: Text.WordWrap
                    visible: text.length > 0
                    color: {
                        if (!opt.enabled)
                            return pal.muted
                        if (text.indexOf("✓") >= 0 || text.indexOf("OK") >= 0)
                            return pal.ok
                        return pal.muted
                    }
                    font.pixelSize: pal.fontCaption
                }
                ColumnLayout {
                    id: extra
                    Layout.fillWidth: true
                    spacing: 0
                }
            }
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
                            text: wizard.step > index ? "✓" : (index + 1).toString()
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

            // Step 0 — AI mode + hardware scan
            ColumnLayout {
                spacing: 12
                WizardCard {
                    Label {
                        text: trRu("How should Etemenanki translate?", "Как Etemenanki будет переводить?")
                        font.weight: Font.DemiBold
                        color: pal.text
                    }
                    OptionCard {
                        id: modeAuto
                        selected: wizard.localAiMode === "auto"
                        title: trRu("Automatic (recommended)", "Автоматически (рекомендуется)")
                        subtitle: trRu("Installs Ollama + model, starts when you translate — easiest",
                                      "Ставит Ollama + модель, запускает при переводе — проще всего")
                        onToggled: function(on) {
                            if (on) wizard.localAiMode = "auto"
                        }
                    }
                    OptionCard {
                        id: modeOllama
                        selected: wizard.localAiMode === "ollama"
                        title: trRu("I already use Ollama", "У меня уже есть Ollama")
                        subtitle: trRu("You start ollama serve yourself", "Вы сами запускаете ollama serve")
                        onToggled: function(on) {
                            if (on) wizard.localAiMode = "ollama"
                        }
                    }
                    OptionCard {
                        selected: wizard.localAiMode === "embedded"
                        title: trRu("Built-in model (no Ollama)", "Встроенная модель (без Ollama)")
                        subtitle: trRu("One download ~1.7 GB — no Ollama install",
                                      "Одна загрузка ~1,7 ГБ — Ollama не нужен")
                        onToggled: function(on) {
                            if (on) {
                                wizard.localAiMode = "embedded"
                                wizard.refreshSelectionFromRecommendations()
                            }
                        }
                    }
                    Label {
                        visible: wizard.gpuList.length > 0
                        text: trRu("GPU for AI (use discrete if you have two)",
                                   "Видеокарта для ИИ (выберите дискретную, если их две)")
                        font.weight: Font.DemiBold
                        color: pal.text
                    }
                    ComboBox {
                        id: gpuBox
                        visible: wizard.gpuList.length > 0
                        Layout.fillWidth: true
                        model: wizard.gpuList
                        delegate: ItemDelegate {
                            required property int index
                            required property var modelData
                            width: gpuBox.width
                            contentItem: Label {
                                text: gpuBox.textAt(index)
                                font.pixelSize: pal.fontCaption
                            }
                        }
                        textRole: "name"
                        onModelChanged: syncGpuIndex()
                        Component.onCompleted: syncGpuIndex()
                        function syncGpuIndex() {
                            for (let i = 0; i < wizard.gpuList.length; ++i) {
                                if (Number(wizard.gpuList[i].index) === wizard.gpuPickIndex) {
                                    currentIndex = i
                                    return
                                }
                            }
                            if (wizard.gpuList.length > 0)
                                currentIndex = 0
                        }
                        onActivated: function(idx) {
                            if (wizard.gpuList[idx])
                                wizard.gpuPickIndex = Number(wizard.gpuList[idx].index)
                        }
                    }
                    GhostBtn {
                        text: trRu("Windows GPU settings for Ollama", "Настройки Windows: GPU для Ollama")
                        onClicked: setup.openWindowsGpuSettings()
                    }
                    Label {
                        text: trRu("We analyze RAM and GPU to recommend Ollama models.",
                                   "Анализируем ОЗУ и видеокарту, чтобы подобрать модели Ollama.")
                        wrapMode: Text.WordWrap
                        Layout.fillWidth: true
                        color: pal.text
                    }
                    Label {
                        text: setup.busy
                            ? trRu("Scanning…", "Сканирование…")
                            : (setup.statusText || trRu("Waiting for scan", "Ожидание сканирования"))
                        color: setup.busy ? pal.accentText : pal.muted
                        font.pixelSize: pal.fontCaption
                        wrapMode: Text.WordWrap
                        Layout.fillWidth: true
                    }
                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 120
                        radius: pal.radiusSm
                        color: pal.surface
                        border.color: pal.border
                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: 12
                            spacing: 6
                            Label {
                                visible: wizard.hwRamGb < 0
                                text: trRu("Press «Scan» or wait — detecting your PC…",
                                           "Нажмите «Сканировать» или подождите…")
                                color: pal.muted
                                wrapMode: Text.WordWrap
                                Layout.fillWidth: true
                            }
                            Label {
                                visible: wizard.hwRamGb >= 0
                                text: trRu("RAM", "ОЗУ") + ": " + wizard.hwRamGb.toFixed(1) + " GB"
                                color: pal.text
                                font.weight: Font.DemiBold
                            }
                            Label {
                                visible: wizard.hwVramGb >= 0
                                text: "VRAM: " + wizard.hwVramGb.toFixed(1) + " GB"
                                color: pal.text
                            }
                            Label {
                                visible: wizard.hwTier.length > 0
                                text: trRu("Profile", "Профиль") + ": " + wizard.hwTierLabel
                                color: pal.accentText
                            }
                            Label {
                                visible: setup.ollamaInstalled.length > 0
                                text: trRu("Ollama models found", "Модели Ollama") + ": "
                                    + setup.ollamaInstalled.join(", ")
                                color: pal.ok
                                wrapMode: Text.WordWrap
                                Layout.fillWidth: true
                                font.pixelSize: pal.fontCaption
                            }
                            Label {
                                visible: setup.probeReady
                                text: wizard.pythonStatusLine()
                                color: setup.depsStatus.python === true ? pal.ok : pal.warn
                                wrapMode: Text.WordWrap
                                Layout.fillWidth: true
                                font.pixelSize: pal.fontCaption
                            }
                        }
                    }
                    RowLayout {
                        Layout.fillWidth: true
                        GhostBtn {
                            text: trRu("Scan again", "Сканировать снова")
                            enabled: !setup.busy
                            onClicked: setup.probeHardware()
                        }
                        Item { Layout.fillWidth: true }
                        Label {
                            visible: setup.probeReady && setup.depsStatus.python === true
                            text: trRu("Python: OK", "Python: OK")
                            color: pal.ok
                            font.pixelSize: pal.fontCaption
                        }
                        Label {
                            visible: setup.probeReady && setup.depsStatus.python === true
                                      && setup.depsStatus.python_libs !== true
                            text: trRu("libs missing", "нет библиотек")
                            color: pal.warn
                            font.pixelSize: pal.fontCaption
                        }
                        Label {
                            visible: setup.probeReady && setup.depsStatus.python !== true
                            text: trRu("Python: not found", "Python: не найден")
                            color: pal.warn
                            font.pixelSize: pal.fontCaption
                        }
                        Label {
                            visible: setup.depsStatus.ollama === true
                            text: trRu("Ollama: connected", "Ollama: подключён")
                            color: pal.ok
                            font.pixelSize: pal.fontCaption
                        }
                        Label {
                            visible: setup.probeReady && setup.depsStatus.ollama !== true
                            text: trRu("Ollama: not found", "Ollama: не найден")
                            color: pal.warn
                            font.pixelSize: pal.fontCaption
                        }
                    }
                }
            }

            // Step 1 — models
            ColumnLayout {
                spacing: 12
                WizardCard {
                    Layout.fillHeight: true
                    Label {
                        text: trRu("Recommended models for your PC. Already installed models are marked.",
                                   "Рекомендуемые модели для вашего ПК. Установленные отмечены.")
                        wrapMode: Text.WordWrap
                        Layout.fillWidth: true
                        color: pal.text
                    }
                    Label {
                        visible: setup.recommendations.length === 0 && !setup.busy
                        text: trRu("No models yet — go back and run Scan.",
                                   "Список пуст — вернитесь и выполните сканирование.")
                        color: pal.warn
                        wrapMode: Text.WordWrap
                        Layout.fillWidth: true
                    }
                    ListView {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clip: true
                        spacing: 4
                        model: setup.recommendations
                        delegate: ItemDelegate {
                            required property var modelData
                            visible: wizard.localAiMode === "embedded"
                                     ? (modelData.provider === "embedded")
                                     : (modelData.provider !== "embedded")
                            checkable: true
                            width: ListView.view.width
                            height: visible ? 58 : 0
                            enabled: !modelData.installed
                            contentItem: RowLayout {
                                spacing: 8
                                ColumnLayout {
                                    Layout.fillWidth: true
                                    spacing: 2
                                    Label {
                                        text: modelData.id
                                        font.weight: Font.DemiBold
                                        color: parent.parent.enabled ? pal.text : pal.muted
                                    }
                                    Label {
                                        text: wizard.modelSubtitle(modelData)
                                        color: modelData.recommended ? pal.ok : pal.muted
                                        font.pixelSize: pal.fontCaption
                                    }
                                }
                                Label {
                                    visible: modelData.installed === true
                                    text: trRu("Installed", "Установлена")
                                    color: pal.ok
                                    font.weight: Font.DemiBold
                                    font.pixelSize: pal.fontCaption
                                }
                            }
                            checked: modelData.installed || wizard.selectedModels.indexOf(modelData.id) >= 0
                            onClicked: {
                                if (modelData.installed)
                                    return
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
                            text: trRu("Models marked «Installed» are already on your PC.",
                                       "Модели с пометкой «Установлена» уже есть на компьютере.")
                            color: pal.muted
                            font.pixelSize: pal.fontCaption
                            wrapMode: Text.WordWrap
                        }
                    }
                }
            }

            // Step 2 — install components
            ColumnLayout {
                spacing: 12
                WizardCard {
                    Label {
                        text: trRu("Optional components for PDF and document translation.",
                                   "Дополнительные компоненты для перевода PDF и документов.")
                        wrapMode: Text.WordWrap
                        Layout.fillWidth: true
                        color: pal.text
                        font.weight: Font.DemiBold
                    }
                    Label {
                        text: wizard.installSummary()
                        wrapMode: Text.WordWrap
                        Layout.fillWidth: true
                        color: pal.muted
                        font.pixelSize: pal.fontCaption
                    }
                    Label {
                        text: wizard.pythonStatusLine()
                        wrapMode: Text.WordWrap
                        Layout.fillWidth: true
                        color: setup.depsStatus.python === true ? pal.text : pal.warn
                        font.pixelSize: pal.fontCaption
                    }

                    OptionCard {
                        id: cbPdf2zh
                        selected: true
                        title: trRu("PDF engine (pdf2zh)", "PDF-движок (pdf2zh)")
                        subtitle: setup.depsStatus.pdf2zh === true
                            ? trRu("Already installed ✓", "Уже установлен ✓")
                            : trRu("Layout-preserving PDF translation", "Перевод PDF с сохранением вёрстки")
                    }
                    Label {
                        visible: wizard.localAiMode === "embedded"
                        text: setup.depsStatus.embedded === true
                            ? trRu("Built-in model: downloaded ✓", "Встроенная модель: скачана ✓")
                            : trRu("Built-in model: not yet downloaded (~1.7 GB)",
                                   "Встроенная модель: ещё не скачана (~1,7 ГБ)")
                        color: setup.depsStatus.embedded === true ? pal.ok : pal.warn
                        font.pixelSize: pal.fontCaption
                        wrapMode: Text.WordWrap
                        Layout.fillWidth: true
                    }
                    PrimaryBtn {
                        visible: wizard.localAiMode === "embedded"
                                 && setup.depsStatus.embedded !== true
                                 && !setup.busy
                        text: trRu("Download built-in model now", "Скачать встроенную модель сейчас")
                        onClicked: setup.downloadEmbeddedModel()
                    }
                    OptionCard {
                        id: cbPythonDeps
                        selected: true
                        enabled: setup.depsStatus.python === true
                        title: trRu("Python libraries", "Библиотеки Python")
                        subtitle: {
                            if (setup.depsStatus.python !== true)
                                return trRu("Python not found — install Python first",
                                            "Python не найден — сначала установите Python")
                            if (setup.depsStatus.python_libs === true)
                                return trRu("PyMuPDF, python-docx — already installed ✓",
                                            "PyMuPDF, python-docx — уже установлены ✓")
                            return trRu("PyMuPDF, python-docx — text extraction from DOCX/PDF",
                                        "PyMuPDF, python-docx — извлечение текста из DOCX/PDF")
                        }
                    }

                    Label {
                        visible: setup.busy
                        text: setup.statusText
                        wrapMode: Text.WordWrap
                        Layout.fillWidth: true
                        color: pal.accentText
                        font.weight: Font.DemiBold
                    }
                    ProgressBar {
                        id: dlProgress
                        Layout.fillWidth: true
                        from: 0
                        to: 100
                        value: setup.downloadProgress >= 0 ? setup.downloadProgress : 0
                        visible: setup.busy && setup.downloadProgress >= 0
                    }
                    ProgressBar {
                        Layout.fillWidth: true
                        indeterminate: true
                        visible: setup.busy && setup.downloadProgress < 0
                    }
                    Label {
                        visible: setup.busy && setup.downloadProgress >= 0
                        text: setup.downloadProgress + "%"
                        color: pal.muted
                        font.pixelSize: pal.fontCaption
                    }
                    GhostBtn {
                        visible: setup.busy || setup.logText.length > 0
                        text: trRu("Open install log", "Открыть журнал установки")
                        onClicked: setup.openInstallLog()
                    }
                    ScrollView {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 120
                        visible: setup.logText.length > 0
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
                    wizard.applyWizardSettings()
                    wizard.applyWizardSettings()
                    setup.markSetupComplete()
                    wizard.close()
                }
            }
            PrimaryBtn {
                text: wizard.step < 2 ? trRu("Next", "Далее") : trRu("Install", "Установить")
                visible: !(wizard.step === 2 && setup.setupComplete)
                enabled: !setup.busy && (wizard.step !== 0 || setup.probeReady)
                onClicked: {
                    if (wizard.step === 0) {
                        wizard.step = 1
                    } else if (wizard.step === 1) {
                        wizard.step = 2
                    } else {
                        setup.runSetup(
                            wizard.localAiMode === "embedded" ? [] : wizard.selectedModels,
                            cbPdf2zh.selected,
                            cbPythonDeps.selected || wizard.localAiMode === "embedded",
                            wizard.localAiMode === "embedded")
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
            if (ok) {
                wizard.applyWizardSettings()
                setup.markSetupComplete()
            }
        }
    }
}

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Window {
    id: win

    EtePalette { id: pal }

    title: settings.uiText("settings", settings.appUiLanguage)
    width: 860
    height: 700
    minimumWidth: 640
    minimumHeight: 480
    color: pal.bg
    flags: Qt.Window | Qt.WindowTitleHint | Qt.WindowSystemMenuHint
           | Qt.WindowMinMaxButtonsHint | Qt.WindowCloseButtonHint

    property int section: 0

    ListModel { id: uiLangModel }

    function reloadUiLangModel() {
        uiLangModel.clear()
        const opts = settings.appUiLanguageOptions()
        for (let i = 0; i < opts.length; ++i) {
            uiLangModel.append({
                code: opts[i].code,
                label: opts[i].label
            })
        }
    }

    readonly property var settingsGpuList: {
        const h = setup.hardware
        return (h && h.gpus) ? h.gpus : []
    }

    Component.onCompleted: {
        reloadUiLangModel()
        if (!setup.probeReady && !setup.busy)
            setup.probeHardware()
    }

    Connections {
        target: settings
        function onChanged() {
            reloadUiLangModel()
            if (typeof uiLangCombo !== "undefined")
                uiLangCombo.currentIndex = settings.appUiLanguageOptionIndex()
        }
    }

    // ——— styled controls ———

    component SectionTitle : Label {
        color: pal.muted
        font.pixelSize: pal.fontCaption
        font.weight: Font.Medium
        font.capitalization: Font.AllUppercase
        font.letterSpacing: 0.6
    }

    component BodyLabel : Label {
        color: pal.text
        font.pixelSize: pal.fontBody
        wrapMode: Text.WordWrap
        Layout.fillWidth: true
    }

    component FieldInput : TextField {
        implicitHeight: 40
        leftPadding: 12
        rightPadding: 12
        color: pal.text
        font.pixelSize: pal.fontBody
        selectionColor: pal.accentSoft
        selectedTextColor: pal.text
        placeholderTextColor: pal.muted
        background: Rectangle {
            radius: pal.radiusSm
            color: pal.card
            border.color: parent.activeFocus ? pal.accent : pal.border
            border.width: parent.activeFocus ? 2 : 1
        }
    }

    component FieldArea : TextArea {
        leftPadding: 12
        rightPadding: 12
        topPadding: 10
        bottomPadding: 10
        color: pal.text
        font.pixelSize: pal.fontBody
        wrapMode: TextArea.Wrap
        placeholderTextColor: pal.muted
        background: Rectangle {
            radius: pal.radiusMd
            color: pal.card
            border.color: parent.activeFocus ? pal.accent : pal.border
            border.width: parent.activeFocus ? 2 : 1
        }
    }

    component SectionCard : Rectangle {
        default property alias content: cardCol.data
        Layout.fillWidth: true
        implicitHeight: cardCol.implicitHeight + 28
        radius: pal.radiusMd
        color: pal.card
        border.color: pal.border
        border.width: 1

        ColumnLayout {
            id: cardCol
            anchors.fill: parent
            anchors.margins: 16
            spacing: 12
        }
    }

    component NavItem : ItemDelegate {
        id: navItem
        required property int navId
        required property string navTitle
        width: ListView.view ? ListView.view.width : 200
        height: 44

        contentItem: Label {
            text: navTitle
            color: win.section === navId ? pal.accentText : pal.text
            font.weight: win.section === navId ? Font.DemiBold : Font.Normal
            font.pixelSize: pal.fontBody
            leftPadding: 14
        }
        background: Rectangle {
            radius: pal.radiusSm
            color: win.section === navId ? pal.accentSoft
                                         : (navItem.hovered ? pal.surface : "transparent")
        }
        onClicked: win.section = navId
    }

    component PrimaryBtn : Button {
        implicitHeight: 40
        padding: 12
        Layout.minimumWidth: 120
        contentItem: Label {
            text: parent.text
            color: "#ffffff"
            font.weight: Font.DemiBold
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }
        background: Rectangle {
            radius: pal.radiusSm
            color: parent.down ? "#1d4ed8" : (parent.hovered ? pal.accentHover : pal.accent)
        }
    }

    component SecondaryBtn : Button {
        implicitHeight: 40
        padding: 12
        contentItem: Label {
            text: parent.text
            color: pal.text
            font.weight: Font.Medium
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }
        background: Rectangle {
            radius: pal.radiusSm
            color: parent.hovered ? pal.surface : pal.card
            border.color: pal.border
            border.width: 1
        }
    }

    component LinkBtn : Button {
        flat: true
        implicitHeight: 32
        contentItem: Label {
            text: parent.text
            color: pal.accent
            font.pixelSize: pal.fontCaption
            font.underline: parent.hovered
        }
    }

    component StyledCombo : ComboBox {
        implicitHeight: 40
        leftPadding: 12
        rightPadding: 12
        popup.height: Math.min(count * 36 + 16, 400)
        background: Rectangle {
            radius: pal.radiusSm
            color: pal.card
            border.color: parent.activeFocus ? pal.accent : pal.border
            border.width: parent.activeFocus ? 2 : 1
        }
        contentItem: Text {
            text: parent.displayText
            color: pal.text
            font.pixelSize: pal.fontBody
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
        }
        indicator: Text {
            x: parent.width - width - 12
            y: (parent.height - height) / 2
            text: "▾"
            color: pal.muted
            font.pixelSize: 10
        }
        delegate: ItemDelegate {
            width: parent.width
            height: 36
            required property string label
            contentItem: Text {
                text: label
                color: pal.text
                leftPadding: 12
                verticalAlignment: Text.AlignVCenter
            }
            background: Rectangle {
                radius: 4
                color: highlighted ? pal.accentSoft : (parent.hovered ? pal.surface : "transparent")
            }
        }
        popup.background: Rectangle {
            radius: pal.radiusMd
            color: pal.card
            border.color: pal.border
            border.width: 1
        }
    }

    component StyledCheck : CheckBox {
        id: chk
        implicitHeight: 32
        spacing: 10
        indicator: Rectangle {
            implicitWidth: 20
            implicitHeight: 20
            x: chk.leftPadding
            y: (chk.height - height) / 2
            radius: 5
            color: chk.checked ? pal.accent : pal.card
            border.color: chk.checked ? pal.accent : pal.border
            border.width: chk.checked ? 0 : 1
            Text {
                anchors.centerIn: parent
                text: "✓"
                color: "#ffffff"
                font.pixelSize: 11
                font.weight: Font.Bold
                visible: chk.checked
            }
        }
        contentItem: Text {
            text: chk.text
            color: pal.text
            font.pixelSize: pal.fontBody
            leftPadding: chk.indicator.width + chk.spacing
            verticalAlignment: Text.AlignVCenter
        }
    }

    component StyledRadio : RadioButton {
        id: rad
        implicitHeight: 36
        spacing: 10
        indicator: Rectangle {
            implicitWidth: 20
            implicitHeight: 20
            x: rad.leftPadding
            y: (rad.height - height) / 2
            radius: 10
            color: pal.card
            border.color: rad.checked ? pal.accent : pal.border
            border.width: 2
            Rectangle {
                width: 10
                height: 10
                anchors.centerIn: parent
                radius: 5
                color: pal.accent
                visible: rad.checked
            }
        }
        contentItem: Text {
            text: rad.text
            color: pal.text
            font.pixelSize: pal.fontBody
            leftPadding: rad.indicator.width + rad.spacing
            verticalAlignment: Text.AlignVCenter
        }
    }

    component StyledSpin : SpinBox {
        id: spin
        implicitHeight: 40
        editable: true
        contentItem: TextInput {
            text: spin.textFromValue(spin.value, spin.locale)
            font.pixelSize: pal.fontBody
            color: pal.text
            horizontalAlignment: Qt.AlignHCenter
            verticalAlignment: Qt.AlignVCenter
            readOnly: !spin.editable
            validator: spin.validator
        }
        background: Rectangle {
            radius: pal.radiusSm
            color: pal.card
            border.color: parent.activeFocus ? pal.accent : pal.border
            border.width: parent.activeFocus ? 2 : 1
        }
        up.indicator: Rectangle {
            x: parent.width - 28
            y: 6
            width: 22
            height: (parent.height - 12) / 2
            radius: 4
            color: parent.up.pressed ? pal.accentSoft : pal.surface
            Text { anchors.centerIn: parent; text: "▲"; font.pixelSize: 8; color: pal.muted }
        }
        down.indicator: Rectangle {
            x: parent.width - 28
            y: parent.height / 2 + 1
            width: 22
            height: (parent.height - 12) / 2
            radius: 4
            color: parent.down.pressed ? pal.accentSoft : pal.surface
            Text { anchors.centerIn: parent; text: "▼"; font.pixelSize: 8; color: pal.muted }
        }
    }

    component SettingsPage : ScrollView {
        id: pageScroll
        default property alias content: pageCol.data
        clip: true
        ScrollBar.vertical.policy: ScrollBar.AsNeeded
        ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

        background: Rectangle { color: "transparent" }

        ColumnLayout {
            id: pageCol
            width: Math.max(pageScroll.availableWidth, 320)
            spacing: 16
        }
    }

    // ——— layout ———

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 56
            color: pal.card
            Rectangle {
                anchors.bottom: parent.bottom
                width: parent.width
                height: 1
                color: pal.border
            }
            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 20
                anchors.rightMargin: 16
                Label {
                    text: settings.uiText("settings", settings.appUiLanguage)
                    font.pixelSize: pal.fontTitle
                    font.weight: Font.Bold
                    color: pal.text
                }
                Item { Layout.fillWidth: true }
                SecondaryBtn {
                    text: settings.uiText("settings_close", settings.appUiLanguage)
                    onClicked: win.persistAndClose()
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

            Rectangle {
                Layout.preferredWidth: 228
                Layout.fillHeight: true
                color: pal.surface
                Rectangle {
                    anchors.right: parent.right
                    width: 1
                    height: parent.height
                    color: pal.border
                }
                ListView {
                    id: nav
                    anchors.fill: parent
                    anchors.margins: 12
                    spacing: 4
                    clip: true
                    model: [
                        { id: 0, title: settings.uiText("settings_nav_general", settings.appUiLanguage) },
                        { id: 1, title: settings.uiText("settings_nav_languages", settings.appUiLanguage) },
                        { id: 2, title: settings.uiText("settings_nav_models", settings.appUiLanguage) },
                        { id: 3, title: settings.uiText("settings_nav_cloud", settings.appUiLanguage) },
                        { id: 4, title: settings.uiText("settings_nav_pdf", settings.appUiLanguage) },
                        { id: 5, title: settings.uiText("settings_nav_translation", settings.appUiLanguage) }
                    ]
                    delegate: NavItem {
                        required property int index
                        navId: nav.model[index].id
                        navTitle: nav.model[index].title
                    }
                }
            }

            StackLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.margins: 20
                currentIndex: win.section

                // General
                SettingsPage {
                    SectionTitle { text: settings.uiText("settings_interface", settings.appUiLanguage) }
                    SectionCard {
                        SectionTitle {
                            text: settings.uiText("settings_app_language", settings.appUiLanguage)
                        }
                        StyledCombo {
                            id: uiLangCombo
                            Layout.fillWidth: true
                            model: uiLangModel
                            textRole: "label"
                            currentIndex: settings.appUiLanguageOptionIndex()
                            onActivated: function(index) {
                                if (index >= 0 && index < uiLangModel.count)
                                    settings.setAppUiLanguage(uiLangModel.get(index).code)
                            }
                        }
                    }
                    SectionTitle { text: "Ollama" }
                    SectionCard {
                        SectionTitle {
                            text: settings.appUiLanguage === "ru" ? "Режим ИИ" : "AI mode"
                        }
                        StyledCombo {
                            id: aiModeCombo
                            Layout.fillWidth: true
                            model: [
                                { id: "auto", label: settings.appUiLanguage === "ru"
                                    ? "Автоматически (рекомендуется)" : "Automatic (recommended)" },
                                { id: "ollama", label: settings.appUiLanguage === "ru"
                                    ? "Свой Ollama" : "My Ollama" },
                                { id: "embedded", label: settings.appUiLanguage === "ru"
                                    ? "Встроенная модель (скоро)" : "Built-in model (soon)" }
                            ]
                            textRole: "label"
                            currentIndex: {
                                if (settings.localAiMode === "ollama") return 1
                                if (settings.localAiMode === "embedded") return 2
                                return 0
                            }
                            onActivated: function(index) {
                                const row = aiModeCombo.model[index]
                                if (row)
                                    settings.setLocalAiMode(row.id)
                            }
                        }
                        SecondaryBtn {
                            visible: settings.localAiMode !== "embedded"
                            text: settings.appUiLanguage === "ru"
                                ? "Запустить Ollama сейчас" : "Start Ollama now"
                            onClicked: setup.ensureOllamaServing()
                        }
                        SecondaryBtn {
                            visible: settings.localAiMode === "embedded"
                            text: settings.appUiLanguage === "ru"
                                ? "Скачать встроенную модель (~1,7 ГБ)"
                                : "Download built-in model (~1.7 GB)"
                            enabled: !setup.busy
                            onClicked: setup.downloadEmbeddedModel()
                        }
                        SecondaryBtn {
                            visible: settings.localAiMode === "embedded"
                            text: settings.appUiLanguage === "ru"
                                ? "Запустить встроенную модель" : "Start built-in model"
                            onClicked: setup.ensureEmbeddedLlmServing()
                        }
                        Label {
                            visible: win.settingsGpuList.length > 0
                            text: settings.appUiLanguage === "ru"
                                ? "Видеокарта для ИИ (дискретная, если их две)"
                                : "GPU for AI (discrete if you have two)"
                            color: pal.muted
                            font.pixelSize: pal.fontCaption
                        }
                        StyledCombo {
                            id: gpuSettingsCombo
                            visible: win.settingsGpuList.length > 0
                            Layout.fillWidth: true
                            model: win.settingsGpuList
                            textRole: "name"
                            Component.onCompleted: syncGpuFromSettings()
                            onModelChanged: syncGpuFromSettings()
                            function syncGpuFromSettings() {
                                const pick = settings.preferredGpuIndex
                                for (let i = 0; i < win.settingsGpuList.length; ++i) {
                                    if (Number(win.settingsGpuList[i].index) === pick) {
                                        currentIndex = i
                                        return
                                    }
                                }
                                const h = setup.hardware
                                if (h && h.recommended_gpu_index !== undefined) {
                                    const rec = Number(h.recommended_gpu_index)
                                    for (let j = 0; j < win.settingsGpuList.length; ++j) {
                                        if (Number(win.settingsGpuList[j].index) === rec) {
                                            currentIndex = j
                                            settings.setPreferredGpuIndex(rec)
                                            return
                                        }
                                    }
                                }
                                if (win.settingsGpuList.length > 0)
                                    currentIndex = 0
                            }
                            onActivated: function(index) {
                                const row = win.settingsGpuList[index]
                                if (row)
                                    settings.setPreferredGpuIndex(Number(row.index))
                            }
                        }
                        SecondaryBtn {
                            text: settings.appUiLanguage === "ru"
                                ? "GPU для Ollama (Windows)" : "GPU for Ollama (Windows)"
                            onClicked: setup.openWindowsGpuSettings()
                        }
                        SectionTitle { text: settings.uiText("settings_server_url", settings.appUiLanguage) }
                        FieldInput {
                            id: ollamaField
                            Layout.fillWidth: true
                            text: settings.ollamaBaseUrl
                            placeholderText: "http://127.0.0.1:11434"
                            onEditingFinished: settings.setOllamaBaseUrl(text)
                        }
                        BodyLabel {
                            text: backend.extractRuntimeStatus()
                            font.pixelSize: pal.fontCaption
                            color: pal.muted
                        }
                    }
                    SectionTitle {
                        text: settings.appUiLanguage === "ru" ? "Обновления (GitHub)" : "Updates (GitHub)"
                    }
                    SectionCard {
                        BodyLabel {
                            text: (settings.appUiLanguage === "ru" ? "Установлена версия " : "Installed ")
                                + setup.appVersion
                                + (settings.appUiLanguage === "ru" ? " · обновления с " : " · updates from ")
                                + "github.com/Baddysays/Etemenanki"
                        }
                        RowLayout {
                            Layout.fillWidth: true
                            PrimaryBtn {
                                text: settings.appUiLanguage === "ru" ? "Проверить обновления" : "Check for updates"
                                onClicked: setup.checkForUpdates()
                            }
                            SecondaryBtn {
                                text: settings.appUiLanguage === "ru" ? "Релизы" : "Releases"
                                onClicked: setup.openGitHubReleases()
                            }
                            SecondaryBtn {
                                text: settings.appUiLanguage === "ru" ? "Мастер настройки" : "Setup wizard"
                                onClicked: {
                                    const w = Qt.createComponent("qrc:/Etemenanki/qml_cpp/SetupWizard.qml")
                                    if (w.status === Component.Ready) {
                                        const dlg = w.createObject(win)
                                        if (dlg) {
                                            dlg.show()
                                            dlg.closed.connect(function() { dlg.destroy() })
                                        }
                                    }
                                }
                            }
                        }
                        Rectangle {
                            Layout.fillWidth: true
                            implicitHeight: updateCol.implicitHeight + 16
                            radius: pal.radiusSm
                            color: setup.updateInfo.available ? "#fff7ed" : pal.surface
                            border.color: setup.updateInfo.available ? pal.warn : pal.border
                            ColumnLayout {
                                id: updateCol
                                anchors.fill: parent
                                anchors.margins: 12
                                spacing: 6
                                Label {
                                    Layout.fillWidth: true
                                    wrapMode: Text.WordWrap
                                    color: pal.text
                                    font.pixelSize: pal.fontBody
                                    text: {
                                        const u = setup.updateInfo
                                        if (!u || Object.keys(u).length === 0)
                                            return settings.appUiLanguage === "ru"
                                                ? "Нажмите «Проверить обновления»."
                                                : "Press «Check for updates»."
                                        if (u.available)
                                            return (settings.appUiLanguage === "ru" ? "Доступно " : "Available ")
                                                + u.latest + (u.notes ? ("\n" + u.notes) : "")
                                        if (u.message)
                                            return u.message
                                        return settings.appUiLanguage === "ru" ? "У вас последняя версия." : "You are up to date."
                                    }
                                }
                                RowLayout {
                                    visible: setup.updateInfo.available === true
                                    PrimaryBtn {
                                        text: settings.appUiLanguage === "ru" ? "Скачать установщик" : "Download installer"
                                        onClicked: setup.openUpdateDownload()
                                    }
                                    SecondaryBtn {
                                        text: settings.appUiLanguage === "ru" ? "Портативная ZIP" : "Portable ZIP"
                                        onClicked: {
                                            const u = setup.updateInfo
                                            if (u && u.portable_url)
                                                Qt.openUrlExternally(u.portable_url)
                                        }
                                    }
                                }
                            }
                        }
                    }
                }

                // Languages
                SettingsPage {
                    SectionTitle {
                        text: settings.uiText("settings_langs_pickers", settings.appUiLanguage)
                    }
                    SectionCard {
                        Flow {
                            Layout.fillWidth: true
                            spacing: 4
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
                                delegate: StyledCheck {
                                    required property var modelData
                                    width: 200
                                    text: modelData.flag + "  " + modelData.label
                                    checked: settings.isLanguageEnabled(modelData.code)
                                    onToggled: settings.setLanguageEnabled(modelData.code, checked)
                                }
                            }
                        }
                    }
                }

                // Models
                SettingsPage {
                    SectionCard {
                        Label {
                            text: settings.uiText("local", settings.appUiLanguage)
                            font.weight: Font.DemiBold
                            color: pal.text
                        }
                        BodyLabel {
                            text: settings.localRuntimeAvailable
                                  ? settings.uiText("settings_local_installed", settings.appUiLanguage)
                                  : settings.uiText("no_local_models", settings.appUiLanguage)
                            color: settings.localRuntimeAvailable ? pal.text : pal.warn
                            font.pixelSize: pal.fontCaption
                        }
                        StyledCombo {
                            Layout.fillWidth: true
                            enabled: settings.availableLocalModels.length > 0
                            model: settings.availableLocalModels
                            currentIndex: Math.max(0, settings.availableLocalModels.indexOf(settings.selectedLocalModel))
                            onActivated: settings.setSelectedLocalModel(currentText)
                        }
                        SecondaryBtn {
                            text: settings.uiText("settings_refresh_list", settings.appUiLanguage)
                            onClicked: settings.refreshAvailableModels()
                        }
                    }
                    SectionCard {
                        Label {
                            text: settings.uiText("cloud", settings.appUiLanguage)
                            font.weight: Font.DemiBold
                            color: pal.text
                        }
                        BodyLabel {
                            text: settings.cloudRuntimeAvailable
                                  ? settings.uiText("settings_cloud_configured", settings.appUiLanguage)
                                  : settings.uiText("no_cloud_models", settings.appUiLanguage)
                            color: settings.cloudRuntimeAvailable ? pal.text : pal.warn
                            font.pixelSize: pal.fontCaption
                        }
                        StyledCombo {
                            Layout.fillWidth: true
                            enabled: settings.availableCloudModels.length > 0
                            model: settings.availableCloudModels
                            currentIndex: Math.max(0, settings.availableCloudModels.indexOf(settings.selectedCloudModel))
                            onActivated: settings.setSelectedCloudModel(currentText)
                        }
                    }
                }

                // Cloud API
                SettingsPage {
                    Repeater {
                        model: settings.cloudProviders()
                        delegate: SectionCard {
                            required property var modelData
                            property var cfg: settings.cloudProvider(modelData.id)

                            StyledCheck {
                                Layout.fillWidth: true
                                text: modelData.title
                                checked: cfg.enabled
                            }
                            SectionTitle { text: "Base URL" }
                            FieldInput {
                                id: urlInput
                                Layout.fillWidth: true
                                text: cfg.baseUrl || modelData.defaultUrl
                            }
                            SectionTitle { text: "API Key" }
                            FieldInput {
                                id: keyInput
                                Layout.fillWidth: true
                                text: cfg.apiKey
                                echoMode: TextInput.Password
                                placeholderText: "sk-…"
                            }
                            SectionTitle { text: "Model ID" }
                            FieldInput {
                                id: modelInput
                                Layout.fillWidth: true
                                text: cfg.modelId || modelData.defaultModel
                            }
                            PrimaryBtn {
                                text: settings.uiText("settings_save_provider", settings.appUiLanguage)
                                onClicked: settings.setCloudProvider(
                                    modelData.id,
                                    urlInput.text,
                                    keyInput.text,
                                    modelInput.text,
                                    true)
                            }
                        }
                    }
                }

                // PDF
                SettingsPage {
                    id: pdfPage

                    property var engineCatalog: {
                        const _lang = settings.appUiLanguage
                        return settings.pdfEngineCatalog()
                    }
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

                    BodyLabel {
                        text: settings.uiText("settings_pdf_intro", settings.appUiLanguage)
                        color: pal.muted
                        font.pixelSize: pal.fontCaption
                    }

                    SectionCard {
                        Label {
                            text: settings.uiText("settings_pdf_engine", settings.appUiLanguage)
                            font.weight: Font.DemiBold
                            color: pal.text
                        }
                        ButtonGroup { id: pdfEngineGroup }
                        Repeater {
                            model: pdfPage.engineCatalog
                            delegate: StyledRadio {
                                required property var modelData
                                ButtonGroup.group: pdfEngineGroup
                                Layout.fillWidth: true
                                text: modelData.name
                                checked: settings.pdfEngine === modelData.id
                                onClicked: {
                                    settings.setPdfEngine(modelData.id)
                                    pdfPage.refreshProbe()
                                }
                            }
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            Layout.preferredHeight: engineInfo.implicitHeight + 20
                            radius: pal.radiusSm
                            color: pal.surface
                            border.color: pal.border
                            ColumnLayout {
                                id: engineInfo
                                anchors.fill: parent
                                anchors.margins: 12
                                spacing: 6
                                BodyLabel {
                                    text: pdfPage.selectedEngine.desc || ""
                                    font.pixelSize: pal.fontCaption
                                }
                                BodyLabel {
                                    visible: (pdfPage.selectedEngine.authors || "").length > 0
                                    text: settings.uiText("settings_authors_prefix", settings.appUiLanguage)
                                          + (pdfPage.selectedEngine.authors || "")
                                    color: pal.muted
                                    font.pixelSize: pal.fontCaption
                                }
                                LinkBtn {
                                    visible: (pdfPage.selectedEngine.url || "").length > 0
                                    text: "GitHub ↗"
                                    onClicked: settings.openExternalUrl(pdfPage.selectedEngine.url)
                                }
                            }
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            SecondaryBtn {
                                text: settings.uiText("settings_check", settings.appUiLanguage)
                                onClicked: pdfPage.refreshProbe()
                            }
                            BodyLabel {
                                text: {
                                    const engines = pdfPage.engineProbe.engines
                                    if (!engines)
                                        return settings.uiText("settings_not_checked", settings.appUiLanguage)
                                    const info = engines[settings.pdfEngine]
                                    if (!info) return ""
                                    return (info.available ? "✓ " : "✗ ") + (info.message || "")
                                }
                                color: {
                                    const engines = pdfPage.engineProbe.engines
                                    if (!engines || !engines[settings.pdfEngine]) return pal.muted
                                    return engines[settings.pdfEngine].available ? pal.ok : pal.warn
                                }
                                font.pixelSize: pal.fontCaption
                            }
                        }

                        RowLayout {
                            visible: settings.pdfEngine === "pdfmathtranslate"
                            Layout.fillWidth: true
                            LinkBtn {
                                text: settings.uiText("settings_pdf2zh_releases", settings.appUiLanguage)
                                onClicked: settings.openExternalUrl(
                                    "https://github.com/Byaidu/PDFMathTranslate/releases")
                            }
                            BodyLabel {
                                text: "tools/setup_pdf2zh.ps1"
                                color: pal.muted
                                font.pixelSize: pal.fontCaption
                            }
                        }

                        StyledCheck {
                            visible: settings.pdfEngine === "etemenanki"
                            text: settings.uiText("settings_auto_layout", settings.appUiLanguage)
                            checked: settings.pdfLayoutAuto
                            onToggled: settings.setPdfLayoutAuto(checked)
                        }
                        StyledCheck {
                            visible: settings.pdfEngine === "etemenanki"
                            text: settings.uiText("settings_preserve_tables", settings.appUiLanguage)
                            checked: settings.pdfLayoutPreserveTables
                            onToggled: settings.setPdfLayoutPreserveTables(checked)
                        }
                    }

                    BodyLabel {
                        text: settings.uiText("settings_pdf_license", settings.appUiLanguage)
                        color: pal.muted
                        font.pixelSize: 10
                    }
                }

                // Translation
                SettingsPage {
                    SectionCard {
                        StyledCheck {
                            Layout.fillWidth: true
                            text: settings.uiText("settings_glossary_prompts", settings.appUiLanguage)
                            checked: settings.glossaryEnabled
                            onToggled: settings.setGlossaryEnabled(checked)
                        }
                        SectionTitle {
                            text: settings.uiText("settings_glossary", settings.appUiLanguage)
                        }
                        FieldArea {
                            id: glossaryField
                            Layout.fillWidth: true
                            Layout.preferredHeight: 140
                            text: settings.glossaryText
                            placeholderText: settings.uiText("settings_glossary_placeholder", settings.appUiLanguage)
                            onEditingFinished: settings.setGlossaryText(text)
                        }
                        RowLayout {
                            Layout.fillWidth: true
                            Label {
                                text: settings.uiText("settings_parallel_requests", settings.appUiLanguage)
                                color: pal.muted
                            }
                            StyledSpin {
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
    }

    function persistFields() {
        if (typeof ollamaField !== "undefined")
            settings.setOllamaBaseUrl(ollamaField.text)
        if (typeof glossaryField !== "undefined")
            settings.setGlossaryText(glossaryField.text)
        settings.refreshAvailableModels()
    }

    function persistAndClose() {
        persistFields()
        close()
    }

    onClosing: function() {
        persistFields()
    }

    function open() {
        show()
        raise()
        requestActivate()
    }
}

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import QtQuick.Pdf

ApplicationWindow {
    id: root

    EtePalette { id: pal }
    width: 1460
    height: 900
    minimumWidth: 1180
    minimumHeight: 760
    visible: true
    title: "Etemenanki — " + tr("subtitle")
    color: pal.bg
    font.family: "Segoe UI"
    font.pixelSize: pal.fontBody

    property string runtime: "local"
    property int elapsedSec: 0
    property string resultViewMode: "text" // "format" | "text"

    function tr(key) {
        return settings.uiText(key, settings.appUiLanguage)
    }

    onResultViewModeChanged: {
        if (resultViewMode === "format")
            formatViewBtn.checked = true
        else
            textViewBtn.checked = true
    }

    readonly property color cText: pal.text
    readonly property color cMuted: pal.muted
    readonly property color cBorder: pal.border
    readonly property color cCard: pal.card
    readonly property color cAccent: pal.accent
    readonly property color cOk: pal.ok

    FileDialog {
        id: openDialog
        title: tr("dialog_open_title")
        nameFilters: settings.openFileFilter().split(";;")
        onAccepted: {
            const picked = openDialog.selectedFile
            if (picked && picked.toString().length > 0)
                backend.loadFileFromUrl(picked)
        }
    }

    FileDialog {
        id: saveDialog
        title: tr("dialog_save_title")
        fileMode: FileDialog.SaveFile
        acceptLabel: tr("dialog_save_accept")
        nameFilters: backend.saveFileFilter().split(";;")
        defaultSuffix: backend.isPdf && backend.hasTranslatedPdf ? "pdf" : "txt"
        onAccepted: {
            if (selectedFile)
                backend.saveResult(selectedFile.toString())
        }
    }

    Timer {
        id: elapsedTimer
        interval: 1000
        repeat: true
        running: backend.busy
        onTriggered: elapsedSec += 1
        onRunningChanged: if (running) elapsedSec = 0
    }

    property string srcLangCode: "en"
    property string dstLangCode: "ru"

    function activeModelId() {
        return settings.modelForRuntime(runtime)
    }

    function syncRuntimeFromModels() {
        if (runtime === "local" && !settings.localRuntimeAvailable && settings.cloudRuntimeAvailable)
            runtime = "cloud"
        else if (runtime === "cloud" && !settings.cloudRuntimeAvailable && settings.localRuntimeAvailable)
            runtime = "local"
        if (typeof modelBox !== "undefined")
            modelBox.syncIndex()
        const mid = activeModelId()
        if (mid)
            backend.updateModelInfo(mid)
    }

    function formatTime(sec) {
        const m = Math.floor(sec / 60)
        const s = sec % 60
        return (m < 10 ? "0" : "") + m + ":" + (s < 10 ? "0" : "") + s
    }

    function remainingSec() {
        if (!backend.busy)
            return 0
        return Math.max(0, backend.estimatedRemainingSec)
    }

    function remainingTimeLabel() {
        if (!backend.busy)
            return tr("main_remaining_none")
        const sec = remainingSec()
        if (sec <= 0)
            return tr("main_remaining_unknown")
        return tr("main_remaining") + formatTime(sec)
    }

    function translatedFileName(name) {
        if (!name) return "—"
        const dot = name.lastIndexOf(".")
        const base = dot < 0 ? name : name.substring(0, dot)
        if (backend.isPdf && backend.hasTranslatedPdf)
            return base + "_ru.pdf"
        if (dot < 0)
            return base + "_ru.txt"
        return base + "_ru" + name.substring(dot)
    }

    function sourceFormatLabel() {
        if (backend.isPdf)
            return "PDF"
        const lower = (backend.fileName || "").toLowerCase()
        if (lower.endsWith(".docx"))
            return "DOCX"
        if (lower.endsWith(".xlsx") || lower.endsWith(".csv"))
            return "XLSX/CSV"
        if (lower.endsWith(".srt") || lower.endsWith(".ass") || lower.endsWith(".vtt"))
            return "SRT"
        if (lower.endsWith(".json"))
            return "JSON"
        if (lower.endsWith(".html") || lower.endsWith(".htm"))
            return "HTML"
        if (lower.endsWith(".epub"))
            return "EPUB"
        if (lower.endsWith(".md"))
            return "MD"
        if (lower.endsWith(".txt"))
            return "TXT"
        return tr("main_file_type")
    }

    function resultFormatAvailable() {
        if (backend.isPdf)
            return backend.hasTranslatedPdf
        return backend.contentIsStructured || backend.documentFormatted
    }

    function updateResultPanelView() {
        if (resultViewMode === "format" && backend.isPdf && backend.hasTranslatedPdf) {
            resultPanel.goToPdfPage(sourcePanel.currentPage - 1)
            resultPanel.fitPdfToView()
            return
        }
        refreshResultView()
    }

    component Card: Rectangle {
        color: cCard
        radius: 14
        border.color: cBorder
        border.width: 1
    }

    component OutlineButton: Button {
        id: btn
        implicitHeight: 40
        padding: 0
        leftPadding: btn.iconText !== "" ? 12 : 16
        rightPadding: 16
        property string iconText: ""
        background: Rectangle {
            radius: 10
            color: btn.down ? "#eef2f8" : (btn.hovered ? "#f7f9fd" : "#ffffff")
            border.color: btn.enabled ? "#cfd9eb" : "#e2e8f2"
        }
        contentItem: Label {
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            text: btn.iconText !== "" ? (btn.iconText + "  " + btn.text) : btn.text
            color: btn.enabled ? cText : "#8c97ac"
            font.weight: Font.DemiBold
            font.pixelSize: 13
        }
    }

    component PrimaryButton: Button {
        id: btn
        implicitHeight: 40
        implicitWidth: 156
        padding: 0
        leftPadding: 18
        rightPadding: 18
        background: Rectangle {
            radius: 10
            color: btn.down ? "#1d4ed8" : (btn.hovered ? pal.accentHover : cAccent)
        }
        contentItem: Label {
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            text: "✦  " + btn.text
            color: "#ffffff"
            font.weight: Font.DemiBold
            font.pixelSize: 14
        }
    }

    component SegmentBtn: Button {
        id: btn
        checkable: true
        implicitHeight: 30
        implicitWidth: 86
        padding: 0
        background: Rectangle {
            radius: 8
            color: btn.checked ? "#e6efff" : "transparent"
            border.color: btn.checked ? "#8fb0e5" : "transparent"
            border.width: 1
        }
        contentItem: Label {
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            text: btn.text
            color: btn.checked ? "#224d93" : "#486180"
            font.weight: Font.DemiBold
            font.pixelSize: 12
        }
    }

    component IconBtn: Button {
        id: btn
        implicitWidth: 36
        implicitHeight: 36
        property string iconGlyph: ""
        background: Rectangle {
            radius: 10
            color: btn.hovered ? "#eef2f8" : "transparent"
            border.color: "#e3eaf5"
            border.width: btn.hovered ? 1 : 0
        }
        contentItem: Item {
            anchors.fill: parent
            Text {
                anchors.centerIn: parent
                text: btn.iconGlyph
                font.pixelSize: 16
                color: cMuted
            }
        }
    }

    component StyledCombo: ComboBox {
        id: combo
        implicitHeight: 38
        property bool withFlags: true
        textRole: withFlags ? "label" : ""
        valueRole: withFlags ? "code" : ""

        function rowAt(index) {
            if (index < 0 || !combo.model)
                return ({ flag: "", label: "", code: "" })
            if (typeof combo.model.get === "function")
                return combo.model.get(index)
            const item = combo.model[index]
            if (typeof item === "string")
                return ({ flag: "", label: item, code: item })
            return item
        }

        background: Rectangle {
            radius: 10
            color: "#fbfcfe"
            border.color: combo.activeFocus ? "#4f86eb" : "#cfd9eb"
        }
        delegate: ItemDelegate {
            width: combo.width
            contentItem: Text {
                leftPadding: 10
                text: combo.withFlags
                      ? ((model.flag !== undefined ? model.flag : "") + "  "
                         + (model.label !== undefined ? model.label : ""))
                      : (typeof modelData === "string" ? modelData : String(modelData))
                color: cText
                verticalAlignment: Text.AlignVCenter
                elide: Text.ElideRight
            }
            background: Rectangle { color: highlighted ? "#eef4ff" : "transparent" }
        }
        contentItem: Text {
            leftPadding: 10
            text: {
                if (combo.currentIndex < 0)
                    return ""
                if (!combo.withFlags)
                    return combo.displayText
                const row = combo.rowAt(combo.currentIndex)
                return row.flag ? row.flag + "  " + row.label : row.label
            }
            color: cText
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
        }
    }

    function refreshSourceView() {
        if (sourcePanel.usePdfPreview)
            return
        const pageText = backend.sourcePageText(sourcePanel.currentPage)
        const nextText = pageText.length > 0 ? pageText : backend.sourceText
        if (sourcePanel.textArea.text !== nextText)
            sourcePanel.textArea.text = nextText
    }

    function refreshResultView() {
        if (resultViewMode === "format" && backend.isPdf && backend.hasTranslatedPdf)
            return
        if (resultViewMode === "format" && backend.isPdf && !backend.hasTranslatedPdf) {
            if (resultPanel.textArea.text !== "")
                resultPanel.textArea.text = ""
            if (resultPanel.richHtmlContent !== "")
                resultPanel.richHtmlContent = ""
            return
        }
        if (resultViewMode === "format" && backend.contentIsStructured) {
            const html = backend.translatedPageHtml(resultPanel.currentPage)
            const nextHtml = html.length > 0 ? html : ""
            if (resultPanel.richHtmlContent !== nextHtml)
                resultPanel.richHtmlContent = nextHtml
            return
        }
        if (resultPanel.richHtmlContent !== "")
            resultPanel.richHtmlContent = ""
        const page = backend.translatedPageText(resultPanel.currentPage)
        const nextText = page.length > 0 ? page
            : (resultPanel.currentPage === 1 ? backend.translatedText : "")
        if (resultPanel.textArea.text !== nextText)
            resultPanel.textArea.text = nextText
    }

    component DocPanel: Card {
        id: docPanel
        property string panelTitle
        property string fileLabel
        property int currentPage: 1
        property int documentPageCount: 1
        property bool usePdfPreview: false
        property url pdfSource
        property var pageTextFn: null
        property bool syncPageCountToBackend: false
        property bool formattedText: false
        property bool richHtml: false
        property string richHtmlContent: ""
        property alias textArea: editor
        property alias pageList: pages

        readonly property int effectivePageCount: usePdfPreview && pdfDoc.pageCount > 0
            ? pdfDoc.pageCount
            : Math.max(1, documentPageCount)

        function fitPdfToView() {
            if (!usePdfPreview || pdfView.width <= 0 || pdfDoc.pageCount <= 0)
                return
            pdfView.scaleToWidth(pdfView.width, pdfView.height)
        }

        function goToPdfPage(zeroBasedIndex) {
            if (!usePdfPreview || pdfDoc.pageCount <= 0)
                return
            const page = Math.max(0, Math.min(zeroBasedIndex, pdfDoc.pageCount - 1))
            pdfView.goToPage(page)
            currentPage = page + 1
        }

        onUsePdfPreviewChanged: {
            if (usePdfPreview)
                Qt.callLater(function() { goToPdfPage(currentPage - 1); fitPdfToView() })
        }

        onPdfSourceChanged: {
            if (pdfSource.toString().length > 0)
                Qt.callLater(function() { goToPdfPage(0); fitPdfToView() })
        }

        PdfDocument {
            id: pdfDoc
            source: docPanel.pdfSource
            onStatusChanged: function(docStatus) {
                if (docStatus === PdfDocument.Ready && pageCount > 0) {
                    if (docPanel.syncPageCountToBackend)
                        backend.ensurePageCount(pageCount)
                    Qt.callLater(function() { docPanel.goToPdfPage(0); docPanel.fitPdfToView() })
                }
            }
        }

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 10
            spacing: 8

            RowLayout {
                Layout.fillWidth: true
                Label {
                    text: panelTitle
                    font.pixelSize: 14
                    font.weight: Font.DemiBold
                    color: cText
                }
                Item { Layout.fillWidth: true }
                Label {
                    text: fileLabel
                    color: cMuted
                    font.pixelSize: 12
                }
                Label {
                    text: currentPage + " / " + effectivePageCount + " " + tr("main_pages_unit")
                    color: cMuted
                    font.pixelSize: 12
                }
            }

            RowLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: 8

                ListView {
                    id: pages
                    Layout.preferredWidth: 118
                    Layout.fillHeight: true
                    clip: true
                    spacing: 6
                    model: Math.max(1, Math.min(effectivePageCount, 120))
                    delegate: Rectangle {
                        id: thumbCard
                        width: 106
                        height: docPanel.usePdfPreview ? 136 : 72
                        radius: 8
                        color: index + 1 === docPanel.currentPage ? "#e8f1ff" : "#f7f9fd"
                        border.color: index + 1 === docPanel.currentPage ? "#7da7ef" : "#d5e0f2"
                        border.width: index + 1 === docPanel.currentPage ? 2 : 1
                        clip: true

                        PdfPageImage {
                            anchors.fill: parent
                            anchors.margins: 3
                            visible: docPanel.usePdfPreview && pdfDoc.status === PdfDocument.Ready
                            document: pdfDoc
                            currentFrame: index
                            asynchronous: true
                            fillMode: Image.PreserveAspectFit
                            sourceSize.width: 96 * Screen.devicePixelRatio
                            sourceSize.height: 0
                        }

                        Text {
                            anchors.fill: parent
                            anchors.margins: 6
                            visible: !docPanel.usePdfPreview && docPanel.pageTextFn
                            text: {
                                if (!docPanel.pageTextFn)
                                    return ""
                                const t = docPanel.pageTextFn(index + 1)
                                return t.length > 120 ? t.slice(0, 120) + "…" : t
                            }
                            color: "#4a5a72"
                            font.pixelSize: 9
                            wrapMode: Text.WordWrap
                            elide: Text.ElideRight
                            verticalAlignment: Text.AlignTop
                        }

                        Rectangle {
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.bottom: parent.bottom
                            height: 20
                            color: "#000000"
                            opacity: 0.45
                        }
                        Label {
                            anchors.bottom: parent.bottom
                            anchors.horizontalCenter: parent.horizontalCenter
                            bottomPadding: 2
                            text: index + 1
                            color: "#ffffff"
                            font.pixelSize: 11
                            font.weight: Font.DemiBold
                        }

                        MouseArea {
                            anchors.fill: parent
                            onClicked: {
                                if (docPanel.usePdfPreview)
                                    docPanel.goToPdfPage(index)
                                else
                                    docPanel.currentPage = index + 1
                            }
                        }
                    }
                }

                Item {
                    Layout.fillWidth: true
                    Layout.fillHeight: true

                    PdfScrollablePageView {
                        id: pdfView
                        visible: docPanel.usePdfPreview
                        anchors.fill: parent
                        clip: true
                        document: pdfDoc
                        onCurrentPageChanged: {
                            if (currentPage >= 0)
                                docPanel.currentPage = currentPage + 1
                        }
                        onWidthChanged: docPanel.fitPdfToView()
                        onHeightChanged: docPanel.fitPdfToView()
                    }

                    ScrollView {
                        id: textScroll
                        anchors.fill: parent
                        visible: !docPanel.usePdfPreview
                        contentWidth: availableWidth
                        clip: true

                        Text {
                            id: richViewer
                            visible: docPanel.richHtml
                            width: textScroll.availableWidth
                            textFormat: Text.RichText
                            text: docPanel.richHtmlContent
                            wrapMode: Text.Wrap
                            font.family: "Segoe UI"
                            font.pixelSize: 12
                            color: cText
                        }

                        TextArea {
                            id: editor
                            visible: !docPanel.richHtml
                            width: textScroll.availableWidth
                            wrapMode: TextArea.Wrap
                            textFormat: docPanel.formattedText ? TextEdit.MarkdownText : TextEdit.PlainText
                            font.family: "Segoe UI"
                            font.pixelSize: 12
                            color: cText
                            selectedTextColor: "#ffffff"
                            selectionColor: cAccent
                            background: Rectangle {
                                radius: 10
                                color: "#fbfcfe"
                                border.color: "#cfd9eb"
                            }
                        }
                    }
                }
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 14
        spacing: 8

        // Header
        Card {
            Layout.fillWidth: true
            Layout.preferredHeight: 92
            RowLayout {
                anchors.fill: parent
                anchors.topMargin: 10
                anchors.bottomMargin: 10
                anchors.leftMargin: 4
                anchors.rightMargin: 10
                spacing: 10

                Image {
                    visible: brandLogoHeader.toString().length > 0
                    source: brandLogoHeader
                    Layout.preferredHeight: 72
                    Layout.preferredWidth: 400
                    Layout.maximumWidth: 460
                    Layout.leftMargin: -2
                    fillMode: Image.PreserveAspectFit
                    smooth: true
                    mipmap: true
                    ToolTip.visible: brandLogoMa.containsMouse
                    ToolTip.text: tr("main_brand_tooltip")
                    MouseArea {
                        id: brandLogoMa
                        anchors.fill: parent
                        hoverEnabled: true
                        acceptedButtons: Qt.NoButton
                    }
                }

                ColumnLayout {
                    visible: brandLogoHeader.toString().length === 0
                    spacing: 2
                    Label {
                        text: "Etemenanki"
                        font.pixelSize: 24
                        font.weight: Font.Bold
                        color: cText
                    }
                    Label {
                        text: tr("main_tagline")
                        font.pixelSize: 12
                        color: cAccent
                    }
                }

                Rectangle {
                    Layout.preferredWidth: 196
                    Layout.preferredHeight: 36
                    radius: 10
                    color: "#f0f4fb"
                    border.color: "#cad7ec"
                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: 3
                        spacing: 4
                        SegmentBtn {
                            text: tr("local")
                            enabled: settings.localRuntimeAvailable
                            checked: runtime === "local"
                            onClicked: {
                                runtime = "local"
                                syncRuntimeFromModels()
                            }
                        }
                        SegmentBtn {
                            text: tr("cloud")
                            enabled: settings.cloudRuntimeAvailable
                            checked: runtime === "cloud"
                            onClicked: {
                                runtime = "cloud"
                                syncRuntimeFromModels()
                            }
                        }
                    }
                }

                ComboBox {
                    id: modelBox
                    implicitHeight: 36
                    Layout.preferredHeight: 36
                    Layout.minimumHeight: 36
                    Layout.fillWidth: true
                    Layout.minimumWidth: 220
                    Layout.preferredWidth: 280
                    enabled: (runtime === "local" && settings.localRuntimeAvailable)
                             || (runtime === "cloud" && settings.cloudRuntimeAvailable)
                    model: runtime === "local"
                           ? settings.availableLocalModels
                           : settings.availableCloudModels

                    function syncIndex() {
                        const id = root.activeModelId()
                        const idx = model.indexOf(id)
                        currentIndex = idx >= 0 ? idx : 0
                    }

                    onModelChanged: syncIndex()
                    Component.onCompleted: syncIndex()
                    onActivated: backend.updateModelInfo(currentText)
                    background: Rectangle {
                        radius: 10
                        color: "#fbfcfe"
                        border.color: modelBox.activeFocus ? "#4f86eb" : "#cfd9eb"
                    }
                    contentItem: Label {
                        leftPadding: 12
                        rightPadding: modelBox.indicator.width + modelBox.spacing + 8
                        text: modelBox.displayText
                        font.pixelSize: 13
                        color: cText
                        verticalAlignment: Text.AlignVCenter
                        elide: Text.ElideRight
                    }
                    delegate: ItemDelegate {
                        width: modelBox.width
                        contentItem: Label {
                            leftPadding: 12
                            text: modelData
                            font.pixelSize: 13
                            color: cText
                            verticalAlignment: Text.AlignVCenter
                            elide: Text.ElideRight
                        }
                        background: Rectangle { color: highlighted ? "#eef4ff" : "transparent" }
                    }
                }

                RowLayout {
                    spacing: 10
                    Rectangle {
                        Layout.preferredHeight: 34
                        implicitWidth: hwRow.implicitWidth + 20
                        radius: 10
                        color: backend.hwCompatible ? "#eef8ef" : "#fff5eb"
                        border.color: backend.hwCompatible ? "#bfe0c4" : "#f0d4b8"
                        RowLayout {
                            id: hwRow
                            anchors.centerIn: parent
                            spacing: 8
                            Label { text: backend.hwCompatible ? "✓" : "!"; color: backend.hwCompatible ? cOk : "#c77b1d"; font.weight: Font.Bold }
                            ColumnLayout {
                                spacing: 0
                                Label { text: backend.ramLabel; color: "#295f31"; font.pixelSize: 11; font.weight: Font.DemiBold }
                                Label { text: backend.vramLabel; color: "#295f31"; font.pixelSize: 11; font.weight: Font.DemiBold }
                            }
                        }
                    }
                    IconBtn {
                        id: settingsBtn
                        iconGlyph: "⚙"
                        ToolTip.visible: hovered
                        ToolTip.text: tr("main_settings_tooltip")
                        onClicked: appSettingsWindow.open()
                    }
                    IconBtn {
                        iconGlyph: "?"
                        ToolTip.visible: hovered
                        ToolTip.text: tr("help")
                        onClicked: helpWindow.open()
                    }
                }
            }
        }

        // Language + actions
        Card {
            Layout.fillWidth: true
            Layout.preferredHeight: 64
            RowLayout {
                anchors.fill: parent
                anchors.margins: 10
                spacing: 10

                Label { text: tr("source_lang"); color: cMuted; font.pixelSize: 12 }
                LangPicker {
                    id: srcLang
                    Layout.preferredWidth: 196
                    entries: settings.enabledLanguages(true)
                    currentCode: srcLangCode
                    onCodeSelected: function(code) { srcLangCode = code }
                }

                Button {
                    implicitWidth: 34
                    implicitHeight: 34
                    text: "⇄"
                    enabled: srcLangCode !== "auto"
                    background: Rectangle {
                        radius: 17
                        color: parent.hovered ? "#eef4ff" : "#ffffff"
                        border.color: "#cfd9eb"
                    }
                    contentItem: Text {
                        text: parent.text
                        color: cText
                        font.pixelSize: 16
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                        anchors.fill: parent
                    }
                    onClicked: {
                        if (srcLangCode === "auto") return
                        const t = srcLangCode
                        srcLangCode = dstLangCode
                        dstLangCode = t
                    }
                }

                Label { text: tr("target_lang"); color: cMuted; font.pixelSize: 12 }
                LangPicker {
                    id: dstLang
                    Layout.preferredWidth: 196
                    entries: settings.enabledLanguages(false)
                    currentCode: dstLangCode
                    onCodeSelected: function(code) { dstLangCode = code }
                }

                Item { Layout.fillWidth: true }

                OutlineButton {
                    text: tr("load_file")
                    iconText: "↑"
                    enabled: !backend.busy
                    onClicked: openDialog.open()
                }
                PrimaryButton {
                    text: tr("translate")
                    enabled: !backend.busy
                             && ((runtime === "local" && settings.localRuntimeAvailable)
                                 || (runtime === "cloud" && settings.cloudRuntimeAvailable))
                    onClicked: backend.startTranslate(
                        runtime,
                        activeModelId(),
                        srcLangCode,
                        dstLangCode)
                }
                OutlineButton {
                    text: tr("save")
                    iconText: "💾"
                    enabled: !backend.busy
                             && (backend.translatedText.length > 0
                                 || backend.hasTranslatedPdf)
                    onClicked: {
                        saveDialog.currentFile = backend.suggestedSaveUrl()
                        saveDialog.open()
                    }
                }
            }
        }

        // Document panels
        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 8

            DocPanel {
                id: sourcePanel
                Layout.fillWidth: true
                Layout.fillHeight: true
                panelTitle: tr("main_source_panel")
                fileLabel: backend.fileName || tr("main_no_file")
                documentPageCount: backend.pageCount || 1
                usePdfPreview: backend.isPdf && backend.pdfPreviewUrl.toString().length > 0
                pdfSource: backend.pdfPreviewUrl
                pageTextFn: function(page) { return backend.sourcePageText(page) }
                syncPageCountToBackend: true
                formattedText: backend.documentFormatted
                onCurrentPageChanged: {
                    refreshSourceView()
                    if (resultViewMode === "format" && backend.isPdf && backend.hasTranslatedPdf)
                        resultPanel.goToPdfPage(currentPage - 1)
                }
                textArea.readOnly: backend.fileName.length > 0
                Connections {
                    target: sourcePanel.textArea
                    enabled: backend.fileName.length === 0
                    function onTextChanged() {
                        backend.setSourceText(sourcePanel.textArea.text)
                    }
                }
                textArea.placeholderText: usePdfPreview
                    ? ""
                    : (backend.fileName.length > 0 ? "" : tr("main_source_placeholder"))
            }

            DocPanel {
                id: resultPanel
                Layout.fillWidth: true
                Layout.fillHeight: true
                panelTitle: tr("main_result_panel")
                fileLabel: translatedFileName(backend.fileName)
                documentPageCount: backend.pageCount || 1
                usePdfPreview: resultViewMode === "format" && backend.isPdf && backend.hasTranslatedPdf
                pdfSource: backend.translatedPdfUrl
                pageTextFn: function(page) { return backend.translatedPageText(page) }
                richHtml: resultViewMode === "format" && !backend.isPdf && backend.contentIsStructured
                formattedText: resultViewMode === "format" && !backend.isPdf && backend.documentFormatted
                    && !backend.contentIsStructured
                onCurrentPageChanged: {
                    if (resultViewMode === "format" && backend.isPdf && backend.hasTranslatedPdf)
                        resultPanel.goToPdfPage(currentPage - 1)
                    else
                        refreshResultView()
                }
                textArea.readOnly: true
                textArea.placeholderText: {
                    if (resultViewMode === "format" && backend.isPdf && !backend.hasTranslatedPdf) {
                        if (backend.busy)
                            return tr("main_pdf_building")
                        return tr("main_pdf_failed")
                    }
                    if (resultViewMode === "format" && !resultFormatAvailable())
                        return tr("main_format_pending")
                    return tr("main_result_placeholder")
                }
            }
        }

        Connections {
            target: backend
            function onSourceTextChanged() { Qt.callLater(refreshSourceView) }
            function onFileNameChanged() {
                resultViewMode = "text"
                sourcePanel.currentPage = 1
                resultPanel.currentPage = 1
                Qt.callLater(function() {
                    refreshSourceView()
                    refreshResultView()
                })
            }
            function onFilePathChanged() {
                Qt.callLater(function() {
                    refreshSourceView()
                    if (sourcePanel.usePdfPreview) {
                        sourcePanel.goToPdfPage(0)
                        sourcePanel.fitPdfToView()
                    }
                })
            }
            function onPageCountChanged() { Qt.callLater(refreshSourceView) }
            function onTranslatedTextChanged() { Qt.callLater(refreshResultView) }
            function onContentIsStructuredChanged() { Qt.callLater(refreshResultView) }
            function onTranslatedPdfPathChanged() {
                if (backend.hasTranslatedPdf) {
                    resultViewMode = "format"
                    Qt.callLater(updateResultPanelView)
                } else {
                    Qt.callLater(refreshResultView)
                }
            }
        }

        // Model stats + result view mode
        Card {
            Layout.fillWidth: true
            Layout.preferredHeight: 58
            RowLayout {
                anchors.fill: parent
                anchors.margins: 10
                spacing: 12

                ButtonGroup { id: resultViewGroup }

                RowLayout {
                    spacing: 6
                    SegmentBtn {
                        id: formatViewBtn
                        ButtonGroup.group: resultViewGroup
                        implicitWidth: 168
                        text: tr("main_format_tab").arg(sourceFormatLabel())
                        enabled: backend.translatedText.length > 0
                            || backend.translatedPageText(1).length > 0
                        onClicked: {
                            resultViewMode = "format"
                            if (backend.isPdf && !backend.hasTranslatedPdf && !backend.busy)
                                backend.retryTranslatedPdfBuild()
                            updateResultPanelView()
                        }
                    }
                    SegmentBtn {
                        id: textViewBtn
                        ButtonGroup.group: resultViewGroup
                        implicitWidth: 168
                        text: tr("main_text_tab")
                        checked: true
                        enabled: backend.translatedText.length > 0
                            || backend.translatedPageText(1).length > 0
                        onClicked: {
                            resultViewMode = "text"
                            refreshResultView()
                        }
                    }
                }

                Item { Layout.fillWidth: true }

                RowLayout {
                    spacing: 28
                    Repeater {
                        model: [
                            { icon: "⏱", titleKey: "main_stat_speed", value: backend.modelSpeed },
                            { icon: "🏅", titleKey: "main_stat_quality", value: backend.modelQuality },
                            { icon: "🧠", titleKey: "", fixedTitle: "RAM", value: backend.modelRamNeed },
                            { icon: "🎮", titleKey: "", fixedTitle: "VRAM", value: backend.modelVramNeed }
                        ]
                        delegate: RowLayout {
                            spacing: 8
                            required property var modelData
                            Label { text: modelData.icon; font.pixelSize: 16 }
                            ColumnLayout {
                                spacing: 0
                                Label {
                                    text: modelData.fixedTitle || tr(modelData.titleKey)
                                    color: cMuted
                                    font.pixelSize: 11
                                }
                                Label {
                                    text: modelData.value
                                    color: cText
                                    font.weight: Font.DemiBold
                                }
                            }
                        }
                    }
                }
            }
        }

        // Status bar
        Card {
            Layout.fillWidth: true
            Layout.preferredHeight: 54
            RowLayout {
                anchors.fill: parent
                anchors.margins: 10
                spacing: 10

                Label { text: backend.fileName ? "✓" : "○"; color: backend.fileName ? cOk : cMuted }

                Label {
                    text: backend.status
                    color: cText
                    font.weight: Font.DemiBold
                    elide: Text.ElideRight
                    Layout.fillWidth: true
                    ToolTip.visible: statusMa.containsMouse && text.length > 0
                    ToolTip.text: backend.status
                    ToolTip.delay: 400

                    MouseArea {
                        id: statusMa
                        anchors.fill: parent
                        hoverEnabled: true
                        acceptedButtons: Qt.NoButton
                    }
                }

                Label {
                    visible: backend.busy
                    text: tr("main_elapsed") + " " + formatTime(elapsedSec)
                    color: cMuted
                    font.pixelSize: 12
                }
                Label {
                    visible: backend.busy
                    text: remainingTimeLabel()
                    color: cMuted
                    font.pixelSize: 12
                }

                Item {
                    Layout.preferredWidth: 120
                    Layout.preferredHeight: 18
                    Rectangle {
                        anchors.fill: parent
                        radius: 9
                        color: "#edf2fa"
                        border.color: "#d0daec"
                    }
                    Rectangle {
                        anchors.left: parent.left
                        anchors.top: parent.top
                        anchors.bottom: parent.bottom
                        width: parent.width * (backend.progress / 100.0)
                        radius: 9
                        color: cAccent
                    }
                    Label {
                        anchors.centerIn: parent
                        text: backend.busy ? (backend.progress + "%") : (backend.progress >= 100 ? tr("main_done") : "")
                        color: "#52627d"
                        font.weight: Font.DemiBold
                        font.pixelSize: 11
                    }
                }

                OutlineButton {
                    text: tr("cancel")
                    iconText: "✕"
                    enabled: backend.busy
                    onClicked: backend.cancelTranslate()
                }
            }
        }
    }

    SettingsWindow {
        id: appSettingsWindow
    }

    HelpWindow {
        id: helpWindow
    }

    SetupWizard {
        id: setupWizard
    }

    Connections {
        target: settings
        function onModelsChanged() { root.syncRuntimeFromModels() }
    }

    Component.onCompleted: {
        syncRuntimeFromModels()
        if (!setup.setupComplete)
            setupWizard.show()
    }
}

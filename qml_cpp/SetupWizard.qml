import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Window {
    id: wizard

    EtePalette { id: pal }

    width: 640
    height: 480
    minimumWidth: 560
    minimumHeight: 400
    title: "Etemenanki — " + trRu("Setup", "Настройка")
    color: pal.bg
    modality: Qt.ApplicationModal
    flags: Qt.Dialog | Qt.WindowTitleHint | Qt.WindowCloseButtonHint

    property bool embeddedReady: false
    property bool pythonReady: false
    property bool pdf2zhReady: false

    function trRu(en, ru) {
        return settings.appUiLanguage === "ru" ? ru : en
    }

    function checkStatus() {
        const d = setup.depsStatus
        pythonReady = d && d.python === true && d.python_libs === true
        pdf2zhReady = d && d.pdf2zh === true
        embeddedReady = d && d.embedded === true

        if (!setup.probeReady && !setup.busy)
            setup.probeHardware()
    }

    onVisibleChanged: {
        if (visible)
            checkStatus()
    }

    Connections {
        target: setup
        function onProbeFinished(ok) { if (ok) wizard.checkStatus() }
        function onHardwareChanged() { wizard.checkStatus() }
        function onDepsStatusChanged() { wizard.checkStatus() }
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
        implicitHeight: 44
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

    component StatusRow : RowLayout {
        property string label: ""
        property bool ok: false
        property string detail: ""
        spacing: 10
        Rectangle {
            width: 24
            height: 24
            radius: 12
            color: ok ? pal.ok : pal.warn
            Text {
                anchors.centerIn: parent
                text: ok ? "✓" : "!"
                color: "#ffffff"
                font.pixelSize: 14
                font.weight: Font.Bold
            }
        }
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 2
            Label {
                text: label
                color: pal.text
                font.weight: Font.DemiBold
                font.pixelSize: pal.fontBody
            }
            Label {
                visible: detail.length > 0
                text: detail
                color: pal.muted
                font.pixelSize: pal.fontCaption
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
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
                    text: trRu("AI document translator — ready to use.",
                               "ИИ-переводчик документов — готов к работе.")
                    wrapMode: Text.WordWrap
                    Layout.fillWidth: true
                    color: pal.muted
                    font.pixelSize: pal.fontBody
                }
            }
        }

        WizardCard {
            Layout.fillHeight: true
            Label {
                text: trRu("Status", "Статус")
                font.weight: Font.DemiBold
                color: pal.text
                font.pixelSize: 16
            }

            StatusRow {
                Layout.fillWidth: true
                label: trRu("Built-in AI model", "Встроенная модель ИИ")
                ok: wizard.embeddedReady
                detail: wizard.embeddedReady
                    ? trRu("Ready — translate documents without internet",
                           "Готова — переводите документы без интернета")
                    : trRu("Not found — use Ollama or cloud API in Settings",
                           "Не найдена — используйте Ollama или облако в Настройках")
            }

            StatusRow {
                Layout.fillWidth: true
                label: "Python + " + trRu("document libraries", "библиотеки документов")
                ok: wizard.pythonReady
                detail: wizard.pythonReady
                    ? trRu("PyMuPDF, python-docx — OK", "PyMuPDF, python-docx — OK")
                    : trRu("Not found — PDF/DOCX extraction unavailable",
                           "Не найдены — извлечение PDF/DOCX недоступно")
            }

            StatusRow {
                Layout.fillWidth: true
                label: "pdf2zh"
                ok: wizard.pdf2zhReady
                detail: wizard.pdf2zhReady
                    ? trRu("Layout-preserving PDF translation — OK",
                           "Перевод PDF с сохранением вёрстки — OK")
                    : trRu("Optional — install from Settings if needed",
                           "Опционально — установите в Настройках при необходимости")
            }

            Item { Layout.fillWidth: true }

            Label {
                visible: setup.busy
                text: setup.statusText || trRu("Scanning…", "Сканирование…")
                color: pal.accentText
                font.pixelSize: pal.fontCaption
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }
            ProgressBar {
                Layout.fillWidth: true
                indeterminate: true
                visible: setup.busy
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 8
            GhostBtn {
                text: trRu("GitHub", "GitHub")
                onClicked: setup.openGitHubRepo()
            }
            GhostBtn {
                text: trRu("Settings", "Настройки")
                onClicked: {
                    wizard.applyAndClose()
                    settingsWindow.show()
                }
            }
            Item { Layout.fillWidth: true }
            Label {
                text: "v" + setup.appVersion
                color: pal.muted
                font.pixelSize: pal.fontCaption
            }
            PrimaryBtn {
                text: trRu("Start", "Начать")
                onClicked: wizard.applyAndClose()
            }
        }
    }

    function applyAndClose() {
        if (wizard.embeddedReady)
            settings.setLocalAiMode("embedded")
        setup.markSetupComplete()
        wizard.close()
    }
}

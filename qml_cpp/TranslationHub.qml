import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: hub
    property color cText: "#1f2b3f"
    property color cMuted: "#6a7892"
    property color cBorder: "#d9e1ef"
    property color cAccent: "#2f78ee"

    radius: 14
    color: "#ffffff"
    border.color: cBorder
    border.width: 1
    implicitHeight: content.implicitHeight + 20

    ColumnLayout {
        id: content
        anchors.fill: parent
        anchors.margins: 10
        spacing: 6

        RowLayout {
            Layout.fillWidth: true
            spacing: 10
            Label {
                text: settings.uiText("hub_title", settings.appUiLanguage)
                font.pixelSize: 14
                font.weight: Font.DemiBold
                color: cText
            }
            Item { Layout.fillWidth: true }
            Label {
                visible: backend.fileName.length > 0
                text: backend.workflowTitle.length > 0 ? backend.workflowTitle : "—"
                font.pixelSize: 12
                font.weight: Font.DemiBold
                color: cAccent
            }
        }

        Label {
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            text: settings.uiText("hub_formats", settings.appUiLanguage)
            color: cMuted
            font.pixelSize: 11
        }

        GridLayout {
            Layout.fillWidth: true
            columns: 2
            columnSpacing: 12
            rowSpacing: 4
            visible: backend.fileName.length > 0

            Label { text: settings.uiText("hub_pipeline", settings.appUiLanguage); color: cMuted; font.pixelSize: 11 }
            Label {
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
                text: backend.workflowPipeline.length > 0 ? backend.workflowPipeline : "—"
                color: cText
                font.pixelSize: 11
            }

            Label { text: settings.uiText("hub_export", settings.appUiLanguage); color: cMuted; font.pixelSize: 11 }
            Label {
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
                text: backend.workflowExport.length > 0 ? backend.workflowExport : "TXT"
                color: cText
                font.pixelSize: 11
            }
        }
    }
}

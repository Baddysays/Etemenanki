import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: hub

    EtePalette { id: pal }

    radius: 14
    color: pal.card
    border.color: pal.border
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
                color: pal.text
            }
            Item { Layout.fillWidth: true }
            Label {
                visible: backend.fileName.length > 0
                text: backend.workflowTitle.length > 0 ? backend.workflowTitle : "—"
                font.pixelSize: pal.fontCaption
                font.weight: Font.DemiBold
                color: pal.accent
            }
        }

        Label {
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            text: settings.uiText("hub_formats", settings.appUiLanguage)
            color: pal.muted
            font.pixelSize: pal.fontCaption
        }

        GridLayout {
            Layout.fillWidth: true
            columns: 2
            columnSpacing: 12
            rowSpacing: 4
            visible: backend.fileName.length > 0

            Label { text: settings.uiText("hub_pipeline", settings.appUiLanguage); color: pal.muted; font.pixelSize: pal.fontCaption }
            Label {
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
                text: backend.workflowPipeline.length > 0 ? backend.workflowPipeline : "—"
                color: pal.text
                font.pixelSize: pal.fontCaption
            }

            Label { text: settings.uiText("hub_export", settings.appUiLanguage); color: pal.muted; font.pixelSize: pal.fontCaption }
            Label {
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
                text: backend.workflowExport.length > 0 ? backend.workflowExport : "TXT"
                color: pal.text
                font.pixelSize: pal.fontCaption
            }
        }
    }
}

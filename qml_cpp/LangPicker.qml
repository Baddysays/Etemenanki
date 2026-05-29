import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root

    EtePalette { id: pal }
    property var entries: []
    property string currentCode: "en"
    signal codeSelected(string code)

    implicitHeight: 38
    implicitWidth: 196
    radius: pal.radiusSm
    color: pal.card
    border.color: popup.opened ? pal.accent : pal.border
    border.width: popup.opened ? 2 : 1

    function entryForCode(code) {
        for (let i = 0; i < entries.length; ++i) {
            if (entries[i].code === code)
                return entries[i]
        }
        return { code: code, label: code, flag: "🌐" }
    }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 10
        anchors.rightMargin: 10
        spacing: 8
        Text {
            text: entryForCode(currentCode).flag
            font.pixelSize: 14
        }
        Text {
            Layout.fillWidth: true
            text: entryForCode(currentCode).label
            color: pal.text
            font.pixelSize: 13
            elide: Text.ElideRight
        }
        Text {
            text: "▾"
            color: pal.muted
            font.pixelSize: 11
        }
    }

    MouseArea {
        anchors.fill: parent
        cursorShape: Qt.PointingHandCursor
        onClicked: popup.open()
    }

    Popup {
        id: popup
        y: root.height + 4
        width: Math.max(root.width, 240)
        implicitHeight: Math.min(300, listView.contentHeight + 16)
        padding: 8
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

        background: Rectangle {
            radius: pal.radiusMd
            color: pal.card
            border.color: pal.border
            border.width: 1
        }

        contentItem: ListView {
            id: listView
            clip: true
            spacing: 2
            boundsBehavior: Flickable.StopAtBounds
            model: root.entries
            ScrollBar.vertical: ScrollBar {
                policy: ScrollBar.AsNeeded
            }

            delegate: ItemDelegate {
                id: rowDel
                width: listView.width
                height: 36
                required property int index
                required property var modelData

                readonly property string code: modelData.code
                readonly property string label: modelData.label
                readonly property string flag: modelData.flag

                contentItem: RowLayout {
                    spacing: 8
                    Text { text: rowDel.flag; font.pixelSize: 14 }
                    Text {
                        Layout.fillWidth: true
                        text: rowDel.label
                        color: pal.text
                        font.pixelSize: 13
                        elide: Text.ElideRight
                    }
                }

                background: Rectangle {
                    radius: pal.radiusSm
                    color: rowDel.hovered || rowDel.code === root.currentCode
                           ? pal.accentSoft : "transparent"
                }

                onClicked: {
                    root.currentCode = rowDel.code
                    root.codeSelected(rowDel.code)
                    popup.close()
                }
            }
        }
    }
}

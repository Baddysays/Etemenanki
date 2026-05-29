import QtQuick

/** Shared UI colors (regular type — instantiate as `EtePalette { id: pal }`). */
QtObject {
    readonly property color bg: "#f0f2f7"
    readonly property color card: "#ffffff"
    readonly property color surface: "#f8fafc"
    readonly property color text: "#1a2332"
    readonly property color muted: "#64748b"
    readonly property color border: "#e2e8f0"
    readonly property color accent: "#2563eb"
    readonly property color accentHover: "#3b82f6"
    readonly property color accentSoft: "#eff6ff"
    readonly property color accentText: "#1d4ed8"
    readonly property color ok: "#16a34a"
    readonly property color warn: "#d97706"
    readonly property int radiusSm: 8
    readonly property int radiusMd: 12
    readonly property int fontBody: 13
    readonly property int fontCaption: 11
    readonly property int fontTitle: 20
}

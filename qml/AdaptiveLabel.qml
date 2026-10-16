import QtQuick

Text {
    id: label
    required property string labelId
    property color preferred: app.colors.text || "#F5F7FF"
    property bool adaptive: true
    property var chosen: app.textColors[labelId]
    color: adaptive && app.settings.AutoTextContrast && chosen ? chosen.color : preferred
    style: adaptive && app.settings.AutoTextContrast && (!chosen || chosen.halo) ? Text.Outline : Text.Normal
    styleColor: chosen ? chosen.edge : (color.hslLightness > 0.5 ? "#D9050910" : "#D9FFFFFF")
    font.family: Qt.platform.os === "windows" ? "Segoe UI Variable" : Qt.platform.os === "osx" ? ".AppleSystemUIFont" : "Noto Sans"
    renderType: Text.QtRendering
    verticalAlignment: Text.AlignVCenter
    function sync() { if (adaptive) app.registerLabel(labelId, label, preferred) }
    Component.onCompleted: sync()
    Component.onDestruction: app.unregisterLabel(labelId, label)
    onPreferredChanged: sync()
    onVisibleChanged: sync()
}

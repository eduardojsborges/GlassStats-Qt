import QtQuick

Canvas {
    id: ring
    property real fraction: -1
    property color accent: app.colors.accent || "#7CA8FF"
    implicitWidth: 26
    implicitHeight: 26
    onFractionChanged: requestPaint()
    onAccentChanged: requestPaint()
    onWidthChanged: requestPaint()
    onHeightChanged: requestPaint()
    Component.onCompleted: app.registerExclusion(ring)
    Component.onDestruction: app.unregisterExclusion(ring)
    onPaint: {
        let c=getContext("2d");c.clearRect(0,0,width,height);let radius=Math.min(width,height)/2-2.3
        c.lineWidth=3.8;c.lineCap="round";c.beginPath();c.arc(width/2,height/2,radius,0,2*Math.PI);c.strokeStyle=Qt.rgba(accent.r,accent.g,accent.b,.24);c.stroke()
        if(fraction>=0){c.beginPath();c.arc(width/2,height/2,radius,-Math.PI/2,-Math.PI/2+Math.max(0,Math.min(1,fraction))*Math.PI*2);c.strokeStyle=accent;c.stroke()}
    }
}

import QtQuick

Canvas {
    id: graph
    property var points: []
    property color accent: app.colors.accent || "#7CA8FF"
    implicitHeight: 30
    antialiasing: true
    onPointsChanged: requestPaint()
    onAccentChanged: requestPaint()
    onWidthChanged: requestPaint()
    onHeightChanged: requestPaint()
    Component.onCompleted: app.registerExclusion(graph)
    Component.onDestruction: app.unregisterExclusion(graph)
    onPaint: {
        let c=getContext("2d");c.clearRect(0,0,width,height)
        if (!points || points.length < 2) return
        let segments=[],segment=[]
        for (let p of points) {
            if (p.value === null || p.value === undefined) { if (segment.length) segments.push(segment);segment=[];continue }
            segment.push({x:Math.max(0,Math.min(1,p.x))*width,y:height-3-Math.max(0,Math.min(100,p.value))/100*(height-7)})
        }
        if (segment.length) segments.push(segment)
        for (let s of segments) {
            let gradient=c.createLinearGradient(0,0,0,height);gradient.addColorStop(0,Qt.rgba(accent.r,accent.g,accent.b,.38));gradient.addColorStop(1,Qt.rgba(accent.r,accent.g,accent.b,.02))
            c.beginPath();c.moveTo(s[0].x,height);for (let p of s)c.lineTo(p.x,p.y);c.lineTo(s[s.length-1].x,height);c.closePath();c.fillStyle=gradient;c.fill()
            c.beginPath();c.moveTo(s[0].x,s[0].y);for(let i=1;i<s.length;i++)c.lineTo(s[i].x,s[i].y);c.strokeStyle=accent;c.lineWidth=1.6;c.lineJoin="round";c.lineCap="round";c.stroke()
        }
    }
}

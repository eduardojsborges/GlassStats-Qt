import QtQuick

Item {
    id:icon
    required property string labelId
    property string glyph:"settings"
    property color preferred:app.colors.text||"#F5F7FF"
    property var chosen:app.textColors[labelId]
    property color ink:app.settings.AutoTextContrast&&chosen?chosen.color:preferred
    implicitWidth:22;implicitHeight:22
    function sync(){app.registerLabel(labelId,icon,preferred);canvas.requestPaint()}
    Component.onCompleted:sync()
    Component.onDestruction:app.unregisterLabel(labelId,icon)
    onPreferredChanged:sync()
    onInkChanged:canvas.requestPaint()
    onChosenChanged:canvas.requestPaint()
    Canvas {
        id:canvas;anchors.fill:parent;antialiasing:true
        onPaint:{
            let c=getContext("2d");c.clearRect(0,0,width,height);c.save();c.scale(width/24,height/24)
            function path(){
                c.beginPath()
                if(icon.glyph==="settings"){
                    for(let i=0;i<32;i++){let a=i*Math.PI/16-Math.PI/2;let r=(i%4===0||i%4===1)?9.1:7.5;let x=12+Math.cos(a)*r,y=12+Math.sin(a)*r;if(i===0)c.moveTo(x,y);else c.lineTo(x,y)}
                    c.closePath();c.moveTo(15.5,12);c.arc(12,12,3.5,0,2*Math.PI)
                }else{c.moveTo(14,4);c.lineTo(20,4);c.lineTo(20,10);c.moveTo(20,4);c.lineTo(13,11);c.moveTo(4,14);c.lineTo(4,20);c.lineTo(10,20);c.moveTo(4,20);c.lineTo(11,13)}
            }
            c.lineJoin="round";c.lineCap="round"
            if(app.settings.AutoTextContrast&&(!icon.chosen||icon.chosen.halo)){path();c.lineWidth=3;c.strokeStyle=icon.chosen?icon.chosen.edge:(icon.ink.hslLightness>.5?"#D9050910":"#D9FFFFFF");c.stroke()}
            path();c.lineWidth=1.5;c.strokeStyle=icon.ink;c.stroke();c.restore()
        }
    }
}

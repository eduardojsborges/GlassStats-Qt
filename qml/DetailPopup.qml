import QtQuick
import QtQuick.Controls

Window {
    id:popup
    objectName:"detail."+metric
    required property string metric
    required property string title
    property bool memory: metric==="ram"||metric==="vram"
    property var reading:app.metrics[metric]||{}
    property var temperature:app.metrics[metric+"Temperature"]||{}
    required property Item anchorItem
    flags:Qt.ToolTip|Qt.FramelessWindowHint|Qt.WindowDoesNotAcceptFocus|Qt.WindowStaysOnTopHint|Qt.NoDropShadowWindowHint
    visible:false;color:"transparent";width:300;height:details.implicitHeight+32
    function open(){let point=anchorItem.mapToGlobal(0,anchorItem.height+10);x=point.x;y=point.y;show()}
    Rectangle {anchors.fill:parent;color:"#171A25";radius:14;border.color:"#485166";border.width:1}
    Column {
        id:details;x:16;y:16;width:parent.width-32;spacing:8
        Text {text:popup.title;color:"#F5F7FF";font.pixelSize:16;font.bold:true}
        Text {width:parent.width;text:popup.metric==="cpu"?(app.metrics.cpuName||"CPU"):popup.metric==="gpu"||popup.metric==="vram"?(app.metrics.gpuName||"GPU"):"System memory";wrapMode:Text.WordWrap;color:"#C1CAE0";font.pixelSize:13}
        Text {visible:!popup.memory;text:"Usage: "+app.formatReading(popup.reading)+(popup.reading.available?"%":" · "+(popup.reading.status||"Unavailable"));color:"#F5F7FF";font.pixelSize:15}
        Text {visible:!popup.memory&&app.settings.ShowTemperatures;text:"Temperature: "+app.formatReading(popup.temperature)+" "+(popup.temperature.unit||"°C");color:"#C1CAE0";font.pixelSize:13}
        Text {visible:popup.memory;text:{let unused=app.metrics;return app.memoryText(popup.metric)} color:"#F5F7FF";font.pixelSize:15}
        Text {visible:popup.memory;text:{let unused=app.metrics;return app.memoryText(popup.metric,"free")} color:"#C1CAE0";font.pixelSize:13}
        Text {width:parent.width;wrapMode:Text.WordWrap;text:popup.memory?(app.metrics[popup.metric+"Used"]||{}).detail||"":((popup.reading.detail||"")+"\n"+(app.settings.ShowTemperatures?(popup.temperature.detail||""):""));color:"#AEB8D0";font.pixelSize:12}
        Graph {visible:!popup.memory&&app.settings.ShowGraphs;width:parent.width;height:55;points:popup.metric==="cpu"?app.cpuHistory:app.gpuHistory}
    }
}

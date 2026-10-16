import QtQuick
import QtQuick.Controls

Popup {
    id:popup
    objectName:"detail."+metric
    required property string metric
    required property string title
    property bool memory: metric==="ram"||metric==="vram"
    property var reading:app.metrics[metric]||{}
    property var temperature:app.metrics[metric+"Temperature"]||{}
    popupType:Popup.Window
    modal:false;focus:false;closePolicy:Popup.CloseOnEscape
    x:0;y:parent.height+10;width:300;padding:16
    palette.window:"#171A25";palette.windowText:"#F5F7FF"
    background:Rectangle {color:"#171A25";radius:14;border.color:"#485166";border.width:1}
    contentItem:Column {
        spacing:8
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

import QtQuick
import QtQuick.Controls

Item {
    id: section
    required property string metric
    required property string title
    required property bool compact
    property bool memory: metric === "ram" || metric === "vram"
    property string labelPrefix: metric+(compact?".compact.":".full.")
    property var reading: app.metrics[metric] || {}
    property var temperature: app.metrics[metric + "Temperature"] || {}
    property real fraction: { let unused=app.metrics;return app.memoryFraction(metric) }
    property string memoryText: { let unused=app.metrics;return app.memoryText(metric) }
    property real valueSize: compact ? app.settings.CompactValueFontSize : app.settings.ValueFontSize
    property real labelSize: compact ? app.settings.CompactLabelFontSize : app.settings.LabelFontSize
    property real detailSize: compact ? app.settings.CompactDetailFontSize : app.settings.DetailFontSize
    property bool showTemp: !memory && app.settings.ShowTemperatures
    property string usageText: memory ? (fraction >= 0 ? Math.round(fraction * 100) + "%" : "—") : app.formatReading(reading) + (reading.available ? "%" : "")
    implicitWidth: compact ? Math.max(label.implicitWidth, values.implicitWidth) : 284
    implicitHeight: compact ? Math.max(60, label.implicitHeight+6+values.implicitHeight) : memory ? label.implicitHeight+12+Math.max(14,detailSize*1.4) : label.implicitHeight+6+values.implicitHeight+(app.settings.ShowGraphs ? 40 : 0)
    FontMetrics { id: usageMeasure; font.pixelSize: section.valueSize; font.weight: Font.DemiBold }
    FontMetrics { id: tempMeasure; font.pixelSize: section.detailSize }
    AdaptiveLabel {
        id: label;labelId: section.labelPrefix+"label";text:section.title;font.pixelSize:section.labelSize
        preferred:app.colors.secondary || "#AEB8D0";anchors.top:parent.top
    }
    AdaptiveLabel {
        visible:section.showTemp && !section.compact
        labelId:section.labelPrefix+"temperature";text:app.formatReading(section.temperature)+" "+(section.temperature.unit || "°C")
        preferred:app.colors.secondary || "#AEB8D0";font.pixelSize:section.detailSize
        anchors.right:parent.right;anchors.top:parent.top
    }
    AdaptiveLabel {
        visible:section.memory && !section.compact
        labelId:section.labelPrefix+"total";text:section.memoryText;font.pixelSize:section.detailSize
        preferred:app.colors.secondary || "#AEB8D0";anchors.right:parent.right;anchors.top:parent.top
    }
    Row {
        id:values;anchors.top:label.bottom;anchors.topMargin:6;spacing:section.compact?8:0
        visible:!section.memory || section.compact
        UsageRing { visible:section.memory;fraction:section.fraction;anchors.verticalCenter:parent.verticalCenter;width:Math.max(24,section.valueSize);height:width }
        AdaptiveLabel {
            labelId:section.labelPrefix+"value";text:section.usageText;font.pixelSize:section.valueSize;font.weight:Font.DemiBold
            width:section.compact?Math.max(usageMeasure.advanceWidth("100%"),usageMeasure.advanceWidth("Stale")):implicitWidth
            anchors.verticalCenter:parent.verticalCenter
        }
        AdaptiveLabel {
            visible:section.showTemp && section.compact
            labelId:section.labelPrefix+"temperature";text:app.formatReading(section.temperature)+" "+(section.temperature.unit || "°C")
            font.pixelSize:section.detailSize;preferred:app.colors.secondary || "#AEB8D0"
            width:Math.max(tempMeasure.advanceWidth("199 °F"),tempMeasure.advanceWidth("Stale °F"));anchors.verticalCenter:parent.verticalCenter
        }
    }
    Graph {
        visible:!section.compact&&!section.memory&&app.settings.ShowGraphs
        anchors.left:parent.left;anchors.right:parent.right;anchors.bottom:parent.bottom;height:30
        points:section.metric==="cpu"?app.cpuHistory:app.gpuHistory
    }
    Row {
        visible:section.memory&&!section.compact
        anchors.top:label.bottom;anchors.topMargin:12;spacing:9;width:parent.width
        Rectangle {
            id:bar;width:parent.width-pct.width-parent.spacing;height:10;radius:5;anchors.verticalCenter:parent.verticalCenter
            color:Qt.rgba(.5,.55,.7,.22)
            Rectangle {width:parent.width*Math.max(0,section.fraction);height:parent.height;radius:5;color:app.colors.accent || "#7CA8FF"}
            Component.onCompleted:app.registerExclusion(bar)
            Component.onDestruction:app.unregisterExclusion(bar)
        }
        AdaptiveLabel {id:pct;labelId:section.labelPrefix+"percent";text:section.usageText;font.pixelSize:section.detailSize;font.weight:Font.DemiBold;preferred:app.colors.accentText || "#7CA8FF"}
    }
    HoverHandler { id:hover;enabled:section.compact&&app.canHover }
    Timer { interval:400;running:hover.hovered&&section.compact&&app.canHover;onTriggered:details.open() }
    onVisibleChanged:if(!visible)details.close()
    Connections {target:hover;function onHoveredChanged(){if(!hover.hovered)details.close()} }
    Connections {target:app;function onDismissDetails(){details.close()}function onNativeChanged(){if(!app.canHover)details.close()} }
    DetailPopup {id:details;metric:section.metric;title:section.title;parent:section}
    Accessible.role:Accessible.StaticText
    Accessible.name:section.title+" "+section.usageText
}

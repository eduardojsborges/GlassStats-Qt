import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

Window {
    id:window
    objectName:"overlayWindow"
    title:"GlassStats Qt"
    width:320;height:400;color:"transparent"
    flags:Qt.Tool|Qt.FramelessWindowHint|Qt.WindowStaysOnTopHint|Qt.WindowDoesNotAcceptFocus|Qt.NoDropShadowWindowHint
    visible:false
    property bool compact:app.settings.ViewMode==="Compact"
    property var visibleMetrics: [ {key:"cpu",title:"CPU",show:app.settings.ShowCpu},{key:"gpu",title:"GPU",show:app.settings.ShowGpu},{key:"ram",title:"RAM",show:app.settings.ShowRam},{key:"vram",title:"VRAM",show:app.settings.ShowVram} ].filter(m=>m.show)
    property color surface:app.colors.surface||"#12111B"
    property color accent:app.colors.accent||"#7CA8FF"
    onClosing:function(close){close.accepted=false;app.action("visibility")}
    Item {
        id:card
        width:window.compact?compactLayout.implicitWidth:320
        height:window.compact?Math.max(78,compactLayout.implicitHeight+28):fullLayout.implicitHeight+34
        scale:app.renderScale;transformOrigin:Item.TopLeft
        onWidthChanged:sizeTimer.restart()
        onHeightChanged:sizeTimer.restart()
        Rectangle {
            anchors.fill:parent;anchors.margins:0.7/Math.max(.2,app.renderScale)
            radius:Math.min((window.compact?app.settings.CompactCornerRadius:app.settings.CornerRadius),height/2)
            color:Qt.rgba(window.surface.r,window.surface.g,window.surface.b,app.colors.opacity === undefined?.62:app.colors.opacity)
            border.width:1/Math.max(.2,app.renderScale*Screen.devicePixelRatio)
            border.color:Qt.rgba(window.accent.r,window.accent.g,window.accent.b,.72)
            antialiasing:true
        }
        MouseArea {
            anchors.fill:parent;enabled:!app.settings.Locked;acceptedButtons:Qt.LeftButton|Qt.RightButton
            onPressed:function(mouse){if(mouse.button===Qt.LeftButton)app.beginDrag()}
            onReleased:function(mouse){app.endDrag();if(mouse.button===Qt.RightButton)app.showMenu()}
            onCanceled:app.endDrag()
        }
        Column {
            id:fullLayout;visible:!window.compact;width:284;x:18;y:16;spacing:13
            Item {
                width:parent.width;height:Math.max(header.implicitHeight,24)
                AdaptiveLabel {id:header;labelId:"header";text:"GlassStats";font.pixelSize:app.settings.TitleFontSize;font.weight:Font.DemiBold;anchors.verticalCenter:parent.verticalCenter}
                Row {
                    anchors.right:parent.right;anchors.verticalCenter:parent.verticalCenter;spacing:5
                    ToolButton {width:24;height:24;hoverEnabled:true;Accessible.name:"Switch to compact view";onClicked:app.action("view");background:Rectangle{color:parent.hovered?"#207CA8FF":"transparent";radius:5}contentItem:AdaptiveLabel{labelId:"full.view";text:"⤢";font.pixelSize:20;horizontalAlignment:Text.AlignHCenter}}
                    ToolButton {width:24;height:24;hoverEnabled:true;Accessible.name:"Settings";onClicked:app.action("settings");background:Rectangle{color:parent.hovered?"#207CA8FF":"transparent";radius:5}contentItem:AdaptiveLabel{labelId:"full.settings";text:"⚙";font.pixelSize:19;horizontalAlignment:Text.AlignHCenter}
                        Rectangle{visible:app.updater.available;width:5;height:5;radius:3;color:window.accent;anchors.right:parent.right;anchors.top:parent.top}
                    }
                }
            }
            Repeater {
                model:window.visibleMetrics
                delegate:Column {
                    required property var modelData
                    width:fullLayout.width;spacing:12
                    Rectangle{width:parent.width;height:1;color:app.colors.dark?"#403E465B":"#406C758B"}
                    MetricSection{metric:modelData.key;title:modelData.title;compact:false;width:parent.width;height:implicitHeight}
                }
            }
            AdaptiveLabel {visible:window.visibleMetrics.length===0;labelId:"empty";text:"All metrics hidden";font.pixelSize:14}
        }
        Row {
            id:compactLayout;visible:window.compact;x:0;y:14;spacing:0
            Item{width:28;height:1}
            Repeater {
                model:window.visibleMetrics
                delegate:Row {
                    required property var modelData
                    required property int index
                    spacing:0
                    Item{visible:index>0;width:visible?8:0;height:1}
                    Rectangle{visible:index>0;width:visible?1:0;height:Math.max(44,compactSection.height-5);color:app.colors.dark?"#403E465B":"#406C758B";anchors.verticalCenter:parent.verticalCenter}
                    Item{visible:index>0;width:visible?8:0;height:1}
                    MetricSection{id:compactSection;metric:modelData.key;title:modelData.title;compact:true;width:implicitWidth;height:implicitHeight}
                }
            }
            AdaptiveLabel {visible:window.visibleMetrics.length===0;labelId:"compact.empty";text:"All metrics hidden";font.pixelSize:14;anchors.verticalCenter:parent.verticalCenter}
            Item{width:14;height:1}
            Column {
                spacing:2;anchors.verticalCenter:parent.verticalCenter
                ToolButton {width:24;height:24;hoverEnabled:true;Accessible.name:"Switch to full view";onClicked:app.action("view");background:Rectangle{color:parent.hovered?"#207CA8FF":"transparent";radius:5}contentItem:AdaptiveLabel{labelId:"compact.view";text:"⤢";font.pixelSize:20;horizontalAlignment:Text.AlignHCenter}}
                ToolButton {width:24;height:24;hoverEnabled:true;Accessible.name:"Settings";onClicked:app.action("settings");background:Rectangle{color:parent.hovered?"#207CA8FF":"transparent";radius:5}contentItem:AdaptiveLabel{labelId:"compact.settings";text:"⚙";font.pixelSize:19;horizontalAlignment:Text.AlignHCenter}
                    Rectangle{visible:app.updater.available;width:5;height:5;radius:3;color:window.accent;anchors.right:parent.right;anchors.top:parent.top}
                }
            }
            Item{width:18;height:1}
        }
    }
    Timer{id:sizeTimer;interval:0;onTriggered:app.setContentSize(card.width,card.height)}
    SettingsWindow{id:settingsWindow}
    Window {
        id:selectionHud;title:"GlassStats · Select a window";visible:app.mode==="Selecting";width:410;height:96
        flags:Qt.Tool|Qt.WindowStaysOnTopHint
        color:"#171A25"
        Column{anchors.fill:parent;anchors.margins:12;spacing:7
            Text{text:app.status||"Click a window to attach · Esc to cancel";color:"#F5F7FF";font.pixelSize:13;width:parent.width;wrapMode:Text.WordWrap}
            Button{text:"Cancel";onClicked:app.action("glue")}
        }
        onClosing:function(close){close.accepted=false;app.action("glue")}
    }
    Connections {target:app;function onOpenSettings(){settingsWindow.show();settingsWindow.raise();settingsWindow.requestActivate()}function onInstallConfirmation(path){settingsWindow.showInstall(path)}}
    Component.onCompleted:sizeTimer.start()
}

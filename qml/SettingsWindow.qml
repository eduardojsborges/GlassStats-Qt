import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

ApplicationWindow {
    id:settingsWindow
    objectName:"settingsWindow"
    title:"GlassStats settings · Qt preview"
    width:620;height:790;minimumWidth:480;minimumHeight:450;visible:false
    property bool dark:app.settings.Theme==="Dark"||(app.settings.Theme==="System"&&!!app.colors.dark)
    property color ink:dark?"#F5F7FF":"#181D27"
    property color muted:dark?"#B5BFD5":"#4D596F"
    property string selectedColorKey:"AccentColor"
    property string installerPath:""
    color:dark?"#191C26":"#F4F6FA"
    palette.window:color;palette.windowText:ink;palette.text:ink;palette.buttonText:ink
    palette.base:dark?"#252A38":"#FFFFFF";palette.button:dark?"#303647":"#E5EAF3";palette.highlight:app.colors.accent||"#7CA8FF";palette.highlightedText:"#101522"
    function showInstall(path){installerPath=path;show();raise();requestActivate();installDialog.open()}
    ScrollView {
        anchors.fill:parent;contentWidth:availableWidth;clip:true
        ColumnLayout {
            width:settingsWindow.width-44;x:22;spacing:12
            Label{text:"GlassStats "+app.version;font.pixelSize:21;font.bold:true;Layout.topMargin:20;color:settingsWindow.ink}
            Label{text:"C++ / Qt preview · Windows parity in validation · Mac/Linux experimental";wrapMode:Text.WordWrap;Layout.fillWidth:true;color:settingsWindow.muted}
            Frame {
                visible:app.legacyAvailable&&!app.settings.ImportOffered;Layout.fillWidth:true
                ColumnLayout {width:parent.width;Label{text:"Import your current GlassStats preferences?";font.bold:true;wrapMode:Text.WordWrap;Layout.fillWidth:true}
                    Label{text:"This copies appearance and placement into a separate Qt profile. The original app and settings stay intact.";wrapMode:Text.WordWrap;Layout.fillWidth:true}
                    RowLayout{Button{text:"Import a copy";onClicked:app.importLegacy()}Button{text:"Use defaults";onClicked:app.skipImport()}}
                }
            }
            Label{visible:app.status.length>0;text:app.status;color:settingsWindow.muted;wrapMode:Text.WordWrap;Layout.fillWidth:true}
            Label{text:"Appearance";font.pixelSize:17;font.bold:true;Layout.topMargin:8}
            RowLayout {
                Layout.fillWidth:true
                Label{text:"Theme";Layout.fillWidth:true}
                ComboBox{model:["System","Light","Dark"];currentIndex:model.indexOf(app.settings.Theme);onActivated:app.setSetting("Theme",currentText);Layout.preferredWidth:200}
            }
            Repeater {
                model:[{key:"BackgroundColor",label:"Background tint",automatic:true},{key:"TextColor",label:"Text color",automatic:true},{key:"AccentColor",label:"Accent color",automatic:false}]
                delegate:RowLayout {
                    required property var modelData
                    Layout.fillWidth:true
                    Label{text:modelData.label;Layout.fillWidth:true}
                    Button {
                        text:"Pick…";Layout.preferredWidth:80
                        background:Rectangle{color:app.settings[modelData.key]||settingsWindow.palette.button;border.width:1;border.color:settingsWindow.muted;radius:5}
                        onClicked:{settingsWindow.selectedColorKey=modelData.key;colorPicker.selectedColor=app.settings[modelData.key]||"#7CA8FF";colorPicker.open()}
                    }
                    TextField {text:app.settings[modelData.key];placeholderText:modelData.automatic?"Automatic":"#7CA8FF";Layout.preferredWidth:118;selectByMouse:true;validator:RegularExpressionValidator{regularExpression:/^(#[a-fA-F0-9]{6})?$/} onEditingFinished:if(acceptableInput)app.setSetting(modelData.key,text)}
                }
            }
            Label{text:"Background opacity: "+Math.round(app.settings.Opacity*100)+"%"}
            Slider{Layout.fillWidth:true;from:0;to:1;stepSize:.01;value:app.settings.Opacity;onMoved:app.setSetting("Opacity",value)}
            Switch{text:"Protect text contrast (optional glass veil)";checked:app.settings.ProtectContrast;onToggled:app.setSetting("ProtectContrast",checked)}
            Label{text:"Adds the minimum background veil needed for readability. Turn off for clear glass at 0% opacity.";color:settingsWindow.muted;wrapMode:Text.WordWrap;Layout.fillWidth:true}
            Switch{text:"Adapt text to background";checked:app.settings.AutoTextContrast;onToggled:app.setSetting("AutoTextContrast",checked)}
            Label{text:"Adjusts text and icons over visible content without changing opacity. Pixels stay in memory only. Unsupported or denied capture uses a subtle glyph halo.";color:settingsWindow.muted;wrapMode:Text.WordWrap;Layout.fillWidth:true}
            Label{text:"Overlay and sizing";font.pixelSize:17;font.bold:true;Layout.topMargin:8}
            RowLayout{Layout.fillWidth:true;Label{text:"View";Layout.fillWidth:true}ComboBox{model:["Full","Compact"];currentIndex:model.indexOf(app.settings.ViewMode);onActivated:app.setSetting("ViewMode",currentText);Layout.preferredWidth:200}}
            Repeater {
                model:[{key:"Scale",label:"Full scale",min:0.67,max:1.67,step:.01,percent:true},{key:"CornerRadius",label:"Full corner radius",min:0,max:32,step:1},{key:"TitleFontSize",label:"Full title text",min:14,max:28,step:1},{key:"LabelFontSize",label:"Full labels",min:11,max:20,step:1},{key:"ValueFontSize",label:"Full usage text",min:26,max:56,step:1},{key:"DetailFontSize",label:"Full details",min:11,max:18,step:1},{key:"CompactScale",label:"Compact scale",min:.67,max:1.67,step:.01,percent:true},{key:"CompactCornerRadius",label:"Compact corner radius",min:0,max:80,step:1},{key:"CompactLabelFontSize",label:"Compact labels",min:11,max:20,step:1},{key:"CompactValueFontSize",label:"Compact usage text",min:18,max:48,step:1},{key:"CompactDetailFontSize",label:"Compact details",min:11,max:24,step:1}]
                delegate:ColumnLayout {
                    required property var modelData
                    Layout.fillWidth:true
                    Label{text:modelData.label+": "+Math.round(app.settings[modelData.key]*(modelData.percent?100:1))+(modelData.percent?"%":" px")}
                    Slider{Layout.fillWidth:true;from:modelData.min;to:modelData.max;stepSize:modelData.step;value:app.settings[modelData.key];onMoved:app.setSetting(modelData.key,value)}
                }
            }
            Label{text:"Metrics";font.pixelSize:17;font.bold:true;Layout.topMargin:8}
            Repeater {
                model:[{key:"ShowCpu",label:"CPU usage"},{key:"ShowGpu",label:"GPU usage"},{key:"ShowRam",label:"RAM usage"},{key:"ShowVram",label:"VRAM usage"},{key:"ShowTemperatures",label:"Temperatures"},{key:"ShowGraphs",label:"60-second graphs"}]
                delegate:Switch{required property var modelData;text:modelData.label;checked:app.settings[modelData.key];onToggled:app.setSetting(modelData.key,checked)}
            }
            RowLayout{Layout.fillWidth:true;Label{text:"Refresh interval";Layout.fillWidth:true}ComboBox{model:["0.5 seconds","1 second","2 seconds"];currentIndex:app.settings.RefreshSeconds===.5?0:app.settings.RefreshSeconds===2?2:1;onActivated:app.setSetting("RefreshSeconds",[.5,1,2][currentIndex]);Layout.preferredWidth:200}}
            RowLayout{Layout.fillWidth:true;Label{text:"Temperature units";Layout.fillWidth:true}ComboBox{model:["Celsius","Fahrenheit"];currentIndex:app.settings.TemperatureUnit==="F"?1:0;onActivated:app.setSetting("TemperatureUnit",currentIndex===1?"F":"C");Layout.preferredWidth:200}}
            RowLayout{Layout.fillWidth:true;Label{text:"GPU";Layout.fillWidth:true}ComboBox{model:app.adapters;textRole:"name";valueRole:"id";Layout.preferredWidth:300;currentIndex:{let list=app.adapters;let i=list.findIndex(a=>a.id===app.settings.GpuId);return i>=0?i:0}onActivated:app.setSetting("GpuId",currentValue)}}
            Label{text:"Temperature sensors";font.pixelSize:17;font.bold:true;Layout.topMargin:8}
            Repeater {
                model:["CPU","GPU"]
                delegate:RowLayout {
                    required property string modelData
                    property string settingKey:modelData==="CPU"?"CpuSensorId":"GpuSensorId"
                    property var choices:[{id:"",name:"Automatic"}].concat(app.sensors.filter(s=>s.kind===modelData&&(modelData==="CPU"||s.device===app.metrics.gpuName)))
                    Layout.fillWidth:true
                    Label{text:modelData+" temperature";Layout.fillWidth:true}
                    ComboBox{model:parent.choices;textRole:"name";valueRole:"id";Layout.preferredWidth:300;currentIndex:{let i=parent.choices.findIndex(s=>s.id===app.settings[parent.settingKey]);return Math.max(0,i)}onActivated:app.setSetting(parent.settingKey,currentValue)}
                }
            }
            Flow{Layout.fillWidth:true;spacing:8;Button{text:"Set up helper";onClicked:app.helper(true)}Button{text:"Remove helper";onClicked:app.helper(false)}}
            Label{text:"Optional Windows setup requests administrator approval. The overlay stays unelevated; other metrics work if setup is canceled.";color:settingsWindow.muted;wrapMode:Text.WordWrap;Layout.fillWidth:true}
            Label{text:"Attachment and recovery";font.pixelSize:17;font.bold:true;Layout.topMargin:8}
            Label{text:app.attachment;wrapMode:Text.WordWrap;Layout.fillWidth:true}
            Flow{Layout.fillWidth:true;spacing:8;Button{text:app.mode==="Selecting"?"Cancel selection":app.mode==="Attached"?"Change attached window…":"Glue to app…";onClicked:app.action("glue")}Button{text:"Unglue and show on desktop";onClicked:app.action("recover")}}
            Label{text:"Attachment follows one window for this session. Readings remain system-wide. Recovery detaches, unlocks, and restores the last desktop position.";color:settingsWindow.muted;wrapMode:Text.WordWrap;Layout.fillWidth:true}
            Switch{text:"Start at sign-in";checked:app.settings.StartAtSignIn;onToggled:app.setSetting("StartAtSignIn",checked)}
            Button{visible:app.legacyStartup;text:"Switch sign-in startup to Qt";onClicked:startupDialog.open()}
            Switch{text:"Lock / let clicks pass through";checked:app.settings.Locked;onToggled:app.setSetting("Locked",checked)}
            Repeater {
                model:[{key:"Shortcut",label:"Lock shortcut"},{key:"RecoveryShortcut",label:"Recovery shortcut"}]
                delegate:RowLayout{required property var modelData;Layout.fillWidth:true;Label{text:modelData.label;Layout.fillWidth:true}TextField{text:app.settings[modelData.key];Layout.preferredWidth:200;onEditingFinished:app.setSetting(modelData.key,text)}}
            }
            Label{text:app.shortcutError;color:settingsWindow.muted;visible:text.length>0;wrapMode:Text.WordWrap;Layout.fillWidth:true}
            Label{text:"Platform capabilities";font.pixelSize:17;font.bold:true;Layout.topMargin:8}
            Repeater{model:app.capabilities;delegate:ColumnLayout{required property var modelData;Layout.fillWidth:true;Label{text:modelData.name+": "+modelData.state;font.bold:true}Label{text:modelData.reason;color:settingsWindow.muted;wrapMode:Text.WordWrap;Layout.fillWidth:true}}}
            Flow{Layout.fillWidth:true;spacing:8;Button{text:"Screen capture permission";onClicked:app.requestPermission("capture")}Button{text:"Accessibility permission";onClicked:app.requestPermission("attachment")}}
            Button{text:"Reset defaults";onClicked:resetDialog.open()}
            Label{text:"Updates";font.pixelSize:17;font.bold:true;Layout.topMargin:8}
            Label{text:"Updates come from eduardojsborges/GlassStats-Qt, separately from the C# app.";color:settingsWindow.muted;wrapMode:Text.WordWrap;Layout.fillWidth:true}
            Switch{text:"Automatically check daily";checked:app.settings.AutoCheckUpdates;onToggled:app.setSetting("AutoCheckUpdates",checked)}
            Switch{text:"Include beta releases";checked:app.settings.IncludeBetaUpdates;onToggled:app.setSetting("IncludeBetaUpdates",checked)}
            Label{text:app.updater.status;wrapMode:Text.WordWrap;Layout.fillWidth:true}
            Flow{Layout.fillWidth:true;spacing:8;Button{text:"Check for updates";enabled:!app.updater.busy;onClicked:app.checkUpdates()}Button{text:"Download and install";visible:app.updater.available;enabled:!app.updater.busy;onClicked:downloadDialog.open()}Button{text:"Open GitHub release";visible:app.updater.available;onClicked:app.updater.openRelease()}Button{text:"Cancel";visible:app.updater.busy;onClicked:app.updater.cancel()}}
            ProgressBar{visible:app.updater.busy;Layout.fillWidth:true;value:app.updater.progress;indeterminate:app.updater.progress<0}
            Label{text:app.updater.notes;wrapMode:Text.WordWrap;Layout.fillWidth:true;color:settingsWindow.muted}
            Label{text:"Local monitoring needs no account or network. Update checks contact GitHub without uploading readings or preferences.";wrapMode:Text.WordWrap;Layout.fillWidth:true;color:settingsWindow.muted;Layout.bottomMargin:24}
        }
    }
    ColorDialog {id:colorPicker;title:"Choose "+settingsWindow.selectedColorKey;options:ColorDialog.NoOptions;onAccepted:app.setSetting(settingsWindow.selectedColorKey,selectedColor.toString().substring(0,7))}
    Dialog{id:downloadDialog;title:"Download update?";modal:true;anchors.centerIn:parent;standardButtons:Dialog.Ok|Dialog.Cancel;Label{text:"Download "+app.updater.releaseTag+" and verify its SHA-256 checksum?\nInstallation requires a separate interactive confirmation.";wrapMode:Text.WordWrap;width:380}onAccepted:app.updater.download()}
    Dialog{id:installDialog;title:"Verified update ready";modal:true;anchors.centerIn:parent;standardButtons:Dialog.Ok|Dialog.Cancel;Label{text:"Open the verified installer/package now?\nMonitoring continues. Installation is interactive.\nmacOS: copy the app from the disk image. Linux: quit before launching the new AppImage.";wrapMode:Text.WordWrap;width:380}onAccepted:app.openVerifiedInstaller(settingsWindow.installerPath)}
    Dialog{id:resetDialog;title:"Reset both views?";modal:true;anchors.centerIn:parent;standardButtons:Dialog.Reset|Dialog.Cancel;Label{text:"Restore appearance, metrics, sizes, and shortcuts to defaults?";wrapMode:Text.WordWrap;width:350}onReset:app.resetDefaults()}
    Dialog{id:startupDialog;title:"Switch startup to Qt?";modal:true;anchors.centerIn:parent;standardButtons:Dialog.Yes|Dialog.No;Label{text:"Remove the legacy GlassStats sign-in entry and start the Qt remake instead?\nThe legacy app and its preferences are kept.";wrapMode:Text.WordWrap;width:360}onAccepted:app.action("switchStartup")}
}

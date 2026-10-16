#include "Controller.h"
#include <QCoreApplication>
#include <QStandardPaths>
#include <QStyleHints>
#include <QJsonDocument>
#include <QFile>
#include <QSaveFile>
#include <QDir>
#include <QDesktopServices>
#include <QFontMetricsF>
#include <QApplication>
#include <QScreen>
#include <QCursor>
#include <QtMath>
#include <cmath>

namespace gs {
Controller::Controller(bool validation,QString profile,QObject* parent):QObject(parent),platform_(createPlatform()),validation_(validation),updater_(this){
    path_=(profile.isEmpty()?QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation):profile)+"/settings.json";
#ifdef Q_OS_WIN
    legacyPath_=qEnvironmentVariable("LOCALAPPDATA")+"/GlassStats/settings.json";
#endif
    settings_=validation?defaults():loadSettings(path_,&status_);
    if(validation_)settings_["StartAtSignIn"]=false;
    worker_=new QObject;worker_->moveToThread(&monitorThread_);connect(&monitorThread_,&QThread::finished,worker_,&QObject::deleteLater);monitorThread_.start();
    sampleTimer_.setInterval(1000);connect(&sampleTimer_,&QTimer::timeout,this,&Controller::scheduleSample);
    adaptiveTimer_.setInterval(250);connect(&adaptiveTimer_,&QTimer::timeout,this,&Controller::sampleSurface);
    watchdog_.setInterval(1000);connect(&watchdog_,&QTimer::timeout,this,[this]{platform_->reconcile();auto b=platform_->bounds();auto covers=platform_->coveredAreas();if(b!=lastBounds_||covers!=lastCovers_){lastBounds_=b;lastCovers_=covers;invalidate();emit dismissDetails();}
        if(lastSample_.isValid()&&lastSample_.msecsTo(QDateTime::currentDateTimeUtc())>5000){for(auto key:{"cpu","gpu","ramUsed","ramTotal","vramUsed","vramTotal","cpuTemperature","gpuTemperature"}){auto reading=metrics_[key].toMap();reading["status"]="Stale";reading["available"]=false;reading["value"]=QVariant();reading["detail"]="Monitoring sample is stale";metrics_[key]=reading;}emit metricsChanged();}
        if(settings_["AutoCheckUpdates"].toBool()&&!validation_)checkUpdates(false);});
    saveTimer_.setSingleShot(true);saveTimer_.setInterval(350);connect(&saveTimer_,&QTimer::timeout,this,&Controller::save);
    dragTimer_.setInterval(16);connect(&dragTimer_,&QTimer::timeout,this,[this]{platform_->updateDrag();invalidate();});
    connect(platform_.get(),&Platform::changed,this,[this]{invalidate();emit nativeChanged();});
    connect(platform_.get(),&Platform::placementChanged,this,&Controller::refreshGeometry);
    connect(platform_.get(),&Platform::positionCommitted,this,[this]{invalidate();if(!platform_->attached()){auto b=platform_->bounds();settings_["RightX"]=b.x()+b.width();settings_["Y"]=b.y();emit settingsChanged();saveTimer_.start();}});
    connect(platform_.get(),&Platform::displayChanged,this,[this]{invalidate();fitWindow();});
    connect(platform_.get(),&Platform::targetLost,this,[this]{action("recover");setStatus("Attached window closed; GlassStats returned to the desktop.");});
    connect(platform_.get(),&Platform::requestCommand,this,&Controller::action);
    connect(platform_.get(),&Platform::status,this,&Controller::setStatus);
    connect(&tray_,&QSystemTrayIcon::activated,this,[this](auto reason){if(reason==QSystemTrayIcon::DoubleClick)action("settings");});
    connect(&menu_,&QMenu::aboutToShow,this,[this]{buildMenu();menuOpen_=true;invalidate();emit dismissDetails();emit nativeChanged();});
    connect(&menu_,&QMenu::aboutToHide,this,[this]{menuOpen_=false;invalidate();emit nativeChanged();});
    connect(&updater_,&Updater::changed,this,[this]{if(!validation_)tray_.setToolTip(updater_.available()?"GlassStats · Update available":"GlassStats · Performance at a glance");});
    connect(&updater_,&Updater::checkFinished,this,[this]{settings_["LastUpdateCheckUtc"]=QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs);saveTimer_.start();});
    connect(&updater_,&Updater::installerReady,this,&Controller::installConfirmation);
    connect(QGuiApplication::styleHints(),&QStyleHints::colorSchemeChanged,this,[this]{refreshPalette();});
    connect(qApp,&QGuiApplication::screenAdded,this,[this](QScreen*){fitWindow();});connect(qApp,&QGuiApplication::screenRemoved,this,[this](QScreen*){fitWindow();});
}
Controller::~Controller(){shuttingDown_=true;sampleTimer_.stop();adaptiveTimer_.stop();watchdog_.stop();dragTimer_.stop();save();tray_.hide();platform_->cancelSelection();QMetaObject::invokeMethod(worker_,[this]{monitor_.reset();capture_.reset();},Qt::BlockingQueuedConnection);monitorThread_.quit();monitorThread_.wait();}
void Controller::initialize(QQuickWindow* w){window_=w;platform_->initialize(w);connect(w,&QWindow::screenChanged,this,[this]{invalidate();QTimer::singleShot(0,this,[this]{fitWindow();});});
    if(!validation_){tray_.setIcon(QIcon(":/assets/GlassStats.ico"));tray_.setToolTip("GlassStats · Performance at a glance");buildMenu();tray_.setContextMenu(&menu_);tray_.show();}
    if(!validation_&&platform_->legacyStartup()&&settings_["StartAtSignIn"].toBool()&&!platform_->startupEnabled()){settings_["StartAtSignIn"]=false;setStatus("Legacy GlassStats starts at sign-in. Switch startup explicitly in Settings when ready.");}
    apply(false);platform_->setUserVisible(true);sampleTimer_.start();adaptiveTimer_.start();watchdog_.start();scheduleSample();
    if(!validation_&&legacyAvailable()&&!settings_["ImportOffered"].toBool())QTimer::singleShot(700,this,[this]{emit openSettings();});
}
void Controller::save(){if(!validation_&&!shuttingDown_){QString error;if(!saveSettings(path_,settings_,&error))setStatus("Could not save preferences: "+error);}}
void Controller::apply(bool persist){settings_=normalize(settings_);sampleTimer_.setInterval(qRound(settings_["RefreshSeconds"].toDouble()*1000));refreshPalette();if(window_){bool compact=settings_["ViewMode"]=="Compact";platform_->setLayout(logical_,.75*settings_[compact?"CompactScale":"Scale"].toDouble(),settings_[compact?"CompactCornerRadius":"CornerRadius"].toDouble());platform_->setLocked(settings_["Locked"].toBool());shortcutError_=platform_->registerShortcuts(settings_["Shortcut"].toString(),settings_["RecoveryShortcut"].toString(),validation_);fitWindow();}
    if(persist)saveTimer_.start();emit statusChanged();emit settingsChanged();emit nativeChanged();}
void Controller::setSetting(QString key,QVariant value){if(!defaults().contains(key))return;auto before=settings_.value(key);if(before==value)return;
    if(key=="StartAtSignIn"&&!validation_){auto error=platform_->setStartup(value.toBool());if(!error.isEmpty()){setStatus(error);return;}}
    settings_[key]=value;invalidate();emit dismissDetails();
    if(key=="GpuId"){++sampleGeneration_;cpuHistory_.points.clear();gpuHistory_.points.clear();frame_.sample.gpu=Reading::missing("%","GPU selection changed",Availability::WarmingUp);frame_.sample.vramUsed=Reading::missing("bytes","GPU selection changed",Availability::WarmingUp);metrics_=frame_.sample.variant(settings_["TemperatureUnit"].toString());emit metricsChanged();}
    apply();if(key=="GpuId"||key=="CpuSensorId"||key=="GpuSensorId")scheduleSample();}
void Controller::resetDefaults(){action("recover");settings_=defaults();settings_["ImportOffered"]=true;if(!validation_){auto error=platform_->setStartup(true);if(!error.isEmpty()){settings_["StartAtSignIn"]=false;setStatus(error);}}apply();}
bool Controller::legacyAvailable()const{return !legacyPath_.isEmpty()&&QFileInfo::exists(legacyPath_);}
void Controller::importLegacy(){if(!legacyAvailable())return;QFile f(legacyPath_);if(!f.open(QIODevice::ReadOnly)){setStatus(f.errorString());return;}auto bytes=f.read(1024*1024);auto document=QJsonDocument::fromJson(bytes);if(!document.isObject()){setStatus("Legacy preferences are invalid; nothing changed.");return;}if(!validation_){QDir().mkpath(QFileInfo(path_).absolutePath());QSaveFile backup(QFileInfo(path_).absolutePath()+"/legacy-import.json");if(backup.open(QIODevice::WriteOnly)){backup.write(bytes);backup.commit();}}
    settings_=migrateLegacy(document.object());settings_["Locked"]=false;settings_["StartAtSignIn"]=platform_->startupEnabled();if(settings_["LegacyLeftAnchor"].toBool()){bool compact=settings_["ViewMode"]=="Compact";int x=settings_["X"].toInt(),y=settings_.value("Y",24).toInt();auto m=selectMonitor(platform_->monitors(),{x,y});double width=compact?640:320;settings_["RightX"]=x+qRound(width*.75*settings_[compact?"CompactScale":"Scale"].toDouble()*m.dpi/96);settings_.remove("LegacyLeftAnchor");}apply();setStatus("Legacy settings copied. The original preferences and startup entry were not changed.");}
void Controller::skipImport(){settings_["ImportOffered"]=true;emit settingsChanged();saveTimer_.start();}
void Controller::scheduleSample(){if(sampling_||shuttingDown_)return;sampling_=true;auto snapshot=settings_;auto generation=sampleGeneration_;QPointer<Controller> self=this;QMetaObject::invokeMethod(worker_,[this,self,snapshot,generation]{Frame frame;try{if(!monitor_)monitor_=std::shared_ptr<MonitorProvider>(createMonitorProvider());frame=monitor_->read(snapshot);}catch(const std::exception& e){frame.error=QString::fromUtf8(e.what());frame.sample.at=QDateTime::currentDateTimeUtc();}QMetaObject::invokeMethod(this,[self,frame,generation]{if(self)self->receive(frame,generation);},Qt::QueuedConnection);},Qt::QueuedConnection);}
void Controller::receive(Frame f,quint64 generation){sampling_=false;if(generation!=sampleGeneration_){scheduleSample();return;}if(!f.error.isEmpty())setStatus(f.error);if(frame_.sample.gpuId!=f.sample.gpuId)gpuHistory_.points.clear();bool devices=frame_.adapters.size()!=f.adapters.size()||frame_.sensors.size()!=f.sensors.size()||frame_.sample.gpuName!=f.sample.gpuName;frame_=f;lastSample_=f.sample.at;cpuHistory_.add(f.sample.at,f.sample.cpu.status==Availability::Available?f.sample.cpu.value:std::nullopt);gpuHistory_.add(f.sample.at,f.sample.gpu.status==Availability::Available?f.sample.gpu.value:std::nullopt);metrics_=f.sample.variant(settings_["TemperatureUnit"].toString());emit metricsChanged();if(devices)emit devicesChanged();}
void Controller::refreshPalette(){auto p=palette(settings_,QGuiApplication::styleHints()->colorScheme()==Qt::ColorScheme::Dark);colors_={{"surface",p.surface},{"text",p.text},{"secondary",p.secondary},{"accent",QColor(settings_["AccentColor"].toString())},{"opacity",p.opacity},{"dark",luminance(p.surface)<.35}};textColors_.clear();for(auto& label:labels_)label.contrast=AdaptiveContrast();emit colorsChanged();emit textColorsChanged();invalidate();}
void Controller::invalidate(){++surfaceGeneration_;}
void Controller::fitWindow(){if(!window_)return;bool compact=settings_["ViewMode"]=="Compact";platform_->setLayout(logical_,.75*settings_[compact?"CompactScale":"Scale"].toDouble(),settings_[compact?"CompactCornerRadius":"CornerRadius"].toDouble());std::optional<QPoint> anchor;if(settings_.value("RightX").isValid()&&!settings_.value("RightX").isNull()&&settings_.value("Y").isValid())anchor=QPoint(settings_["RightX"].toInt(),settings_["Y"].toInt());if(platform_->attached())platform_->fitCurrent();else{auto p=platform_->fitDesktop(anchor);settings_["RightX"]=p.rect.x()+p.rect.width();settings_["Y"]=p.rect.y();}refreshGeometry();invalidate();}
void Controller::refreshGeometry(){if(!window_)return;auto b=platform_->bounds();double d=window_->devicePixelRatio();renderScale_=b.width()/std::max(1.,logical_.width())/d;emit geometryChanged();}
void Controller::setContentSize(double width,double height){if(!std::isfinite(width)||!std::isfinite(height)||width<=0||height<=0)return;QSizeF size(std::ceil(width),std::ceil(height));if(size==logical_)return;logical_=size;fitWindow();saveTimer_.start();}
void Controller::registerLabel(QString id,QQuickItem* item,QColor preferred){if(!item)return;auto i=labels_.find(id);if(i!=labels_.end()&&i->item==item){if(i->preferred!=preferred){i->preferred=preferred;i->contrast=AdaptiveContrast();invalidate();}return;}labels_[id]={item,preferred,{}};invalidate();}
void Controller::unregisterLabel(QString id,QQuickItem* item){auto i=labels_.find(id);if(i!=labels_.end()&&i->item==item){labels_.erase(i);textColors_.remove(id);invalidate();}}
void Controller::registerExclusion(QQuickItem* item){if(item&&!exclusions_.contains(item)){exclusions_<<item;invalidate();}}
void Controller::unregisterExclusion(QQuickItem* item){exclusions_.removeAll(item);invalidate();}
void Controller::sampleSurface(){if(captureBusy_||!window_||!settings_["AutoTextContrast"].toBool()||!platform_->effectiveVisible()||platform_->selecting()||platform_->dragging()||menuOpen_)return;auto b=platform_->bounds();auto covers=platform_->coveredAreas();if(fullyCovered(b,covers))return;
    auto rect=[&](QQuickItem* item){auto scene=item->mapRectToScene(QRectF(0,0,item->width(),item->height()));double d=window_->devicePixelRatio();return QRect(b.topLeft()+QPoint(qRound(scene.x()*d),qRound(scene.y()*d)),QSize(qCeil(scene.width()*d),qCeil(scene.height()*d)));};
    CaptureRequest request{surfaceGeneration_,b,platform_->placement().radius,{},{},covers};for(auto i=labels_.begin();i!=labels_.end();){auto item=i->item;if(!item){i=labels_.erase(i);continue;}if(item->isVisible()&&item->opacity()>0&&item->width()>0){auto r=rect(item);request.regions<<CaptureRegion{i.key(),r,i->preferred};request.exclusions<<r;}++i;}
    for(auto item:exclusions_)if(item&&item->isVisible())request.exclusions<<rect(item);if(request.regions.isEmpty())return;captureBusy_=true;QPointer<Controller> self=this;
    QMetaObject::invokeMethod(worker_,[this,self,request]{if(!capture_)capture_=createSurfaceCapture();auto frame=capture_->capture(request);QMetaObject::invokeMethod(this,[self,frame,request]{if(!self)return;self->captureBusy_=false;if(frame.generation!=self->surfaceGeneration_||frame.card!=self->platform_->bounds()||request.covers!=self->platform_->coveredAreas()||!self->platform_->effectiveVisible())return;++self->adaptiveFrames_;auto next=self->textColors_;for(const auto& region:request.regions){auto i=self->labels_.find(region.id);if(i==self->labels_.end())continue;auto result=i->contrast.select(i->preferred,frame.surfaces.value(region.id));next[region.id]=QVariantMap{{"color",result.color},{"halo",result.halo},{"edge",luminance(result.color)>.4?QColor(5,9,16,220):QColor(255,255,255,220)}};}if(next!=self->textColors_){self->textColors_=next;emit self->textColorsChanged();}},Qt::QueuedConnection);},Qt::QueuedConnection);
}
QVariantList Controller::adapters()const{QVariantList a;for(auto d:frame_.adapters)a<<QVariantMap{{"id",d.persistentId.isEmpty()?d.id:d.persistentId},{"name",d.name}};return a;}
QVariantList Controller::sensors()const{QVariantList a;for(auto s:frame_.sensors)a<<QVariantMap{{"id",s.id},{"name",s.name+" · "+s.device},{"kind",s.kind},{"device",s.device}};return a;}
QVariantList Controller::capabilities()const{QVariantList out;auto list=platform_->capabilities();for(auto i=list.begin();i!=list.end();++i){auto value=i.value().variant();value["name"]=i.key();out<<value;}return out;}
void Controller::beginDrag(){if(settings_["Locked"].toBool())return;emit dismissDetails();invalidate();platform_->beginDrag();dragTimer_.start();}
void Controller::endDrag(){dragTimer_.stop();platform_->endDrag();invalidate();}
void Controller::buildMenu(){menu_.clear();auto add=[&](QString text,QString command){auto a=menu_.addAction(text);connect(a,&QAction::triggered,this,[this,command]{action(command);});return a;};add(platform_->userVisible()?"Hide overlay":"Show overlay","visibility");auto lock=add("Lock / click-through","lock");lock->setCheckable(true);lock->setChecked(settings_["Locked"].toBool());add("Settings…","settings");add(settings_["ViewMode"]=="Compact"?"Switch to Full View":"Switch to Compact View","view");add(updater_.available()?"Update available…":"Check for updates…","updates");menu_.addSeparator();if(platform_->attached()){auto a=menu_.addAction("Attached: "+platform_->attachmentDescription());a->setEnabled(false);}add(platform_->selecting()?"Cancel window selection":platform_->attached()?"Change attached window…":"Glue to app…","glue");add("Unglue and show on desktop","recover");menu_.addSeparator();add("Quit GlassStats","quit");}
void Controller::showMenu(){if(settings_["Locked"].toBool())return;endDrag();buildMenu();menu_.popup(QCursor::pos());}
void Controller::action(QString command){emit dismissDetails();invalidate();if(command=="settings")emit openSettings();else if(command=="lock")setSetting("Locked",!settings_["Locked"].toBool());else if(command=="view")setSetting("ViewMode",settings_["ViewMode"]=="Compact"?"Full":"Compact");else if(command=="visibility")platform_->setUserVisible(!platform_->userVisible());else if(command=="show")platform_->setUserVisible(true);else if(command=="glue"){endDrag();if(platform_->selecting())platform_->cancelSelection();else platform_->beginSelection();}else if(command=="recover"){endDrag();platform_->cancelSelection();platform_->detach();settings_["Locked"]=false;platform_->setLocked(false);platform_->setUserVisible(true);fitWindow();saveTimer_.start();emit settingsChanged();emit nativeChanged();}else if(command=="updates"){emit openSettings();checkUpdates();}else if(command=="switchStartup"){auto error=platform_->setStartup(true,true);settings_["StartAtSignIn"]=error.isEmpty();setStatus(error.isEmpty()?"Sign-in startup switched to GlassStats Qt.":error);emit settingsChanged();emit nativeChanged();saveTimer_.start();}else if(command=="quit"){save();qApp->quit();}}
QString Controller::formatReading(QVariantMap r,int decimals)const{if(r["available"].toBool()&&r["value"].isValid())return QString::number(r["value"].toDouble(),'f',decimals);return r["status"]=="Stale"?"Stale":"—";}
QString Controller::memoryText(QString metric,QString part)const{auto used=metrics_.value(metric+"Used").toMap(),total=metrics_.value(metric+"Total").toMap();if(!used["available"].toBool()||!total["available"].toBool())return used["status"]=="Stale"?"Stale":"Unavailable";double u=used["value"].toDouble(),t=total["value"].toDouble();auto gb=[](double v){return QString::number(v/(1024.*1024*1024),'f',1);};if(part=="free")return gb(std::max(0.,t-u))+" GB free";return gb(u)+" / "+gb(t)+" GB";}
double Controller::memoryFraction(QString metric)const{auto used=metrics_.value(metric+"Used").toMap(),total=metrics_.value(metric+"Total").toMap();if(!used["available"].toBool()||!total["available"].toBool()||total["value"].toDouble()<=0)return -1;return std::clamp(used["value"].toDouble()/total["value"].toDouble(),0.,1.);}
void Controller::checkUpdates(bool manual){if(validation_)return;if(!manual){auto at=QDateTime::fromString(settings_["LastUpdateCheckUtc"].toString(),Qt::ISODateWithMs);if(at.isValid()&&at.secsTo(QDateTime::currentDateTimeUtc())<86400)return;}updater_.check(settings_["IncludeBetaUpdates"].toBool());}
void Controller::helper(bool install){setStatus(platform_->helperAction(install));}
void Controller::requestPermission(QString feature){setStatus(platform_->permissionAction(feature));emit nativeChanged();}
void Controller::openVerifiedInstaller(QString path){auto root=QStandardPaths::writableLocation(QStandardPaths::CacheLocation)+"/updates/";if(!QFileInfo(path).absoluteFilePath().startsWith(QFileInfo(root).absoluteFilePath()+"/")||!QFileInfo::exists(path)){setStatus("The verified installer is unavailable.");return;}
#ifdef Q_OS_LINUX
    QFile::setPermissions(path,QFile::permissions(path)|QFileDevice::ExeOwner);QDesktopServices::openUrl(QUrl::fromLocalFile(QFileInfo(path).absolutePath()));setStatus("Verified AppImage is ready in the opened folder. Quit this instance before running the new AppImage.");
#else
    QDesktopServices::openUrl(QUrl::fromLocalFile(path));
#endif
}
}

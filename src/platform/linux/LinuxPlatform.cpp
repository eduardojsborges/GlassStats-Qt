#include "LinuxPlatform.h"
#include <QFile>
#include <QDir>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QSaveFile>
#include <QCoreApplication>
#include <QGuiApplication>
#include <QLibrary>
#include <QSocketNotifier>
#include <QFileInfo>
#include <QRegion>
#include <cmath>
#ifdef GLASSSTATS_X11
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/keysym.h>
#include <X11/Xutil.h>
#undef Status
#endif

namespace gs {
namespace {
QByteArray readFile(QString path){QFile f(path);return f.open(QIODevice::ReadOnly)?f.readAll():QByteArray();}
std::optional<double> number(QString path){bool ok;double value=QString::fromUtf8(readFile(path)).trimmed().toDouble(&ok);return ok&&std::isfinite(value)?std::optional<double>(value):std::nullopt;}
class LinuxMonitor final:public MonitorProvider {
    quint64 total_=0,idle_=0;QLibrary nvml_{"libnvidia-ml.so.1"};bool nvmlReady_=false;
    using Init=int(*)();using Count=int(*)(unsigned*);using Handle=int(*)(unsigned,void**);using Name=int(*)(void*,char*,unsigned);using Util=int(*)(void*,void*);using Mem=int(*)(void*,void*);using Temp=int(*)(void*,unsigned,unsigned*);
    Count count_=nullptr;Handle handle_=nullptr;Name name_=nullptr;Util util_=nullptr;Mem memory_=nullptr;Temp temp_=nullptr;
public:
    LinuxMonitor(){if(nvml_.load()){auto init=reinterpret_cast<Init>(nvml_.resolve("nvmlInit_v2"));count_=reinterpret_cast<Count>(nvml_.resolve("nvmlDeviceGetCount_v2"));handle_=reinterpret_cast<Handle>(nvml_.resolve("nvmlDeviceGetHandleByIndex_v2"));name_=reinterpret_cast<Name>(nvml_.resolve("nvmlDeviceGetName"));util_=reinterpret_cast<Util>(nvml_.resolve("nvmlDeviceGetUtilizationRates"));memory_=reinterpret_cast<Mem>(nvml_.resolve("nvmlDeviceGetMemoryInfo"));temp_=reinterpret_cast<Temp>(nvml_.resolve("nvmlDeviceGetTemperature"));nvmlReady_=init&&count_&&handle_&&name_&&util_&&memory_&&temp_&&init()==0;}}
    ~LinuxMonitor()override{if(nvmlReady_){auto shutdown=reinterpret_cast<Init>(nvml_.resolve("nvmlShutdown"));if(shutdown)shutdown();}}
    Frame read(const QVariantMap& settings)override{Frame f;auto& s=f.sample;s.at=QDateTime::currentDateTimeUtc();auto cpu=readFile("/proc/stat").split('\n').first().simplified().split(' ');quint64 total=0,idle=0;if(cpu.size()>5){for(int i=1;i<std::min(9,qsizetype(cpu.size()));i++)total+=cpu[i].toULongLong();idle=cpu[4].toULongLong()+cpu[5].toULongLong();if(total_&&total>total_&&idle>=idle_)s.cpu=Reading::of(std::clamp(100.*(1.-double(idle-idle_)/(total-total_)),0.,100.),"%","/proc/stat CPU-time utilization");else s.cpu=Reading::missing("%","CPU counter is warming up",Availability::WarmingUp);total_=total;idle_=idle;}
        auto info=QString::fromUtf8(readFile("/proc/cpuinfo"));auto model=QRegularExpression("(?:model name|Hardware)\\s*:\\s*([^\\n]+)").match(info);s.cpuName=model.hasMatch()?model.captured(1).trimmed():"CPU";
        auto mem=QString::fromUtf8(readFile("/proc/meminfo"));auto kb=[&](QString key){auto m=QRegularExpression("(?:^|\\n)"+key+":\\s*(\\d+)").match(mem);return m.hasMatch()?m.captured(1).toDouble()*1024:-1;};double t=kb("MemTotal"),available=kb("MemAvailable");if(t>0&&available>=0){s.ramTotal=Reading::of(t,"bytes");s.ramUsed=Reading::of(t-available,"bytes","MemTotal minus MemAvailable");}else{s.ramUsed=Reading::missing("bytes","/proc/meminfo unavailable");s.ramTotal=s.ramUsed;}
        struct Gpu{Adapter adapter;Reading usage,used,total,temp;};QVector<Gpu> devices;
        if(nvmlReady_){unsigned count=0;if(count_(&count)==0)for(unsigned i=0;i<count;i++){void* h=nullptr;char name[128]{};if(handle_(i,&h)||name_(h,name,128))continue;Gpu g;g.adapter={"nvml:"+QString::number(i),QString::fromUtf8(name),"nvml:"+QString::fromUtf8(name)+":"+QString::number(i),0,true};struct Rates{unsigned gpu,memory;}r{};if(util_(h,&r)==0)g.usage=Reading::of(r.gpu,"%","NVML GPU utilization");else g.usage=Reading::missing("%","NVML utilization unavailable");struct Memory{unsigned long long total,free,used;}m{};if(memory_(h,&m)==0){g.used=Reading::of(double(m.used),"bytes","NVML dedicated memory");g.total=Reading::of(double(m.total),"bytes");g.adapter.dedicatedBytes=m.total;}unsigned c=0;if(temp_(h,0,&c)==0){g.temp=Reading::of(c,"°C");f.sensors<<Sensor{g.adapter.id+":core",g.adapter.name,"GPU Core","GPU",double(c)};}devices<<g;}}
        QDir drm("/sys/class/drm");for(auto card:drm.entryList(QDir::Dirs|QDir::NoDotAndDotDot)){if(!QRegularExpression("^card\\d+$").match(card).hasMatch())continue;QString base=drm.filePath(card)+"/device";QString vendor=QString::fromUtf8(readFile(base+"/vendor")).trimmed();if(vendor=="0x10de"&&nvmlReady_)continue;QString vendorName=vendor=="0x1002"?"AMD":vendor=="0x8086"?"Intel":vendor=="0x10de"?"NVIDIA":"GPU";Gpu g;QString id=QFileInfo(base).canonicalFilePath();g.adapter={id,vendorName+" "+card,id,0,false};if(auto value=number(base+"/gpu_busy_percent"))g.usage=Reading::of(*value,"%","DRM gpu_busy_percent");else g.usage=Reading::missing("%","This DRM driver does not expose aggregate GPU utilization");if(auto value=number(base+"/mem_info_vram_total")){g.total=Reading::of(*value,"bytes","DRM dedicated capacity");g.adapter.dedicatedBytes=quint64(*value);g.adapter.discrete=*value>512ull*1024*1024;}else g.total=Reading::missing("bytes","No dedicated VRAM counter");if(auto value=number(base+"/mem_info_vram_used"))g.used=Reading::of(*value,"bytes","DRM dedicated memory");else g.used=Reading::missing("bytes","DRM memory usage unavailable");QDir hw(base+"/hwmon");for(auto device:hw.entryList(QDir::Dirs|QDir::NoDotAndDotDot)){QDir d(hw.filePath(device));for(auto file:d.entryList({"temp*_input"},QDir::Files)){auto v=number(d.filePath(file));if(!v)continue;QString label=file;label.replace("_input","_label");QString name=QString::fromUtf8(readFile(d.filePath(label))).trimmed();if(name.isEmpty())name="GPU Core";f.sensors<<Sensor{d.filePath(file),g.adapter.name,name,"GPU",*v/1000.};}}devices<<g;}
        QDir hw("/sys/class/hwmon");for(auto device:hw.entryList(QDir::Dirs|QDir::NoDotAndDotDot)){QDir d(hw.filePath(device));QString name=QString::fromUtf8(readFile(d.filePath("name"))).trimmed();if(!QStringList{"coretemp","k10temp","zenpower","cpu_thermal"}.contains(name))continue;for(auto file:d.entryList({"temp*_input"},QDir::Files)){auto v=number(d.filePath(file));if(!v)continue;QString label=file;label.replace("_input","_label");QString text=QString::fromUtf8(readFile(d.filePath(label))).trimmed();f.sensors<<Sensor{d.filePath(file),s.cpuName,text.isEmpty()?"CPU Package":text,"CPU",*v/1000.};}}
        s.gpu=Reading::missing("%","No supported GPU driver detected");s.vramUsed=Reading::missing("bytes","Dedicated VRAM usage unavailable");s.vramTotal=Reading::missing("bytes","Dedicated VRAM capacity unavailable");int index=0;for(int i=0;i<devices.size();i++){f.adapters<<devices[i].adapter;if(devices[i].adapter.id==settings["GpuId"].toString()||devices[i].adapter.persistentId==settings["GpuId"].toString())index=i;}if(!devices.isEmpty()){auto g=devices[index];s.gpuId=g.adapter.id;s.gpuName=g.adapter.name;s.gpu=g.usage;s.vramUsed=g.used;s.vramTotal=g.total;}
        f.sensorsAt=s.at;s.cpuTemperature=temperature(f.sensors,s.at,"CPU",{},settings["CpuSensorId"].toString(),s.at);s.gpuTemperature=temperature(f.sensors,s.at,"GPU",s.gpuName,settings["GpuSensorId"].toString(),s.at);return f;
    }
};
class XCapture final:public SurfaceCapture {
public:SurfaceFrame capture(const CaptureRequest& request)override{
#ifdef GLASSSTATS_X11
    if(QGuiApplication::platformName()=="xcb"&&qEnvironmentVariableIsSet("GLASSSTATS_EXPERIMENTAL_CAPTURE")){Display* d=XOpenDisplay(nullptr);if(d){auto image=XGetImage(d,DefaultRootWindow(d),request.card.x(),request.card.y(),unsigned(request.card.width()),unsigned(request.card.height()),AllPlanes,ZPixmap);if(image){QImage buffer(request.card.size(),QImage::Format_RGB32);for(int y=0;y<buffer.height();y++)for(int x=0;x<buffer.width();x++){auto pixel=XGetPixel(image,x,y);buffer.setPixel(x,y,qRgb((pixel&image->red_mask)>>16,(pixel&image->green_mask)>>8,pixel&image->blue_mask));}XDestroyImage(image);XCloseDisplay(d);return analyzeSurface(request,buffer);}XCloseDisplay(d);}}
#endif
    return {request.generation,request.card,{}};}
};
}
struct LinuxPlatform::Native {
    bool x11=false,enabled=false;QTimer selectionTimer;QPointF relative;QRect host;
#ifdef GLASSSTATS_X11
    Display* display=nullptr;Window target=0,overlay=0;unsigned long pid=0;QString start;
    std::unique_ptr<QSocketNotifier> notifier;
    Atom atom(const char* name)const{return XInternAtom(display,name,False);}
    QVector<unsigned long> property(Window w,const char* key,Atom type=XA_CARDINAL)const{Atom actual;int format;unsigned long count,remaining;unsigned char* bytes=nullptr;QVector<unsigned long> out;if(XGetWindowProperty(display,w,atom(key),0,65536,False,type,&actual,&format,&count,&remaining,&bytes)==Success&&bytes&&format==32){auto values=reinterpret_cast<unsigned long*>(bytes);for(unsigned long i=0;i<count;i++)out<<values[i];}if(bytes)XFree(bytes);return out;}
    QRect area(Window w)const{XWindowAttributes a{};if(!XGetWindowAttributes(display,w,&a)||a.map_state!=IsViewable)return {};Window child;int x=0,y=0;XTranslateCoordinates(display,w,DefaultRootWindow(display),0,0,&x,&y,&child);return {x,y,a.width,a.height};}
    QString identity(unsigned long process)const{auto parts=QString::fromUtf8(readFile("/proc/"+QString::number(process)+"/stat"));int end=parts.lastIndexOf(')');return parts.mid(end+2).split(' ').value(19);}
    Window frame(Window w)const{Window root,parent,*children=nullptr;unsigned count=0;while(XQueryTree(display,w,&root,&parent,&children,&count)){if(children)XFree(children);children=nullptr;if(!parent||parent==root)break;w=parent;}return w;}
#endif
};
LinuxPlatform::LinuxPlatform():native_(std::make_unique<Native>()){
#ifdef GLASSSTATS_X11
    native_->x11=QGuiApplication::platformName()=="xcb";native_->enabled=native_->x11&&qEnvironmentVariableIsSet("GLASSSTATS_EXPERIMENTAL_WINDOWING");if(native_->x11){native_->display=XOpenDisplay(nullptr);if(native_->display){XSetErrorHandler([](Display*,XErrorEvent*){return 0;});native_->notifier=std::make_unique<QSocketNotifier>(ConnectionNumber(native_->display),QSocketNotifier::Read);connect(native_->notifier.get(),&QSocketNotifier::activated,this,[this]{while(XPending(native_->display)){XEvent e;XNextEvent(native_->display,&e);if(e.type==DestroyNotify&&e.xdestroywindow.window==native_->target){detach();emit targetLost();}else if(e.type==KeyPress){auto sym=XLookupKeysym(&e.xkey,0);if(selecting_&&sym==XK_Escape)cancelSelection();else if(!selecting_)emit requestCommand(sym==XK_u||sym==XK_U?"recover":"lock");}else if(selecting_&&e.type==ButtonRelease){auto p=cursor();Window root=DefaultRootWindow(native_->display),child=0,current=root;int rx,ry,wx,wy;unsigned mask;while(XQueryPointer(native_->display,current,&root,&child,&rx,&ry,&wx,&wy,&mask)&&child){current=child;}Window chosen=current;while(chosen&&chosen!=root&&native_->property(chosen,"_NET_WM_PID").isEmpty()){Window parent,*children=nullptr;unsigned count;XQueryTree(native_->display,chosen,&root,&parent,&children,&count);if(children)XFree(children);chosen=parent;}QString reason;if(chosen!=root&&attachTo(chosen,p,&reason))cancelSelection();else emit status(reason.isEmpty()?"Choose an application window.":reason);}else reconcile();}});}}
#endif
    native_->selectionTimer.setSingleShot(true);connect(&native_->selectionTimer,&QTimer::timeout,this,&LinuxPlatform::cancelSelection);
}
LinuxPlatform::~LinuxPlatform(){cancelSelection();
#ifdef GLASSSTATS_X11
if(native_->display){XUngrabKey(native_->display,AnyKey,AnyModifier,DefaultRootWindow(native_->display));native_->notifier.reset();XCloseDisplay(native_->display);}
#endif
}
Capabilities LinuxPlatform::capabilities()const{return {{"Monitoring",{"unverified","Native /proc, hwmon, DRM and optional NVML; hardware testing required"}},{"Attachment",{native_->enabled?"unverified":"unavailable",native_->x11?"X11 attachment exists behind GLASSSTATS_EXPERIMENTAL_WINDOWING=1; real-desktop validation pending":"Wayland requires a tested compositor integration; standard Qt positioning is insufficient"}},{"Adaptive text",{"unverified","X11 capture requires GLASSSTATS_EXPERIMENTAL_CAPTURE=1; other sessions use glyph halos"}},{"Global shortcuts",{"unverified",native_->x11?"X11 key grabs; desktop conflicts require manual testing":"Global shortcut portal integration is not validated; use recovery from the tray or relaunch"}},{"Backdrop",{"unavailable","Translucent tint fallback; compositor-specific native blur is not validated"}}};}
void LinuxPlatform::initialize(QWindow* w){Platform::initialize(w);
#ifdef GLASSSTATS_X11
    if(native_->display)native_->overlay=Window(w->winId());
#endif
}
void LinuxPlatform::setLocked(bool value){Platform::setLocked(value);
#ifdef GLASSSTATS_X11
    if(native_->display)native_->overlay=Window(window_->winId());
#endif
}
QString LinuxPlatform::attachmentDescription()const{
#ifdef GLASSSTATS_X11
if(attached_){char* name=nullptr;QString title="Attached X11 window";if(XFetchName(native_->display,native_->target,&name)&&name){title=QString::fromUtf8(name);XFree(name);}return title;}
#endif
return "Desktop overlay";}
bool LinuxPlatform::attachTo(quintptr handle,QPoint anchor,QString* reason){
#ifdef GLASSSTATS_X11
    if(native_->enabled&&native_->display){auto target=Window(handle);auto ids=native_->property(target,"_NET_WM_PID");QRect area=native_->area(target);if(target==native_->overlay||ids.isEmpty()||ids[0]==unsigned(QCoreApplication::applicationPid())||area.isEmpty()){if(reason)*reason="Choose an accessible application window outside GlassStats.";return false;}native_->target=target;native_->pid=ids[0];native_->start=native_->identity(ids[0]);if(native_->start.isEmpty()){if(reason)*reason="Target identity is inaccessible";return false;}native_->relative=relativeAnchor(area,anchor);native_->host=area;XSelectInput(native_->display,target,StructureNotifyMask|PropertyChangeMask);attached_=true;userVisible_=true;setLocked(false);fitCurrent();emit changed();return true;}
#endif
    return Platform::attachTo(handle,anchor,reason);}
void LinuxPlatform::reconcile(){
#ifdef GLASSSTATS_X11
if(attached_){auto ids=native_->property(native_->target,"_NET_WM_PID");if(ids.isEmpty()||ids[0]!=native_->pid||native_->identity(ids[0])!=native_->start){detach();emit targetLost();return;}auto area=native_->area(native_->target);if(area.isEmpty()){if(window_)window_->hide();emit changed();return;}if(!dragging_&&area!=native_->host){native_->host=area;auto m=selectMonitor(monitors(),absoluteAnchor(area,native_->relative));place(fitHost(area,absoluteAnchor(area,native_->relative),logical_,scale_*m.dpi/96,radius_));}XWindowChanges changes{};changes.sibling=native_->frame(native_->target);changes.stack_mode=Above;XConfigureWindow(native_->display,native_->frame(native_->overlay),CWSibling|CWStackMode,&changes);XFlush(native_->display);if(window_)window_->setVisible(userVisible_&&!selecting_);emit changed();}
#endif
}
void LinuxPlatform::fitCurrent(){
#ifdef GLASSSTATS_X11
if(attached_){auto area=native_->area(native_->target);if(area.isEmpty())return;auto b=bounds();auto m=selectMonitor(monitors(),b.topRight());place(fitHost(area,{b.x()+b.width(),b.y()},logical_,scale_*m.dpi/96,radius_));return;}
#endif
Platform::fitCurrent();}
void LinuxPlatform::endDrag(){Platform::endDrag();
#ifdef GLASSSTATS_X11
if(attached_){auto b=bounds();native_->relative=relativeAnchor(native_->area(native_->target),{b.x()+b.width(),b.y()});}
#endif
}
void LinuxPlatform::detach(){
#ifdef GLASSSTATS_X11
native_->target=0;
#endif
Platform::detach();if(window_)window_->setFlag(Qt::WindowStaysOnTopHint,true);}
void LinuxPlatform::beginSelection(){
#ifdef GLASSSTATS_X11
if(native_->enabled&&native_->display){auto root=DefaultRootWindow(native_->display);if(XGrabPointer(native_->display,root,False,ButtonPressMask|ButtonReleaseMask,GrabModeAsync,GrabModeAsync,0,0,CurrentTime)!=GrabSuccess){emit status("X11 pointer selection is busy.");return;}XGrabKeyboard(native_->display,root,False,GrabModeAsync,GrabModeAsync,CurrentTime);selecting_=true;if(window_)window_->hide();native_->selectionTimer.start(30000);emit status("Click a window to attach · Esc to cancel (experimental X11)");emit changed();return;}
#endif
Platform::beginSelection();}
void LinuxPlatform::cancelSelection(){native_->selectionTimer.stop();
#ifdef GLASSSTATS_X11
if(selecting_&&native_->display){XUngrabPointer(native_->display,CurrentTime);XUngrabKeyboard(native_->display,CurrentTime);XFlush(native_->display);}
#endif
Platform::cancelSelection();reconcile();}
QVector<QRect> LinuxPlatform::coveredAreas()const{QVector<QRect> list;
#ifdef GLASSSTATS_X11
if(native_->display){auto windows=native_->property(DefaultRootWindow(native_->display),"_NET_CLIENT_LIST_STACKING",XA_WINDOW);auto own=windows.indexOf(native_->overlay);if(own>=0)for(int i=own+1;i<windows.size();i++){auto area=native_->area(windows[i]);if(area.intersects(bounds()))list<<area;}}
#endif
return list;}
bool LinuxPlatform::canHover()const{
#ifdef GLASSSTATS_X11
if(attached_){auto active=native_->property(DefaultRootWindow(native_->display),"_NET_ACTIVE_WINDOW",XA_WINDOW);if(active.isEmpty()||active[0]!=native_->target)return false;}
#endif
return Platform::canHover();}
QString LinuxPlatform::registerShortcuts(QString lock,QString recover,bool validation){
#ifdef GLASSSTATS_X11
if(native_->display){auto root=DefaultRootWindow(native_->display);XUngrabKey(native_->display,AnyKey,AnyModifier,root);if(validation)return {};for(auto text:{lock,recover}){auto key=parseShortcut(text);if(!key)return "Invalid shortcut";unsigned mods=0;if(key->modifiers&2)mods|=ControlMask;if(key->modifiers&1)mods|=Mod1Mask;if(key->modifiers&4)mods|=ShiftMask;if(key->modifiers&8)mods|=Mod4Mask;auto code=XKeysymToKeycode(native_->display,KeySym(key->key));for(unsigned extra:{0u,unsigned(LockMask),unsigned(Mod2Mask),unsigned(LockMask|Mod2Mask)})XGrabKey(native_->display,code,mods|extra,root,False,GrabModeAsync,GrabModeAsync);}XFlush(native_->display);return "X11 shortcuts are experimental; verify registration and keep tray recovery available.";}
#endif
return Platform::registerShortcuts(lock,recover,validation);}
bool LinuxPlatform::startupEnabled()const{return QFileInfo::exists(QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation)+"/autostart/glassstats-qt.desktop");}
QString LinuxPlatform::setStartup(bool enabled,bool){auto path=QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation)+"/autostart/glassstats-qt.desktop";if(!enabled){QFile::remove(path);return {};}auto executable=qEnvironmentVariable("APPIMAGE",QCoreApplication::applicationFilePath());if(executable.contains('\n')||executable.contains('\r'))return "Invalid application path";executable.replace("\\","\\\\");executable.replace("\"","\\\"");QDir().mkpath(QFileInfo(path).absolutePath());QSaveFile f(path);if(!f.open(QIODevice::WriteOnly))return f.errorString();f.write(("[Desktop Entry]\nType=Application\nName=GlassStats\nExec=\""+executable+"\" --startup\nTerminal=false\nX-GNOME-Autostart-enabled=true\n").toUtf8());return f.commit()?QString():f.errorString();}
std::unique_ptr<Platform> createPlatform(){return std::make_unique<LinuxPlatform>();}
std::unique_ptr<MonitorProvider> createMonitorProvider(){return std::make_unique<LinuxMonitor>();}
std::unique_ptr<SurfaceCapture> createSurfaceCapture(){return std::make_unique<XCapture>();}
}

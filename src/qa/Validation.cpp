#include "Validation.h"
#include "app/Controller.h"
#include <QApplication>
#include <QWidget>
#include <QPainter>
#include <QTimer>
#include <QProcess>
#include <QSaveFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDir>
#include <QElapsedTimer>
#include <QQuickItem>
#include <QThread>
#ifdef Q_OS_WIN
#include <windows.h>
#include <psapi.h>
#endif

namespace gs {
namespace {
void write(QString path,QJsonObject object){QSaveFile f(path);if(f.open(QIODevice::WriteOnly)){f.write(QJsonDocument(object).toJson());f.commit();}}
void screenCapture(Controller* app,QString path){
#ifdef Q_OS_WIN
    auto r=app->platform()->bounds();HDC screen=GetDC(nullptr),memory=CreateCompatibleDC(screen);BITMAPINFO info{};info.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);info.bmiHeader.biWidth=r.width();info.bmiHeader.biHeight=-r.height();info.bmiHeader.biPlanes=1;info.bmiHeader.biBitCount=32;void* bits=nullptr;auto bitmap=CreateDIBSection(screen,&info,DIB_RGB_COLORS,&bits,nullptr,0);if(bitmap){auto old=SelectObject(memory,bitmap);if(BitBlt(memory,0,0,r.width(),r.height(),screen,r.x(),r.y(),SRCCOPY|CAPTUREBLT))QImage(static_cast<uchar*>(bits),r.width(),r.height(),r.width()*4,QImage::Format_RGB32).save(path);SelectObject(memory,old);DeleteObject(bitmap);}DeleteDC(memory);ReleaseDC(nullptr,screen);
#else
    Q_UNUSED(app);Q_UNUSED(path);
#endif
}
class HostWindow:public QWidget {
public:
    QWidget* cover=nullptr;int background=0;
    HostWindow(){setWindowTitle("GlassStats Qt QA host");setGeometry(180,160,1100,650);cover=new QWidget;cover->setWindowTitle("GlassStats Qt QA cover");cover->setStyleSheet("background:#52617d;");}
    ~HostWindow()override{delete cover;}
    void paintEvent(QPaintEvent*)override{QPainter p(this);p.fillRect(rect(),background==1?Qt::black:Qt::white);if(background==2)p.fillRect(QRect(width()/2,0,width()/2,height()),Qt::black);if(background==3){for(int y=0;y<height();y+=20)for(int x=0;x<width();x+=20)if((x/20+y/20)%2)p.fillRect(x,y,20,20,QColor("#161b2b"));}}
#ifdef Q_OS_WIN
    bool nativeEvent(const QByteArray&,void* message,qintptr* result)override{auto m=static_cast<MSG*>(message);if(m->message>=0x8101&&m->message<=0x8110){switch(m->message){case 0x8101:showNormal();setGeometry(220,180,900,560);break;case 0x8102:showMinimized();break;case 0x8103:showNormal();show();raise();activateWindow();break;case 0x8104:hide();break;case 0x8105:show();break;case 0x8106:cover->setGeometry(frameGeometry());cover->show();cover->raise();cover->activateWindow();break;case 0x8107:cover->setGeometry(x(),y(),width()/2,height());cover->show();cover->raise();cover->activateWindow();break;case 0x8108:cover->hide();raise();activateWindow();break;case 0x8109:background=1;update();break;case 0x810A:background=0;update();break;case 0x810B:background=2;update();break;case 0x810C:QApplication::quit();break;case 0x810D:setWindowTitle("GlassStats renamed QA host");break;case 0x810E:setGeometry(100,100,160,130);break;case 0x810F:background=3;update();break;}*result=0;return true;}return false;}
#endif
};
quint64 handleFrom(QString path){QFile f(path);if(!f.open(QIODevice::ReadOnly))return 0;return QJsonDocument::fromJson(f.readAll()).object()["Host"].toVariant().toULongLong();}
struct Usage{double cpu=0;quint64 privateBytes=0,workingSet=0,handles=0,gdi=0;};
Usage resources(){Usage u;
#ifdef Q_OS_WIN
    FILETIME c,e,k,v;if(GetProcessTimes(GetCurrentProcess(),&c,&e,&k,&v))u.cpu=(double((quint64(k.dwHighDateTime)<<32)|k.dwLowDateTime)+double((quint64(v.dwHighDateTime)<<32)|v.dwLowDateTime))/1e7;PROCESS_MEMORY_COUNTERS_EX memory{};if(GetProcessMemoryInfo(GetCurrentProcess(),reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&memory),sizeof memory)){u.privateBytes=memory.PrivateUsage;u.workingSet=memory.WorkingSetSize;}DWORD count=0;GetProcessHandleCount(GetCurrentProcess(),&count);u.handles=count;u.gdi=GetGuiResources(GetCurrentProcess(),GR_GDIOBJECTS);
#endif
    return u;}
}
int runTestHost(QString directory){QDir().mkpath(directory);HostWindow host;host.show();QTimer::singleShot(100,[&]{
#ifdef Q_OS_WIN
    ShowWindow(reinterpret_cast<HWND>(host.winId()),SW_SHOWNORMAL);SetForegroundWindow(reinterpret_cast<HWND>(host.winId()));
#endif
    write(directory+"/host.json",{{"Host",QString::number(quintptr(host.winId()))},{"Cover",QString::number(quintptr(host.cover->winId()))},{"Pid",QCoreApplication::applicationPid()}});});return QApplication::exec();}
void runValidation(Controller* app,QQuickWindow* window,QString directory,bool soak){
#ifdef Q_OS_WIN
    SetThreadExecutionState(ES_CONTINUOUS|ES_DISPLAY_REQUIRED|ES_SYSTEM_REQUIRED);
    DWORD_PTR reply=0;SendMessageTimeoutW(HWND_BROADCAST,WM_SYSCOMMAND,SC_MONITORPOWER,-1,SMTO_ABORTIFHUNG,200,&reply);
    QObject::connect(qApp,&QCoreApplication::aboutToQuit,qApp,[]{SetThreadExecutionState(ES_CONTINUOUS);});
#endif
if(directory.isEmpty())directory=QDir::currentPath()+"/artifacts/qa/"+(soak?"stability":"smoke");QDir().mkpath(directory);auto fixture=new QProcess(app);fixture->setProgram(QCoreApplication::applicationFilePath());fixture->setArguments({"--test-host","--qa-dir",directory});fixture->start();auto timer=new QTimer(app);timer->setInterval(250);auto wait=std::make_shared<QElapsedTimer>();wait->start();QObject::connect(timer,&QTimer::timeout,app,[=]{auto handle=handleFrom(directory+"/host.json");if(!handle&&wait->elapsed()<10000)return;timer->stop();if(!handle){write(directory+"/result.json",{{"Passed",false},{"Error","QA host did not start"}});QCoreApplication::exit(1);return;}auto command=[handle](int cmd){
#ifdef Q_OS_WIN
PostMessageW(reinterpret_cast<HWND>(handle),UINT(cmd),0,0);
#else
Q_UNUSED(handle);Q_UNUSED(cmd);
#endif
};
        if(soak){bool ok=false;QString reason;app->setSetting("ProtectContrast",false);app->setSetting("Opacity",0.);app->setSetting("AutoTextContrast",true);ok=app->platform()->attachTo(handle,{700,240},&reason);auto elapsed=std::make_shared<QElapsedTimer>();elapsed->start();auto start=resources();auto rows=std::make_shared<QJsonArray>();double duration=qEnvironmentVariable("GLASSSTATS_SOAK_MINUTES","120").toDouble();auto sample=new QTimer(app);sample->setInterval(30000);auto background=std::make_shared<int>(0);QObject::connect(sample,&QTimer::timeout,app,[=]{auto r=resources();double minutes=elapsed->elapsed()/60000.;int processors=std::max(1,QThread::idealThreadCount());double cpu=(r.cpu-start.cpu)/std::max(.001,elapsed->elapsed()/1000.)/processors*100;QJsonObject row{{"Minutes",minutes},{"CpuPercent",cpu},{"PrivateBytes",double(r.privateBytes)},{"WorkingSet",double(r.workingSet)},{"Handles",double(r.handles)},{"GdiObjects",double(r.gdi)},{"AdaptiveFrames",double(app->adaptiveFrames())},{"Attached",app->platform()->attached()},{"SampleAgeSeconds",app->frame().sample.at.msecsTo(QDateTime::currentDateTimeUtc())/1000.}};rows->append(row);write(directory+"/soak-progress.json",{{"Rows",*rows}});++*background;command(*background%4==0?0x810A:*background%4==1?0x8109:*background%4==2?0x810B:0x810F);app->setSetting("ViewMode",*background%2?"Compact":"Full");if(minutes>=duration){sample->stop();write(directory+"/soak-result.json",{{"Complete",true},{"Minutes",minutes},{"AttachedInitially",ok},{"AttachError",reason},{"Rows",*rows}});command(0x810C);QCoreApplication::exit(ok?0:1);}});sample->start();write(directory+"/run.json",{{"StartedUtc",QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs)},{"DurationMinutes",duration},{"AppPid",QCoreApplication::applicationPid()},{"HostPid",fixture->processId()},{"Attached",ok},{"AttachError",reason}});return;}
        struct Step{QString name;int delay;std::function<bool()> run;};auto steps=std::make_shared<QVector<Step>>();auto results=std::make_shared<QJsonObject>();auto index=std::make_shared<int>(0);auto next=std::make_shared<std::function<void()>>();
        steps->push_back({"RealMetrics",1800,[=]{return app->frame().sample.cpu.value.has_value()&&app->frame().sample.ramTotal.value.has_value();}});
        steps->push_back({"Attach",200,[=]{QString reason;return app->platform()->attachTo(handle,{650,220},&reason);}});
        steps->push_back({"OpacityUnchanged",500,[=]{app->setSetting("ProtectContrast",false);app->setSetting("Opacity",0.);return app->settings()["Opacity"].toDouble()==0;}});
        steps->push_back({"AdaptiveFrames",1800,[=]{window->grabWindow().save(directory+"/full-white.png");screenCapture(app,directory+"/screen-full-white.png");return app->adaptiveFrames()>0;}});
        steps->push_back({"WhiteTextReadable",400,[=]{for(auto id:{"header","cpu.full.value","gpu.full.value","ram.full.total","vram.full.percent","full.settings","full.view"}){auto v=app->textColors().value(id).toMap();if(v.isEmpty()||contrast(v["color"].value<QColor>(),Qt::white)<4.5)return false;}return true;}});
        steps->push_back({"Compact",300,[=]{auto b=app->platform()->bounds();app->setSetting("ViewMode","Compact");Q_UNUSED(b);return true;}});
        steps->push_back({"CompactFits",700,[=]{auto b=app->platform()->bounds();window->grabWindow().save(directory+"/compact-white.png");screenCapture(app,directory+"/screen-compact-white.png");return b.width()>b.height()&&b.height()>20;}});
        steps->push_back({"CompactWhiteTextReadable",400,[=]{for(auto id:{"cpu.compact.value","gpu.compact.temperature","ram.compact.value","vram.compact.value","compact.settings","compact.view"}){auto v=app->textColors().value(id).toMap();if(v.isEmpty()||contrast(v["color"].value<QColor>(),Qt::white)<4.5)return false;}return true;}});
        steps->push_back({"FahrenheitLargeFonts",200,[=]{app->setSetting("TemperatureUnit","F");app->setSetting("CompactValueFontSize",48);app->setSetting("CompactDetailFontSize",24);return true;}});
        steps->push_back({"LargeFontsFit",500,[=]{auto b=app->platform()->bounds();return b.width()>100&&b.height()>30;}});
        steps->push_back({"HiddenMetrics",200,[=]{app->setSetting("ShowCpu",false);app->setSetting("ShowGpu",false);app->setSetting("ShowRam",false);app->setSetting("ShowVram",false);return true;}});
        steps->push_back({"AllHiddenSafe",400,[=]{auto b=app->platform()->bounds();app->resetDefaults();return b.width()>30;}});
        steps->push_back({"Reattach",200,[=]{QString reason;return app->platform()->attachTo(handle,{650,220},&reason);}});
        steps->push_back({"MoveResize",300,[=]{command(0x8101);return true;}});
        steps->push_back({"MoveResizeFits",700,[=]{auto b=app->platform()->bounds();return b.x()>=200&&b.y()>=180;}});
        steps->push_back({"Minimize",100,[=]{command(0x8102);return true;}});
        steps->push_back({"MinimizeHides",600,[=]{return !app->platform()->effectiveVisible();}});
        steps->push_back({"Restore",100,[=]{command(0x8103);return true;}});
        steps->push_back({"RestoreShows",600,[=]{return app->platform()->effectiveVisible();}});
        steps->push_back({"Cover",200,[=]{command(0x8106);return true;}});
        steps->push_back({"CoverDetected",600,[=]{return fullyCovered(app->platform()->bounds(),app->platform()->coveredAreas());}});
        steps->push_back({"Uncover",100,[=]{command(0x8108);command(0x8109);return true;}});
        steps->push_back({"DarkCapture",800,[=]{window->grabWindow().save(directory+"/full-dark.png");screenCapture(app,directory+"/screen-full-dark.png");return true;}});
        steps->push_back({"TinyHost",100,[=]{command(0x810E);return true;}});
        steps->push_back({"TinyHostFits",700,[=]{auto b=app->platform()->bounds();return b.width()<=160&&b.height()<=130;}});
        steps->push_back({"LockedRecovery",300,[=]{app->setSetting("Locked",true);app->action("recover");return !app->platform()->attached()&&!app->platform()->locked()&&app->platform()->userVisible();}});
        steps->push_back({"SelectionCancel",200,[=]{app->action("glue");bool selecting=app->platform()->selecting();app->action("glue");return selecting&&!app->platform()->selecting();}});
        steps->push_back({"RestoreHost",100,[=]{command(0x8101);return true;}});
        steps->push_back({"TargetClosure",400,[=]{QString reason;bool attached=app->platform()->attachTo(handle,{650,220},&reason);command(0x810C);return attached;}});
        steps->push_back({"TargetClosureRecovers",1200,[=]{return !app->platform()->attached()&&app->platform()->userVisible();}});
        *next=[=]{if(*index>=steps->size()){bool passed=true;for(auto it=results->begin();it!=results->end();++it)if(!it.value().toBool())passed=false;write(directory+"/result.json",{{"Passed",passed},{"Version",app->version()},{"Checks",*results},{"AdaptiveFrames",double(app->adaptiveFrames())}});command(0x810C);QTimer::singleShot(0,app,[=]{*next={};QCoreApplication::exit(passed?0:1);});return;}auto step=steps->at((*index)++);QTimer::singleShot(step.delay,app,[=]{bool ok=false;try{ok=step.run();}catch(...){ok=false;}results->insert(step.name,ok);(*next)();});};(*next)();
    });timer->start();
}
}

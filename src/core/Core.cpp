#include "Core.h"
#include <QFile>
#include <QSaveFile>
#include <QDir>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonArray>
#include <QRegularExpression>
#include <QUrl>
#include <algorithm>
#include <cmath>
#include <limits>

namespace gs {
Reading Reading::of(double v, QString u, QString d) { return std::isfinite(v) ? Reading{v,u,d,Availability::Available} : missing(u,d); }
Reading Reading::missing(QString u, QString d, Availability s) { return {{},u,d,s}; }
QVariantMap Reading::variant() const {
    static const QStringList names = {"Available","Warming up","Unavailable","Stale"};
    return {{"value",value ? QVariant(*value) : QVariant()},{"unit",unit},{"status",names[int(status)]},{"detail",detail},{"available",status==Availability::Available && value.has_value()}};
}
QVariantMap Sample::variant(const QString& units) const {
    auto temp = [&](Reading reading) { if(units=="F") { if(reading.value) reading.value=*reading.value*1.8+32; reading.unit="°F"; } return reading.variant(); };
    return {{"timestamp",at.toString(Qt::ISODateWithMs)},{"cpuName",cpuName},{"gpuName",gpuName},{"gpuId",gpuId},{"memoryModel",memoryModel},
        {"cpu",cpu.variant()},{"gpu",gpu.variant()},{"ramUsed",ramUsed.variant()},{"ramTotal",ramTotal.variant()},{"vramUsed",vramUsed.variant()},{"vramTotal",vramTotal.variant()},
        {"cpuTemperature",temp(cpuTemperature)},{"gpuTemperature",temp(gpuTemperature)}};
}
double luminance(QColor c) { auto ch=[](double x) { return x<=.04045 ? x/12.92 : std::pow((x+.055)/1.055,2.4); }; return .2126*ch(c.redF())+.7152*ch(c.greenF())+.0722*ch(c.blueF()); }
double contrast(QColor a,QColor b) { double x=luminance(a),y=luminance(b); return (std::max(x,y)+.05)/(std::min(x,y)+.05); }
QColor blend(QColor a,QColor b,double t) { return QColor(qRound(a.red()*(1-t)+b.red()*t),qRound(a.green()*(1-t)+b.green()*t),qRound(a.blue()*(1-t)+b.blue()*t)); }
double worstContrast(QColor text,QColor surface,double alpha) { auto lo=blend(Qt::black,surface,alpha), hi=blend(Qt::white,surface,alpha); double l=luminance(text); if(l>=luminance(lo)&&l<=luminance(hi)) return 1; return std::min(contrast(text,lo),contrast(text,hi)); }
Palette palette(const QVariantMap& s,bool dark) {
    QString theme=s.value("Theme").toString(); if(theme!="System") dark=theme=="Dark";
    QColor surface(s.value("BackgroundColor").toString()),text(s.value("TextColor").toString());
    if(!surface.isValid()) surface=dark?QColor(18,17,27):QColor(238,241,248);
    if(!text.isValid()) text=luminance(surface)>.35?QColor(24,29,39):QColor(245,247,255);
    QColor secondary=blend(text,surface,.16); double opacity=s.value("Opacity").toDouble();
    if(!s.value("ProtectContrast").toBool()) return {surface,text,secondary,opacity};
    QColor neutral=contrast(text,Qt::black)>contrast(text,Qt::white)?Qt::black:Qt::white,original=surface;
    for(int i=0;i<=255;i++) { surface=blend(original,neutral,i/255.); if(contrast(text,surface)>=4.55) break; }
    secondary=blend(text,surface,.16); if(contrast(secondary,surface)<4.55) secondary=text;
    double minimum=1; for(int i=0;i<=255;i++) if(worstContrast(text,surface,i/255.)>=4.55 && worstContrast(secondary,surface,i/255.)>=4.55) {minimum=i/255.;break;}
    return {surface,text,secondary,std::ceil((minimum+(1-minimum)*opacity)*255)/255};
}
ContrastResult AdaptiveContrast::select(QColor preferred,const QVector<QColor>& surfaces) {
    if(surfaces.isEmpty()) return {current_.value_or(preferred),true};
    auto score=[&](QColor c) {double result=1000;for(auto b:surfaces) result=std::min(result,contrast(c,b));return result;};
    QColor light(245,247,255),dark(20,24,32),candidate=preferred;
    if(score(candidate)<4.5) { QColor end=score(dark)>=score(light)?dark:light; candidate=end;for(int i=1;i<=20;i++) {auto c=blend(preferred,end,i/20.);if(score(c)>=4.5){candidate=c;break;}} }
    if(!current_ || score(*current_)<3 || candidate==*current_) {current_=candidate;pending_.reset();confirmations_=0;}
    else {if(pending_==candidate) ++confirmations_;else {pending_=candidate;confirmations_=1;} if(confirmations_>=2){current_=candidate;pending_.reset();confirmations_=0;}}
    double darkest=1,lightest=0;for(auto surface:surfaces){darkest=std::min(darkest,luminance(surface));lightest=std::max(lightest,luminance(surface));}
    return {*current_,score(*current_)<4.5 || lightest-darkest>.3};
}
Monitor selectMonitor(const QVector<Monitor>& monitors,QPoint a) {
    if(monitors.isEmpty()) return {{0,0,1920,1080},{0,0,1920,1040},96};
    QPoint p(a.x()==std::numeric_limits<int>::min()?a.x():a.x()-1,a.y());
    for(auto m:monitors) if(m.bounds.contains(p)) return m;
    auto distance=[&](Monitor m) {double dx=double(p.x())-std::clamp(p.x(),m.work.left(),m.work.right());double dy=double(p.y())-std::clamp(p.y(),m.work.top(),m.work.bottom());return dx*dx+dy*dy;};
    return *std::min_element(monitors.begin(),monitors.end(),[&](auto x,auto y){return distance(x)<distance(y);});
}
Placement fitHost(QRect host,QPoint a,QSizeF size,double pixels,double radius) {
    if(host.isEmpty()||size.isEmpty()) return {};
    pixels=std::min({pixels,host.width()/size.width(),host.height()/size.height()});
    int w=std::clamp(int(std::ceil(size.width()*pixels)),1,host.width()),h=std::clamp(int(std::ceil(size.height()*pixels)),1,host.height());
    int x=int(std::clamp(qint64(a.x())-w,qint64(host.left()),qint64(host.left()+host.width()-w)));
    int y=std::clamp(a.y(),host.top(),host.top()+host.height()-h);
    return {{x,y,w,h},std::min(radius*pixels,std::min(w,h)/2.),pixels};
}
Placement fit(const QVector<Monitor>& monitors,std::optional<QPoint> anchor,QSizeF size,double scale,double radius,QPoint cursor) {
    auto m=selectMonitor(monitors,anchor.value_or(cursor));
    return fitHost(m.work,anchor.value_or(QPoint(m.work.x()+m.work.width()-24,m.work.y()+24)),size,scale*m.dpi/96.,radius);
}
QPointF relativeAnchor(QRect h,QPoint a) {return {std::clamp((a.x()-double(h.x()))/std::max(1,h.width()),0.,1.),std::clamp((a.y()-double(h.y()))/std::max(1,h.height()),0.,1.)};}
QPoint absoluteAnchor(QRect h,QPointF a) {return {h.x()+qRound(a.x()*h.width()),h.y()+qRound(a.y()*h.height())};}
bool fullyCovered(QRect card,const QVector<QRect>& covers) {
    if(card.isEmpty()||covers.isEmpty()) return false;
    QVector<int> edges={card.x(),card.x()+card.width()};for(auto r:covers) if(r.intersects(card)){edges<<std::max(r.x(),card.x())<<std::min(r.x()+r.width(),card.x()+card.width());}
    std::sort(edges.begin(),edges.end()); edges.erase(std::unique(edges.begin(),edges.end()),edges.end());
    for(int i=0;i+1<edges.size();i++){int bottom=card.y();QVector<QRect> ranges;for(auto r:covers)if(r.x()<=edges[i]&&r.x()+r.width()>=edges[i+1])ranges<<r;std::sort(ranges.begin(),ranges.end(),[](auto a,auto b){return a.y()<b.y();});for(auto r:ranges){if(r.y()>bottom)break;bottom=std::max(bottom,r.y()+r.height());}if(bottom<card.y()+card.height())return false;}return true;
}
void History::add(QDateTime at,std::optional<double> v){if(!points.isEmpty()&&at<points.last().at)points.clear();points.push_back({at,v});while(!points.isEmpty()&&(points.first().at.msecsTo(at)>60000||points.size()>256))points.removeFirst();}
QVariantList History::variant(QDateTime now) const {QVariantList list;for(auto p:points)list<<QVariantMap{{"x",1-std::clamp(p.at.msecsTo(now)/60000.,0.,1.)},{"value",p.value?QVariant(*p.value):QVariant()}};return list;}
std::optional<double> memoryRatio(const Reading& used,const Reading& total){if(used.status!=Availability::Available||total.status!=Availability::Available||!used.value||!total.value||*used.value<0||*total.value<=0)return {};return std::clamp(*used.value / *total.value,0.,1.);}
std::optional<QPair<QString,QString>> parseGpuCounter(QString name){static const QRegularExpression re("luid_0x([0-9a-f]+)_0x([0-9a-f]+)_phys_(\\d+)(?:_eng_(\\d+))?",QRegularExpression::CaseInsensitiveOption);auto m=re.match(name);if(!m.hasMatch())return {};bool hi,lo;auto h=m.captured(1).toUInt(&hi,16),l=m.captured(2).toUInt(&lo,16);if(!hi||!lo)return {};return QPair<QString,QString>{QString("%1:%2").arg(h,8,16,QChar('0')).arg(l,8,16,QChar('0')),m.captured(3)+":"+m.captured(4)};}
std::optional<double> busiestEngine(const QMap<QString,double>& counters,const QString& adapter){QMap<QString,double> engines;for(auto i=counters.begin();i!=counters.end();++i){auto p=parseGpuCounter(i.key());if(p&&p->first==adapter&&std::isfinite(i.value())&&i.value()>=0)engines[p->second]+=i.value();}if(engines.isEmpty())return {};double value=0;for(auto v:engines)value=std::max(value,v);return std::clamp(value,0.,100.);}
std::optional<Shortcut> parseShortcut(QString text){auto parts=text.toUpper().split('+',Qt::SkipEmptyParts);if(parts.size()<2)return {};quint32 mods=0x4000;for(int i=0;i+1<parts.size();i++){auto p=parts[i].trimmed();if(p=="CTRL")mods|=2;else if(p=="ALT")mods|=1;else if(p=="SHIFT")mods|=4;else if(p=="WIN"||p=="META"||p=="CMD")mods|=8;else return {};}auto last=parts.last().trimmed();if(last.size()!=1||!((last[0]>='A'&&last[0]<='Z')||(last[0]>='0'&&last[0]<='9')))return {};return Shortcut{mods,last[0].unicode()};}
std::optional<Version> Version::parse(QString text){static const QRegularExpression re("^v?(0|[1-9]\\d*)\\.(0|[1-9]\\d*)\\.(0|[1-9]\\d*)(?:-([0-9A-Za-z-]+(?:\\.[0-9A-Za-z-]+)*))?(?:\\+[0-9A-Za-z.-]+)?$");auto m=re.match(text);if(!m.hasMatch())return {};for(auto p:m.captured(4).split('.'))if(p.size()>1&&p[0]=='0'&&QRegularExpression("^\\d+$").match(p).hasMatch())return {};bool a,b,c;int x=m.captured(1).toInt(&a),y=m.captured(2).toInt(&b),z=m.captured(3).toInt(&c);if(!a||!b||!c)return {};return Version{x,y,z,m.captured(4)};}
int Version::compare(const Version& o)const{for(auto p:{qMakePair(major,o.major),qMakePair(minor,o.minor),qMakePair(patch,o.patch)})if(p.first!=p.second)return p.first<p.second?-1:1;if(pre==o.pre)return 0;if(pre.isEmpty())return 1;if(o.pre.isEmpty())return -1;auto a=pre.split('.'),b=o.pre.split('.');QRegularExpression numeric("^\\d+$");for(int i=0;i<std::min(a.size(),b.size());i++){bool an=numeric.match(a[i]).hasMatch(),bn=numeric.match(b[i]).hasMatch();int r=an&&bn?a[i].size()-b[i].size():an!=bn?(an?-1:1):0;if(r==0)r=QString::compare(a[i],b[i]);if(r)return r<0?-1:1;}return a.size()==b.size()?0:a.size()<b.size()?-1:1;}
QVariantMap defaults(){return {{"Version",3},{"Theme","System"},{"BackgroundColor",""},{"TextColor",""},{"AccentColor","#7CA8FF"},{"Opacity",.62},{"ProtectContrast",true},{"AutoTextContrast",true},{"TitleFontSize",18},{"ValueFontSize",36},{"LabelFontSize",13},{"DetailFontSize",13},{"Scale",1.},{"CornerRadius",20},{"ViewMode","Full"},{"CompactScale",1.},{"CompactLabelFontSize",13},{"CompactValueFontSize",26},{"CompactDetailFontSize",16},{"CompactCornerRadius",44},{"ShowCpu",true},{"ShowGpu",true},{"ShowRam",true},{"ShowVram",true},{"ShowTemperatures",true},{"ShowGraphs",true},{"RefreshSeconds",1.},{"TemperatureUnit","C"},{"GpuId",""},{"CpuSensorId",""},{"GpuSensorId",""},{"StartAtSignIn",true},{"AutoCheckUpdates",true},{"IncludeBetaUpdates",false},{"Locked",false},{"Shortcut","Ctrl+Alt+O"},{"RecoveryShortcut","Ctrl+Alt+U"},{"ImportOffered",false}};}
QVariantMap normalize(QVariantMap s){auto d=defaults();for(auto i=d.begin();i!=d.end();++i)if(!s.contains(i.key()))s[i.key()]=i.value();auto range=[&](QString key,double lo,double hi){bool ok;double v=s[key].toDouble(&ok);s[key]=ok&&std::isfinite(v)?std::clamp(v,lo,hi):d[key];};
    range("Opacity",0,1);range("Scale",.67,1.67);range("CompactScale",.67,1.67);range("CornerRadius",0,32);range("CompactCornerRadius",0,80);
    range("TitleFontSize",14,28);range("ValueFontSize",26,56);range("LabelFontSize",11,20);range("DetailFontSize",11,18);range("CompactLabelFontSize",11,20);range("CompactValueFontSize",18,48);range("CompactDetailFontSize",11,24);
    if(!QStringList{"System","Light","Dark"}.contains(s["Theme"].toString()))s["Theme"]="System";if(!QStringList{"Full","Compact"}.contains(s["ViewMode"].toString()))s["ViewMode"]="Full";
    if(!QStringList{"C","F"}.contains(s["TemperatureUnit"].toString()))s["TemperatureUnit"]="C";double interval=s["RefreshSeconds"].toDouble();if(interval!=.5&&interval!=1&&interval!=2)s["RefreshSeconds"]=1.;
    QRegularExpression color("^#[0-9a-fA-F]{6}$");for(auto key:{"BackgroundColor","TextColor","AccentColor"})if(!color.match(s[key].toString()).hasMatch())s[key]=d[key];
    for(auto key:{"Shortcut","RecoveryShortcut"})if(!parseShortcut(s[key].toString()))s[key]=d[key];s["Version"]=3;return s;
}
QVariantMap migrateLegacy(QJsonObject object){auto s=object.toVariantMap();int version=s.value("Version",0).toInt();if(version<2){for(auto key:{"Scale","CompactScale"})if(s.contains(key))s[key]=s[key].toDouble()/.75;if(s.contains("X")&&!s.contains("RightX"))s["LegacyLeftAnchor"]=true;}s.remove("LastUpdateCheckUtc");s["ImportOffered"]=true;return normalize(s);}
QVariantMap loadSettings(const QString& path,QString* error){QFile f(path);if(!f.exists())return defaults();if(!f.open(QIODevice::ReadOnly)){if(error)*error=f.errorString();return defaults();}QJsonParseError e;auto doc=QJsonDocument::fromJson(f.read(1024*1024),&e);if(!doc.isObject()||e.error!=QJsonParseError::NoError){if(error)*error="Settings are invalid; defaults loaded";return defaults();}return normalize(doc.object().toVariantMap());}
bool saveSettings(const QString& path,const QVariantMap& s,QString* error){QDir().mkpath(QFileInfo(path).absolutePath());QSaveFile f(path);if(!f.open(QIODevice::WriteOnly)||f.write(QJsonDocument(QJsonObject::fromVariantMap(normalize(s))).toJson())<0||!f.commit()){if(error)*error=f.errorString();return false;}return true;}
Reading temperature(const QVector<Sensor>& sensors,QDateTime at,QString kind,QString device,QString id,QDateTime now){if(!at.isValid())return Reading::missing("°C","Sensor bridge is not connected");if(at.msecsTo(now)>5000)return Reading::missing("°C","Sensor readings are stale",Availability::Stale);QVector<Sensor> choices;for(auto s:sensors)if(s.kind==kind&&(kind=="CPU"||s.device==device))choices<<s;std::stable_sort(choices.begin(),choices.end(),[&](auto a,auto b){auto rank=[&](Sensor s){return kind=="CPU"?(s.name.contains("Package",Qt::CaseInsensitive)||s.name.contains("Tctl",Qt::CaseInsensitive)?0:1):(s.name.compare("GPU Core",Qt::CaseInsensitive)==0?0:1);};return rank(a)<rank(b);});for(auto s:choices)if((id.isEmpty()||s.id==id)&&s.celsius)return Reading::of(*s.celsius,"°C",s.name+" · "+s.device);return Reading::missing("°C","Sensor unavailable; check the selected sensor or helper");}
QString platformKey(){
#ifdef Q_OS_WIN
return "windows-x64";
#elif defined(Q_OS_MACOS)
#if defined(Q_PROCESSOR_ARM_64)
return "macos-arm64";
#else
return "macos-x64";
#endif
#else
return "linux-x64";
#endif
}
QString assetName(const QString& tag,const QString& p){QString suffix=p.startsWith("windows")?"-setup.exe":p.startsWith("macos")?".dmg":".AppImage";return "GlassStats-Qt-"+QString(tag).remove(QRegularExpression("^v"))+"-"+p+suffix;}
std::optional<Release> selectRelease(const QJsonArray& list,QString installed,bool beta,QString platform){auto current=Version::parse(installed);if(!current)return {};std::optional<Release> best;std::optional<Version> bestVersion;for(auto value:list){auto o=value.toObject();auto v=Version::parse(o["tag_name"].toString());if(o["draft"].toBool()||!v||(!beta&&(o["prerelease"].toBool()||!v->pre.isEmpty()))||v->compare(*current)<=0||(bestVersion&&v->compare(*bestVersion)<=0))continue;QString name=assetName(o["tag_name"].toString(),platform);Release r{o["tag_name"].toString(),o["body"].toString(),o["html_url"].toString(),{},name,{},{},0};for(auto av:o["assets"].toArray()){auto a=av.toObject();QString n=a["name"].toString(),url=a["browser_download_url"].toString();QUrl u(url);if(u.scheme()!="https"||u.host()!="github.com"||!u.path().startsWith("/eduardojsborges/GlassStats-Qt/releases/download/"))continue;if(n==name){r.assetUrl=url;r.digest=a["digest"].toString();r.size=a["size"].toInteger();}if(n==name+".sha256")r.checksumUrl=url;}if(!r.assetUrl.isEmpty()){best=r;bestVersion=v;}}return best;}
}

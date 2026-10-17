#include "Platform.h"
#include <QGuiApplication>
#include <QScreen>
#include <QCursor>
#include <QStandardPaths>
#include <QFile>
#include <QDir>
#include <QCoreApplication>
#include <algorithm>

namespace gs {
SurfaceFrame analyzeSurface(const CaptureRequest& r,const QImage& image){
    SurfaceFrame result{r.generation,r.card,{}};if(image.isNull())return result;
    for(const auto& region:r.regions){QVector<QColor> colors;QRect area=region.rect.adjusted(-18,-12,18,12).intersected(r.card);
        for(int y=area.top();y<=area.bottom();y+=3)for(int x=area.left();x<=area.right();x+=3){QPoint p(x,y);bool excluded=false;for(auto rect:r.exclusions)if(rect.adjusted(-2,-2,2,2).contains(p)){excluded=true;break;}if(!excluded)for(auto rect:r.covers)if(rect.contains(p)){excluded=true;break;}if(excluded)continue;
            QPoint local=p-r.card.topLeft();if(local.x()<4||local.y()<4||local.x()>=image.width()-4||local.y()>=image.height()-4)continue;
            double rad=r.radius,dx=std::max({rad-local.x(),local.x()-(image.width()-1-rad),0.}),dy=std::max({rad-local.y(),local.y()-(image.height()-1-rad),0.});if(dx>0&&dy>0&&dx*dx+dy*dy>rad*rad)continue;
            colors.push_back(image.pixelColor(local));}
        if(colors.size()>=5){std::sort(colors.begin(),colors.end(),[](auto a,auto b){return luminance(a)<luminance(b);});result.surfaces[region.id]={colors[colors.size()/10],colors[colors.size()/2],colors[colors.size()*9/10]};}
    }return result;
}
void Platform::initialize(QWindow* w){window_=w;window_->setFlag(Qt::Tool,true);window_->setFlag(Qt::FramelessWindowHint,true);window_->setFlag(Qt::WindowStaysOnTopHint,true);window_->setFlag(Qt::WindowDoesNotAcceptFocus,true);}
QVector<Monitor> Platform::monitors()const{QVector<Monitor> list;for(auto screen:QGuiApplication::screens()){double pixelRatio=screen->devicePixelRatio();QRect b=screen->geometry(),w=screen->availableGeometry();list<<Monitor{QRect(b.topLeft()*pixelRatio,b.size()*pixelRatio),QRect(w.topLeft()*pixelRatio,w.size()*pixelRatio),pixelRatio*96};}return list;}
QRect Platform::bounds()const{if(!window_)return {};double pixelRatio=window_->devicePixelRatio();return QRect(window_->position()*pixelRatio,window_->size()*pixelRatio);}
QPoint Platform::cursor()const{auto p=QCursor::pos();auto screen=QGuiApplication::screenAt(p);return p*(screen?screen->devicePixelRatio():1);}
void Platform::place(Placement p){placed_=p;if(window_){double pixelRatio=window_->devicePixelRatio();window_->setGeometry(QRect(p.rect.topLeft()/pixelRatio,p.rect.size()/pixelRatio));}emit placementChanged();}
void Platform::setLayout(QSizeF logical,double scale,double radius){logical_=logical;scale_=scale;radius_=radius;}
Placement Platform::fitDesktop(std::optional<QPoint> anchor){auto p=fit(monitors(),anchor,logical_,scale_,radius_,cursor());place(p);return p;}
void Platform::fitCurrent(){auto r=bounds();fitDesktop(QPoint(r.x()+r.width(),r.y()));}
void Platform::beginDrag(){if(locked_)return;dragging_=true;if(window_)window_->startSystemMove();}
void Platform::updateDrag(){}
void Platform::endDrag(){if(!dragging_)return;dragging_=false;fitCurrent();emit positionCommitted();}
void Platform::setLocked(bool locked){locked_=locked;if(window_){bool visible=window_->isVisible();window_->setFlag(Qt::WindowTransparentForInput,locked);if(visible)window_->show();}emit changed();}
void Platform::setUserVisible(bool visible){userVisible_=visible;if(window_)window_->setVisible(visible&&!selecting_);emit changed();}
void Platform::reconcile(){}
void Platform::beginSelection(){emit status("Window attachment is unavailable in this desktop session.");}
void Platform::cancelSelection(){selecting_=false;setUserVisible(userVisible_);}
void Platform::detach(){attached_=false;emit changed();}
bool Platform::attachTo(quintptr,QPoint,QString* reason){if(reason)*reason="Attachment is not supported in this desktop session";return false;}
QString Platform::registerShortcuts(QString,QString,bool){return "Global shortcuts are unavailable; use the tray/menu-bar recovery action.";}
bool Platform::legacyStartup()const{return false;}
bool Platform::startupEnabled()const{return false;}
QString Platform::setStartup(bool,bool){return "Startup is not supported on this platform";}
QString Platform::helperAction(bool){return "The temperature helper is Windows-only. Native sensors are used on this system.";}
QString Platform::permissionAction(QString){return "This platform does not provide an additional permission flow for this feature.";}
}

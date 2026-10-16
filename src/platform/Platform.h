#pragma once
#include "core/Core.h"
#include <QObject>
#include <QWindow>
#include <QImage>
#include <QAbstractNativeEventFilter>
#include <QTimer>
#include <QPointer>
#include <memory>

namespace gs {
struct Frame { Sample sample; QVector<Adapter> adapters; QVector<Sensor> sensors; QDateTime sensorsAt; QString error; };
class MonitorProvider {
public:
    virtual ~MonitorProvider() = default;
    virtual Frame read(const QVariantMap& settings) = 0;
};
std::unique_ptr<MonitorProvider> createMonitorProvider();
struct CaptureRegion { QString id; QRect rect; QColor preferred; };
struct CaptureRequest { quint64 generation; QRect card; double radius; QVector<CaptureRegion> regions; QVector<QRect> exclusions, covers; };
struct SurfaceFrame { quint64 generation; QRect card; QMap<QString,QVector<QColor>> surfaces; };
class SurfaceCapture {
public:
    virtual ~SurfaceCapture() = default;
    virtual SurfaceFrame capture(const CaptureRequest& request) = 0;
};
std::unique_ptr<SurfaceCapture> createSurfaceCapture();
SurfaceFrame analyzeSurface(const CaptureRequest& request, const QImage& image);

class Platform : public QObject, public QAbstractNativeEventFilter {
    Q_OBJECT
protected:
    QPointer<QWindow> window_;
    bool attached_=false, selecting_=false, userVisible_=true, locked_=false, dragging_=false;
    QSizeF logical_{320,400}; double scale_=.75,radius_=20;
    Placement placed_;
public:
    explicit Platform(QObject* parent=nullptr):QObject(parent){}
    ~Platform() override = default;
    virtual Capabilities capabilities() const = 0;
    virtual void initialize(QWindow* window);
    virtual QVector<Monitor> monitors() const;
    virtual QRect bounds() const;
    virtual QPoint cursor() const;
    virtual QVector<QRect> coveredAreas() const { return {}; }
    virtual bool canHover() const { return effectiveVisible()&&!selecting_&&!locked_; }
    virtual bool effectiveVisible() const { return window_&&window_->isVisible(); }
    virtual QString attachmentDescription() const { return "Desktop overlay"; }
    virtual QString backdropDescription() const { return "Translucent tint (native blur unavailable)"; }
    virtual void place(Placement placement);
    virtual void setLayout(QSizeF logical,double scale,double radius);
    virtual Placement fitDesktop(std::optional<QPoint> anchor);
    virtual void fitCurrent();
    virtual void beginDrag();
    virtual void updateDrag();
    virtual void endDrag();
    virtual void setLocked(bool locked);
    virtual void setUserVisible(bool visible);
    virtual void reconcile();
    virtual void beginSelection();
    virtual void cancelSelection();
    virtual void detach();
    virtual bool attachTo(quintptr handle,QPoint anchor,QString* reason=nullptr);
    virtual QString registerShortcuts(QString lock,QString recovery,bool validation);
    virtual bool legacyStartup() const;
    virtual bool startupEnabled() const;
    virtual QString setStartup(bool enabled,bool replaceLegacy=false);
    virtual QString helperAction(bool install);
    virtual QString permissionAction(QString feature);
    bool attached() const {return attached_;}
    bool selecting() const {return selecting_;}
    bool userVisible() const {return userVisible_;}
    bool dragging() const {return dragging_;}
    bool locked() const {return locked_;}
    Placement placement() const {return placed_;}
    bool nativeEventFilter(const QByteArray&,void*,qintptr*) override {return false;}
signals:
    void changed();
    void placementChanged();
    void positionCommitted();
    void displayChanged();
    void requestCommand(QString command);
    void status(QString message);
    void targetLost();
};
std::unique_ptr<Platform> createPlatform();
}

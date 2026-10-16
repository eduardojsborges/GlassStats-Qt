#pragma once
#include "platform/Platform.h"
#include "Updater.h"
#include <QObject>
#include <QQuickWindow>
#include <QQuickItem>
#include <QPointer>
#include <QThread>
#include <QFutureWatcher>
#include <QSystemTrayIcon>
#include <QMenu>

namespace gs {
class Controller:public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantMap settings READ settings NOTIFY settingsChanged)
    Q_PROPERTY(QVariantMap metrics READ metrics NOTIFY metricsChanged)
    Q_PROPERTY(QVariantMap colors READ colors NOTIFY colorsChanged)
    Q_PROPERTY(QVariantMap textColors READ textColors NOTIFY textColorsChanged)
    Q_PROPERTY(QVariantList cpuHistory READ cpuHistory NOTIFY metricsChanged)
    Q_PROPERTY(QVariantList gpuHistory READ gpuHistory NOTIFY metricsChanged)
    Q_PROPERTY(QVariantList adapters READ adapters NOTIFY devicesChanged)
    Q_PROPERTY(QVariantList sensors READ sensors NOTIFY devicesChanged)
    Q_PROPERTY(QVariantList capabilities READ capabilities NOTIFY nativeChanged)
    Q_PROPERTY(QString attachment READ attachment NOTIFY nativeChanged)
    Q_PROPERTY(QString mode READ mode NOTIFY nativeChanged)
    Q_PROPERTY(bool userVisible READ userVisible NOTIFY nativeChanged)
    Q_PROPERTY(bool canHover READ canHover NOTIFY nativeChanged)
    Q_PROPERTY(bool effectiveVisible READ effectiveVisible NOTIFY nativeChanged)
    Q_PROPERTY(bool legacyAvailable READ legacyAvailable CONSTANT)
    Q_PROPERTY(bool legacyStartup READ legacyStartup NOTIFY nativeChanged)
    Q_PROPERTY(QString status READ status NOTIFY statusChanged)
    Q_PROPERTY(QString shortcutError READ shortcutError NOTIFY statusChanged)
    Q_PROPERTY(double renderScale READ renderScale NOTIFY geometryChanged)
    Q_PROPERTY(double logicalWidth READ logicalWidth NOTIFY geometryChanged)
    Q_PROPERTY(double logicalHeight READ logicalHeight NOTIFY geometryChanged)
    Q_PROPERTY(QString version READ version CONSTANT)
    Q_PROPERTY(Updater* updater READ updater CONSTANT)
    QVariantMap settings_,metrics_,colors_,textColors_;
    History cpuHistory_,gpuHistory_;Frame frame_;
    std::unique_ptr<Platform> platform_;std::unique_ptr<SurfaceCapture> capture_;
    QThread monitorThread_;QObject* worker_=nullptr;std::shared_ptr<MonitorProvider> monitor_;
    QTimer sampleTimer_,adaptiveTimer_,watchdog_,saveTimer_,dragTimer_;
    bool captureBusy_=false,sampling_=false,validation_=false,menuOpen_=false,shuttingDown_=false;
    quint64 sampleGeneration_=0,surfaceGeneration_=0,adaptiveFrames_=0;
    struct Label { QPointer<QQuickItem> item; QColor preferred; AdaptiveContrast contrast; };
    QMap<QString,Label> labels_;QVector<QPointer<QQuickItem>> exclusions_;
    QPointer<QQuickWindow> window_;QSystemTrayIcon tray_;QMenu menu_;Updater updater_;
    QString path_,legacyPath_,status_,shortcutError_;QSizeF logical_{320,430};double renderScale_=.75;
    QVector<QRect> lastCovers_;QRect lastBounds_;QDateTime lastSample_;
    void apply(bool persist=true);void save();void scheduleSample();void receive(Frame frame,quint64 generation);
    void fitWindow();void refreshGeometry();void sampleSurface();void refreshPalette();void buildMenu();void invalidate();
public:
    explicit Controller(bool validation,QString profile,QObject* parent=nullptr);
    ~Controller()override;
    void initialize(QQuickWindow* window);
    QVariantMap settings()const{return settings_;}QVariantMap metrics()const{return metrics_;}QVariantMap colors()const{return colors_;}QVariantMap textColors()const{return textColors_;}
    QVariantList cpuHistory()const{return cpuHistory_.variant(QDateTime::currentDateTimeUtc());}QVariantList gpuHistory()const{return gpuHistory_.variant(QDateTime::currentDateTimeUtc());}
    QVariantList adapters()const;QVariantList sensors()const;QVariantList capabilities()const;
    QString attachment()const{return platform_->attachmentDescription();}QString mode()const{return platform_->selecting()?"Selecting":platform_->attached()?"Attached":"Desktop";}
    bool userVisible()const{return platform_->userVisible();}bool canHover()const{return platform_->canHover()&&!menuOpen_;}bool effectiveVisible()const{return platform_->effectiveVisible();}
    bool legacyAvailable()const;bool legacyStartup()const{return platform_->legacyStartup();}
    QString status()const{return status_;}QString shortcutError()const{return shortcutError_;}
    double renderScale()const{return renderScale_;}double logicalWidth()const{return logical_.width();}double logicalHeight()const{return logical_.height();}
    QString version()const{return GLASSSTATS_VERSION;}Updater* updater(){return &updater_;}
    Platform* platform(){return platform_.get();}const Frame& frame()const{return frame_;}quint64 adaptiveFrames()const{return adaptiveFrames_;}
    Q_INVOKABLE void setSetting(QString key,QVariant value);
    Q_INVOKABLE void resetDefaults();Q_INVOKABLE void importLegacy();Q_INVOKABLE void skipImport();
    Q_INVOKABLE void action(QString command);Q_INVOKABLE void setContentSize(double width,double height);
    Q_INVOKABLE void registerLabel(QString id,QQuickItem* item,QColor preferred);
    Q_INVOKABLE void unregisterLabel(QString id,QQuickItem* item);
    Q_INVOKABLE void registerExclusion(QQuickItem* item);Q_INVOKABLE void unregisterExclusion(QQuickItem* item);
    Q_INVOKABLE void beginDrag();Q_INVOKABLE void endDrag();Q_INVOKABLE void showMenu();
    Q_INVOKABLE QString formatReading(QVariantMap reading,int decimals=0)const;
    Q_INVOKABLE QString memoryText(QString metric,QString part="usedTotal")const;
    Q_INVOKABLE double memoryFraction(QString metric)const;
    Q_INVOKABLE void checkUpdates(bool manual=true);
    Q_INVOKABLE void helper(bool install);Q_INVOKABLE void requestPermission(QString feature);
    Q_INVOKABLE void openVerifiedInstaller(QString path);
    void setStatus(QString text){status_=text;emit statusChanged();}
signals:
    void settingsChanged();void metricsChanged();void colorsChanged();void textColorsChanged();void devicesChanged();void nativeChanged();void statusChanged();void geometryChanged();
    void openSettings();void dismissDetails();void installConfirmation(QString path);
};
}

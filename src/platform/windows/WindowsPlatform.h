#pragma once
#include "platform/Platform.h"
#include <windows.h>
#include <atomic>

namespace gs {
class WindowsPlatform final:public Platform {
    Q_OBJECT
    HWND hwnd_=nullptr,target_=nullptr,candidate_=nullptr;
    DWORD targetPid_=0; quint64 targetStart_=0,generation_=0;
    std::atomic_bool destroyed_{false},queued_{false};
    QPointF relative_;QRect lastHost_;QPoint dragCursor_,dragOrigin_;
    QVector<HWINEVENTHOOK> events_;HHOOK mouse_=nullptr,keyboard_=nullptr;
    QVector<QWindow*> highlights_;QTimer selectionTimeout_;
    bool consumeUp_=false,selectionValid_=false;QPoint selectionPoint_;
    QString selectionReason_;
    bool sessionLocked_=false,displayOn_=true;
    HPOWERNOTIFY powerNotification_=nullptr;
    static WindowsPlatform* instance_;
    static void CALLBACK eventProc(HWINEVENTHOOK,DWORD,HWND,LONG,LONG,DWORD,DWORD);
    static LRESULT CALLBACK mouseProc(int,WPARAM,LPARAM);
    static LRESULT CALLBACK keyProc(int,WPARAM,LPARAM);
    bool validTarget() const;
    bool hostArea(QRect& area) const;
    bool candidate(HWND window,QRect& area,QString& reason) const;
    void cleanupEvents();void applyVisibility();void restoreStacking();void clip();void highlight(QRect area);
    void placeAttached(QRect host,bool proportional);
public:
    explicit WindowsPlatform(QObject* parent=nullptr);
    ~WindowsPlatform()override;
    Capabilities capabilities()const override;
    void initialize(QWindow*)override;
    QVector<Monitor> monitors()const override;
    QRect bounds()const override;
    QPoint cursor()const override;
    QVector<QRect> coveredAreas()const override;
    bool canHover()const override;
    bool effectiveVisible()const override;
    bool canSampleSurface()const override{return effectiveVisible()&&!sessionLocked_&&displayOn_;}
    QString attachmentDescription()const override;
    QString backdropDescription()const override;
    void place(Placement p)override;
    Placement fitDesktop(std::optional<QPoint>)override;
    void fitCurrent()override;
    void beginDrag()override;void updateDrag()override;void endDrag()override;
    void setLocked(bool)override;void setUserVisible(bool)override;void reconcile()override;
    void beginSelection()override;void cancelSelection()override;void detach()override;
    bool attachTo(quintptr,QPoint,QString* reason=nullptr)override;
    QString registerShortcuts(QString,QString,bool)override;
    bool legacyStartup()const override;bool startupEnabled()const override;
    QString setStartup(bool,bool=false)override;
    QString helperAction(bool)override;
    bool nativeEventFilter(const QByteArray&,void*,qintptr*)override;
};
}

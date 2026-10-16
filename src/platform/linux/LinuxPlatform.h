#pragma once
#include "platform/Platform.h"
#include <QSocketNotifier>

namespace gs {
class LinuxPlatform final:public Platform {
    Q_OBJECT
    struct Native;
    std::unique_ptr<Native> native_;
public:
    LinuxPlatform();~LinuxPlatform()override;
    Capabilities capabilities()const override;
    void initialize(QWindow*)override;
    void setLocked(bool)override;
    void reconcile()override;
    void beginSelection()override;void cancelSelection()override;
    bool attachTo(quintptr,QPoint,QString* reason=nullptr)override;void detach()override;
    void fitCurrent()override;
    void endDrag()override;
    QString attachmentDescription()const override;
    QVector<QRect> coveredAreas()const override;
    bool canHover()const override;
    QString registerShortcuts(QString,QString,bool)override;
    bool startupEnabled()const override;QString setStartup(bool,bool=false)override;
};
}

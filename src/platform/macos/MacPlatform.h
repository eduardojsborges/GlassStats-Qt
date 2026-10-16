#pragma once
#include "platform/Platform.h"

namespace gs {
class MacPlatform final:public Platform {
    Q_OBJECT
    struct Native;std::unique_ptr<Native> native_;
public:
    MacPlatform();~MacPlatform()override;
    void initialize(QWindow*)override;
    Capabilities capabilities()const override;
    QString backdropDescription()const override;
    void setLocked(bool)override;void setUserVisible(bool)override;
    void beginSelection()override;void cancelSelection()override;
    bool attachTo(quintptr,QPoint,QString* reason=nullptr)override;
    void detach()override;void reconcile()override;void fitCurrent()override;void endDrag()override;
    QString attachmentDescription()const override;bool canHover()const override;
    QVector<QRect> coveredAreas()const override;
    QString registerShortcuts(QString,QString,bool)override;
    bool startupEnabled()const override;QString setStartup(bool,bool=false)override;
    QString permissionAction(QString)override;
};
}

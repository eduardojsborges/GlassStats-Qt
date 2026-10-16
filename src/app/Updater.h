#pragma once
#include "core/Core.h"
#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QFile>
#include <QCryptographicHash>

namespace gs {
class Updater:public QObject {
    Q_OBJECT
    Q_PROPERTY(QString status READ status NOTIFY changed)
    Q_PROPERTY(QString notes READ notes NOTIFY changed)
    Q_PROPERTY(QString releaseTag READ releaseTag NOTIFY changed)
    Q_PROPERTY(bool busy READ busy NOTIFY changed)
    Q_PROPERTY(bool available READ available NOTIFY changed)
    Q_PROPERTY(double progress READ progress NOTIFY changed)
    QNetworkAccessManager network_;QNetworkReply* reply_=nullptr;
    std::optional<Release> release_;QString status_="No update check yet.",path_,expected_;bool busy_=false;double progress_=0;
    std::unique_ptr<QFile> output_;std::unique_ptr<QCryptographicHash> hash_;
    void downloadAsset();void fail(QString error);QNetworkRequest request(QUrl url)const;
public:
    explicit Updater(QObject* parent=nullptr):QObject(parent){}
    QString status()const{return status_;}QString notes()const{return release_?release_->notes:QString();}QString releaseTag()const{return release_?release_->tag:QString();}
    bool busy()const{return busy_;}bool available()const{return release_.has_value();}double progress()const{return progress_;}
    void check(bool beta);
    Q_INVOKABLE void download();
    Q_INVOKABLE void cancel();
    Q_INVOKABLE void openRelease();
signals:
    void changed();void checkFinished();void installerReady(QString path);
};
}

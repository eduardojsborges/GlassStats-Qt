#include "app/Controller.h"
#include "qa/Validation.h"
#include <QApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QCommandLineParser>
#include <QLocalServer>
#include <QLocalSocket>
#include <QStandardPaths>
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QTimer>
#include <QMutex>
#include <QMutexLocker>

static QString logPath;
static QMutex logMutex;
static void logMessage(QtMsgType type,const QMessageLogContext&,const QString& text){if(type==QtDebugMsg)return;QMutexLocker guard(&logMutex);QFile f(logPath);if(f.size()>1024*1024){QFile::remove(logPath+".old");f.rename(logPath+".old");f.setFileName(logPath);}if(f.open(QIODevice::Append))f.write((QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs)+" "+text+"\n").toUtf8());fprintf(stderr,"%s\n",text.toUtf8().constData());}
int main(int argc,char** argv){
    QApplication application(argc,argv);QCoreApplication::setOrganizationName("eduardojsborges");QCoreApplication::setApplicationName("GlassStats-Qt");QCoreApplication::setApplicationVersion(GLASSSTATS_VERSION);QApplication::setQuitOnLastWindowClosed(false);QQuickStyle::setStyle("Basic");
    QCommandLineParser parser;parser.addHelpOption();parser.addVersionOption();parser.addOptions({{"smoke","Run isolated UI checks"},{"soak","Run two-hour isolated stability check"},{"qml-check","Load the QML and native provider without changing preferences"},{"test-host","Run separate-process test host"},{"profile","Use a separate preference profile","directory"},{"qa-dir","Write explicit QA artifacts","directory"},{"startup","Launch at sign-in"}});parser.process(application);
    if(parser.isSet("test-host"))return gs::runTestHost(parser.value("qa-dir"));
    bool validation=parser.isSet("smoke")||parser.isSet("soak")||parser.isSet("qml-check");QString data=parser.value("profile");if(data.isEmpty())data=validation?QDir::tempPath()+"/GlassStatsQt-validation":QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);QDir().mkpath(data);logPath=data+"/errors.log";qInstallMessageHandler(logMessage);
    QLocalServer server;QString instance="GlassStatsQt-"+QString::fromLatin1(QCryptographicHash::hash(data.toUtf8(),QCryptographicHash::Sha256).toHex().left(20));QLocalSocket existing;existing.connectToServer(instance);if(existing.waitForConnected(350)){if(!validation){existing.write("recover");existing.waitForBytesWritten(500);}return 0;}QLocalServer::removeServer(instance);server.setSocketOptions(QLocalServer::UserAccessOption);if(!server.listen(instance)){qCritical("Could not create the single-instance endpoint");return 1;}
    gs::Controller controller(validation,data);QQmlApplicationEngine engine;engine.rootContext()->setContextProperty("app",&controller);engine.load(QUrl("qrc:/qml/Main.qml"));if(engine.rootObjects().isEmpty())return 1;auto window=qobject_cast<QQuickWindow*>(engine.rootObjects().first());if(!window)return 1;
    QObject::connect(&server,&QLocalServer::newConnection,&controller,[&]{while(auto socket=server.nextPendingConnection()){QObject::connect(socket,&QLocalSocket::readyRead,&controller,[socket,&controller,validation]{if(!validation&&socket->readAll().startsWith("recover"))controller.action("recover");socket->disconnectFromServer();});QObject::connect(socket,&QLocalSocket::disconnected,socket,&QObject::deleteLater);}});
    controller.initialize(window);if(parser.isSet("qml-check"))QTimer::singleShot(1800,&application,&QCoreApplication::quit);else if(validation)gs::runValidation(&controller,window,parser.value("qa-dir"),parser.isSet("soak"));int result=application.exec();return result;
}

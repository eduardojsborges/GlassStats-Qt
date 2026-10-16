#pragma once
#include <QString>
class QQuickWindow;
namespace gs {class Controller;int runTestHost(QString directory);void runValidation(Controller*,QQuickWindow*,QString directory,bool soak);}

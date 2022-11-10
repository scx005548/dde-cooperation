#include <DApplication>

#include "ui/MainWindow.h"

DWIDGET_USE_NAMESPACE

int main(int argc, char *argv[]) {
    DApplication app(argc, argv);
    app.setOrganizationName("deepin");
    app.setApplicationName("dde-scrcpy");

    QApplication::setAttribute(Qt::AA_UseOpenGLES);

    QStringList argList = app.arguments();
    if (argList.count() < 2) {
        qWarning() << "arguments count error!";
        return -1;
    }

    QString ip = argList[1];

    qDebug() << "IP:" << ip;

    MainWindow w(ip);

    return app.exec();
}

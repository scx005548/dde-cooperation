#include <DApplication>
#include <QDebug>

#include "confirmdialog.h"

DWIDGET_USE_NAMESPACE

const int IP = 1;
const int MachineName = 2;
const int PipeFd = 3;

int main(int argc, char *argv[])
{
    DApplication app(argc, argv);
    app.setOrganizationName("deepin");
    app.setApplicationName("dde-cooperation-dialog");

    QStringList argList = app.arguments();
    if (argList.count() < 4) {
        qWarning() << "arguments count error!";
        return -1;
    }

    qDebug() << "IP:" << argList[IP] << " MachineName:" << argList[MachineName] << " PipeFd:" << argList[PipeFd];
    
    bool ok = false;
    int pipeFd = argList[PipeFd].toInt(&ok);
    if (!ok) {
        qWarning() << "arg pipe fd has error!";
        return -1;
    }

    ConfirmDialog dialog(argList[IP], argList[MachineName], pipeFd);
    dialog.show();

    return app.exec();
}
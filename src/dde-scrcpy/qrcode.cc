#include <DApplication>
#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusReply>

#include "ui/NewDeviceDialog.h"

DWIDGET_USE_NAMESPACE

#define DDE_PROTO_VER 13

int main(int argc, char *argv[]) {
    DApplication app(argc, argv);
    app.setOrganizationName("deepin");
    app.setApplicationName("dde-scrcpy");

    auto bus = QDBusConnection::sessionBus();
    QDBusInterface cooperation("com.deepin.Cooperation",
                               "/com/deepin/Cooperation",
                               "com.deepin.Cooperation",
                               bus);
    QDBusReply<QString> reply = cooperation.call("GetUUID");
    QString uuid = reply.value();

    NewDeviceDialog w(uuid, DDE_PROTO_VER);
    w.show();

    return app.exec();
}

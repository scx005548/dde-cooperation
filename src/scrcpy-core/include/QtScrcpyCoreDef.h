#pragma once
#include <QString>

namespace qsc {

struct DeviceParams {
    // necessary
    QString serial = ""; // 设备序列号

    // optional
    QString recordPath = "";          // 视频保存路径

    QString pushFilePath = "/sdcard/"; // 推送到安卓设备的文件保存路径（必须以/结尾）

    bool closeScreen = false;         // 启动时自动息屏
    QString gameScript = "";          // 游戏映射脚本
};

} // namespace qsc

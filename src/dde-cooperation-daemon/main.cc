#include "Manager.h"

#include <QApplication>

namespace fs = std::filesystem;

int main(int argc, char *argv[]) {
    std::string runtimeDir = getenv("XDG_RUNTIME_DIR");
    if (runtimeDir.empty()) {
        throw std::runtime_error("XDG_RUNTIME_DIR not set in the environment");
    }

    fs::path dataDir = fs::path(runtimeDir) / "dde-cooperation";

    QApplication::setQuitOnLastWindowClosed(false);
    QApplication app(argc, argv);

    Manager cooperation(dataDir);

    app.exec();
}

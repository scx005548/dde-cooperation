#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include "Manager.h"

#include <QApplication>

namespace fs = std::filesystem;

int main(int argc, char* argv[]) {
    auto console = spdlog::stdout_color_mt("console");
    spdlog::set_default_logger(console);
    spdlog::set_level(spdlog::level::debug);
    spdlog::set_pattern("[%^%l%$] [thread %t]: %v");

    std::string runtimeDir = getenv("XDG_RUNTIME_DIR");
    if (runtimeDir.empty()) {
        throw std::runtime_error("XDG_RUNTIME_DIR not set in the environment");
    }

    fs::path dataDir = fs::path(runtimeDir) / "dde-cooperation";

    QApplication app(argc, argv);

    Manager cooperation(dataDir);

    app.exec();
}

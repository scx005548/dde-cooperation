#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <glibmm.h>
#include <giomm.h>

#include "Manager.h"

namespace fs = std::filesystem;

int main() {
    auto console = spdlog::stdout_color_mt("console");
    spdlog::set_default_logger(console);
    spdlog::set_level(spdlog::level::debug);
    spdlog::set_pattern("[%^%l%$] [thread %t]: %v");

    std::string runtimeDir = getenv("XDG_RUNTIME_DIR");
    if (runtimeDir.empty()) {
        throw std::runtime_error("XDG_RUNTIME_DIR not set in the environment");
    }

    fs::path dataDir = fs::path(runtimeDir) / "dde-cooperation";

    Glib::init();
    Gio::init();

    Manager cooperation(dataDir);

    Glib::RefPtr<Glib::MainLoop> loop = Glib::MainLoop::create();
    loop->run();
}

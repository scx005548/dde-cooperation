#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <glibmm.h>
#include <giomm.h>

#include "Cooperation.h"

int main() {
    auto console = spdlog::stdout_color_mt("console");
    spdlog::set_default_logger(console);
    spdlog::set_level(spdlog::level::debug);
    spdlog::set_pattern("[%^%l%$] [thread %t]: %v");

    Glib::init();
    Gio::init();

    Cooperation cooperation;

    Glib::RefPtr<Glib::MainLoop> loop = Glib::MainLoop::create();
    loop->run();
}

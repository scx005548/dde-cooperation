#include <spdlog/spdlog.h>
#include <spdlog/sinks/syslog_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <glibmm.h>
#include <giomm.h>

#include "Manager.h"
#include "FuseClient.h"

int main() {
    spdlog::set_default_logger(spdlog::stdout_color_mt("console"));
    spdlog::set_level(spdlog::level::debug);
    spdlog::set_pattern("[%^%l%$] [thread %t]: %v");

    Glib::init();
    Gio::init();

    Manager manager;

    Glib::RefPtr<Glib::MainLoop> loop = Glib::MainLoop::create();
    loop->run();
}

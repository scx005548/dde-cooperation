#include <spdlog/spdlog.h>
#include <spdlog/sinks/syslog_sink.h>
#include <glibmm.h>
#include <giomm.h>

#include "Manager.h"

std::shared_ptr<spdlog::logger> logger;

int main() {
    logger = spdlog::syslog_logger_mt("syslog", "dde-cooperation-user", LOG_PID | LOG_CONS);
    logger->set_pattern("[%Y-%m-%d %H:%M:%S %z] [%^%l%$] [thread %t]: %v");
    logger->set_level(spdlog::level::debug); // Set global log level to debug

    Glib::init();
    Gio::init();

    Manager manager;

    Glib::RefPtr<Glib::MainLoop> loop = Glib::MainLoop::create();
    loop->run();
}

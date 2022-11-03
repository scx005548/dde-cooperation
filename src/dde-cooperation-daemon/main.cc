#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <glibmm.h>
#include <giomm.h>

#include "uvxx/Loop.h"
#include "uvxx/Signal.h"

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

    auto loop = uvxx::Loop::defaultLoop();
    Glib::RefPtr<Glib::MainLoop> glibLoop = Glib::MainLoop::create();

    Manager cooperation(loop, dataDir);

    auto glibThread = std::thread([glibLoop]() { glibLoop->run(); });

    auto exitCb = [loop, glibLoop, &glibThread]([[maybe_unused]] int signum) {
        if (glibThread.joinable()) {
            glibLoop->quit();

            glibThread.join();
        }

        loop->stop();
    };
    auto signalInt = std::make_shared<uvxx::Signal>(loop);
    signalInt->onTrigger(exitCb);
    signalInt->start(SIGINT);
    auto signalQuit = std::make_shared<uvxx::Signal>(loop);
    signalQuit->onTrigger(exitCb);
    signalQuit->start(SIGQUIT);
    auto signalTerm = std::make_shared<uvxx::Signal>(loop);
    signalTerm->onTrigger(exitCb);
    signalTerm->start(SIGTERM);
    auto signalHup = std::make_shared<uvxx::Signal>(loop);
    signalHup->onTrigger(exitCb);
    signalHup->start(SIGHUP);

    loop->run();

    signalInt->close();
    signalQuit->close();
    signalTerm->close();
    signalHup->close();
}

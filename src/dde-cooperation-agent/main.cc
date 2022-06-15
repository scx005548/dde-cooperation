#include <glibmm.h>
#include <giomm.h>

#include "Agent.h"

int main() {
    Glib::init();
    Gio::init();

    Agent agent;

    Glib::RefPtr<Glib::MainLoop> loop = Glib::MainLoop::create();
    loop->run();
}

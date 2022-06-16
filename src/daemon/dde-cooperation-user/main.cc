#include <glibmm.h>
#include <giomm.h>

#include "Manager.h"

int main() {
    Glib::init();
    Gio::init();

    Manager manager;

    Glib::RefPtr<Glib::MainLoop> loop = Glib::MainLoop::create();
    loop->run();
}

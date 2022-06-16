#include <glibmm.h>
#include <giomm.h>

#include "Cooperation.h"

int main() {
    Glib::init();
    Gio::init();

    Cooperation cooperation;

    Glib::RefPtr<Glib::MainLoop> loop = Glib::MainLoop::create();
    loop->run();
}

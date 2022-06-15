#ifndef DDE_COOPERATION_AGENT_EDGE_DETECTOR_X11_H
#define DDE_COOPERATION_AGENT_EDGE_DETECTOR_X11_H

#include <memory>

#include <X11/Xlib-xcb.h>

#include "../EdgeDetector.h"

class X11 : public EdgeDetector {
public:
    X11();

    virtual void start() override;

private:
    std::unique_ptr<Display, decltype(&XCloseDisplay)> m_dpy;
    xcb_connection_t *m_conn;
    xcb_screen_t *m_screen;

    int m_xinput2OPCode;
};

#endif // !DDE_COOPERATION_AGENT_EDGE_DETECTOR_X11_H

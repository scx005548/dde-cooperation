#ifndef DDE_COOPERATION_USER_EDGE_DETECTOR_X11_H
#define DDE_COOPERATION_USER_EDGE_DETECTOR_X11_H

#include <memory>

#include "../DisplayServer.h"

#include <X11/Xlib-xcb.h>

class X11 : public DisplayServer {
public:
    explicit X11(Manager *manager);

    virtual void start() override;

protected:
    virtual void hideMouse(bool hide) override;
    virtual void moveMouse(uint16_t x, uint16_t y) override;

private:
    std::unique_ptr<Display, decltype(&XCloseDisplay)> m_dpy;
    xcb_connection_t *m_conn;
    xcb_screen_t *m_screen;

    int m_xinput2OPCode;

    void initXinputExtension();
    void initXfixesExtension();
};

#endif // !DDE_COOPERATION_USER_EDGE_DETECTOR_X11_H

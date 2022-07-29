#ifndef DDE_COOPERATION_USER_EDGE_DETECTOR_X11_H
#define DDE_COOPERATION_USER_EDGE_DETECTOR_X11_H

#include <memory>

#include "../DisplayServer.h"

#include <xcb/xcb.h>

class X11 : public DisplayServer {
public:
    explicit X11(Manager *manager);

    virtual void start() override;

protected:
    virtual void hideMouse(bool hide) override;
    virtual void moveMouse(uint16_t x, uint16_t y) override;

private:
    xcb_connection_t *m_conn;
    int m_screenDefaultNbr;
    xcb_screen_t *m_screen;

    uint8_t m_xinput2OPCode;

    xcb_screen_t *screenOfDisplay(int screen);

    void initRandrExtension();
    void initXinputExtension();
    void initXfixesExtension();
};

#endif // !DDE_COOPERATION_USER_EDGE_DETECTOR_X11_H

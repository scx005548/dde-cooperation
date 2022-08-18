#ifndef DDE_COOPERATION_DEAMON_X11_X11_H
#define DDE_COOPERATION_DEAMON_X11_X11_H

#include <memory>

#include <xcb/xcb.h>

#define XCB_REPLY_CONNECTION_ARG(connection, ...) connection
#define XCB_REPLY(call, ...)                                                                       \
    std::unique_ptr<call##_reply_t>(                                                               \
        call##_reply(XCB_REPLY_CONNECTION_ARG(__VA_ARGS__), call(__VA_ARGS__), nullptr))

namespace uvxx {
class Loop;
class Poll;
} // namespace uvxx

namespace X11 {

class X11 {
public:
    explicit X11(const std::shared_ptr<uvxx::Loop> &uvLoop);
    virtual ~X11();

    virtual void handleEvent(std::shared_ptr<xcb_generic_event_t> event) = 0;

protected:
    std::shared_ptr<uvxx::Loop> m_uvLoop;
    std::shared_ptr<uvxx::Poll> m_uvPoll;
    xcb_connection_t *m_conn;
    int m_xcbFd;
    const xcb_setup_t *m_setup;
    xcb_screen_t *m_screen;

    xcb_screen_t *screenOfDisplay(int screen);
    void onEvent(int events);
};

} // namespace X11

#endif // !DDE_COOPERATION_DEAMON_X11_X11_H

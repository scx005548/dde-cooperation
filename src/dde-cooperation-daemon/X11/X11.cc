#include "X11.h"

#include <stdexcept>

#include <cstdlib>
#include <cstdio>

#include <xcb/xcb.h>
#include <xcb/randr.h>
#include <xcb/xproto.h>
#include <xcb/xfixes.h>
#include <xcb/xinput.h>

#include <fmt/core.h>
#include <spdlog/spdlog.h>

#include "uvxx/Loop.h"
#include "uvxx/Poll.h"

#include "utils/ptr.h"

namespace X11 {

X11::X11(const std::shared_ptr<uvxx::Loop> &uvLoop)
    : m_uvLoop(uvLoop) {
    int screenDefaultNbr;
    m_conn = xcb_connect(nullptr, &screenDefaultNbr);

    if (int err = xcb_connection_has_error(m_conn)) {
        throw std::runtime_error(fmt::format("failed to connect to X11: {}", err));
    }

    m_xcbFd = xcb_get_file_descriptor(m_conn);
    spdlog::info("xcb fd: {}", m_xcbFd);
    m_uvPoll = std::make_shared<uvxx::Poll>(m_uvLoop, m_xcbFd);
    m_uvPoll->onEvent([this](int events) { onEvent(events); });
    m_uvPoll->start(UV_READABLE);

    m_setup = xcb_get_setup(m_conn);
    m_screen = screenOfDisplay(screenDefaultNbr);
}

X11::~X11() {
    m_uvPoll->stop();
}

xcb_screen_t *X11::screenOfDisplay(int screen) {
    xcb_screen_iterator_t iter = xcb_setup_roots_iterator(m_setup);
    for (; iter.rem; --screen, xcb_screen_next(&iter)) {
        if (screen == 0) {
            return iter.data;
        }
    }

    return nullptr;
}

void X11::onEvent([[maybe_unused]] int events) {
    std::shared_ptr<xcb_generic_event_t> event;
    while (event.reset(xcb_poll_for_event(m_conn)), event) {
        handleEvent(event);
    }
}

} // namespace X11

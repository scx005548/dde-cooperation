#include "Display.h"

#include <stdexcept>

#include <xcb/xcb.h>
#include <xcb/randr.h>
#include <xcb/xproto.h>
#include <xcb/xfixes.h>
#include <xcb/xinput.h>

#include <fmt/core.h>
#include <spdlog/spdlog.h>

using namespace X11;

Display::Display(const std::shared_ptr<uvxx::Loop> &uvLoop, Manager *manager)
    : X11(uvLoop)
    , DisplayBase(manager) {

    initXinputExtension();
    initXfixesExtension();

    uint32_t mask = XCB_EVENT_MASK_STRUCTURE_NOTIFY;
    xcb_change_window_attributes(m_conn, m_screen->root, XCB_CW_EVENT_MASK, &mask);

    handleScreenSizeChange(m_screen->width_in_pixels, m_screen->height_in_pixels);
}

Display::~Display() {
}

void Display::initRandrExtension() {
    const xcb_query_extension_reply_t *reply = xcb_get_extension_data(m_conn, &xcb_randr_id);
    if (!reply->present) {
        throw std::runtime_error("randr extension not found");
    }
}

void Display::initXinputExtension() {
    {
        const char *extname = "XInputExtension";
        auto reply = XCB_REPLY(xcb_query_extension, m_conn, strlen(extname), extname);
        if (!reply->present) {
            throw std::runtime_error("XInput extension is not available");
        }
        m_xinput2OPCode = reply->major_opcode;
    }

    {
        auto reply = XCB_REPLY(xcb_input_xi_query_version,
                               m_conn,
                               XCB_INPUT_MAJOR_VERSION,
                               XCB_INPUT_MINOR_VERSION);
        if (!reply || reply->major_version != 2) {
            throw std::runtime_error("X server does not support XInput 2");
        }

        spdlog::debug("XInput version {}.{}", reply->major_version, reply->minor_version);
    }

    struct {
        xcb_input_event_mask_t head;
        xcb_input_xi_event_mask_t mask;
    } mask;
    mask.head.deviceid = XCB_INPUT_DEVICE_ALL;
    mask.head.mask_len = sizeof(mask.mask) / sizeof(uint32_t);
    mask.mask = static_cast<xcb_input_xi_event_mask_t>(XCB_INPUT_XI_EVENT_MASK_HIERARCHY |
                                                       XCB_INPUT_XI_EVENT_MASK_MOTION);
    auto cookie = xcb_input_xi_select_events(m_conn, m_screen->root, 1, &mask.head);
    auto err = xcb_request_check(m_conn, cookie);
    if (err) {
        throw std::runtime_error(fmt::format("xcb_input_xi_select_events: {}", err->error_code));
    }
}

void Display::initXfixesExtension() {
    m_xfixes = xcb_get_extension_data(m_conn, &xcb_xfixes_id);
    if (!m_xfixes->present) {
        spdlog::warn("xfixes is not present");
        return;
    }

    xcb_generic_error_t *err = nullptr;
    auto reply = XCB_REPLY(xcb_xfixes_query_version,
                           m_conn,
                           XCB_XFIXES_MAJOR_VERSION,
                           XCB_XFIXES_MINOR_VERSION);
    if (err != nullptr) {
        spdlog::warn("xcb_xfixes_query_version: {}", err->error_code);
        return;
    }

    spdlog::debug("Xfixes version {}.{}", reply->major_version, reply->minor_version);
}

void Display::handleEvent(std::shared_ptr<xcb_generic_event_t> event) {
    auto response_type = event->response_type & ~0x80;
    switch (response_type) {
    case XCB_CONFIGURE_NOTIFY: {
        auto *cne = reinterpret_cast<xcb_configure_notify_event_t *>(event.get());
        handleScreenSizeChange(cne->width, cne->height);

        break;
    }
    case XCB_GE_GENERIC: {
        auto *ge = reinterpret_cast<xcb_ge_generic_event_t *>(event.get());

        if (edgeDetectionStarted() && ge->extension == m_xinput2OPCode &&
            ge->event_type == XCB_INPUT_MOTION) {
            auto reply = XCB_REPLY(xcb_query_pointer, m_conn, m_screen->root);
            if (!reply) {
                spdlog::warn("failed to query pointer");
                break;
            }

            handleMotion(reply->root_x, reply->root_y);
        }

        break;
    }
    }
}

void Display::hideMouse(bool hide) {
#if 0 // TODO this operation does not work on some pc machines
    if (hide) {
        xcb_void_cookie_t cookie = xcb_xfixes_hide_cursor_checked(m_conn, m_screen->root);
        xcb_generic_error_t *err = xcb_request_check(m_conn, cookie);
        if (err != nullptr) {
            spdlog::error("failed to hide cursor: {}", err->error_code);
        }
    } else {
        xcb_void_cookie_t cookie = xcb_xfixes_show_cursor_checked(m_conn, m_screen->root);
        xcb_generic_error_t *err = xcb_request_check(m_conn, cookie);
        if (err != nullptr) {
            spdlog::error("failed to show cursor: {}", err->error_code);
        }
    }
#endif
}

void Display::moveMouse(uint16_t x, uint16_t y) {
    xcb_void_cookie_t
        cookie = xcb_warp_pointer_checked(m_conn, XCB_NONE, m_screen->root, 0, 0, 0, 0, x, y);
    xcb_generic_error_t *err = xcb_request_check(m_conn, cookie);
    if (err != nullptr) {
        spdlog::error("failed to move cursor: {}", err->error_code);
    }
}

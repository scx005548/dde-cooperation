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

#include "utils/ptr.h"

#define XCB_REPLY_CONNECTION_ARG(connection, ...) connection
#define XCB_REPLY(call, ...)                                                                       \
    std::unique_ptr<call##_reply_t>(                                                               \
        call##_reply(XCB_REPLY_CONNECTION_ARG(__VA_ARGS__), call(__VA_ARGS__), nullptr))

X11::X11(Manager *manager)
    : DisplayServer(manager) {
    m_conn = xcb_connect(nullptr, &m_screenDefaultNbr);

    if (int err = xcb_connection_has_error(m_conn)) {
        throw std::runtime_error(fmt::format("failed to connect to X11: {}", err));
    }

    m_screen = screenOfDisplay(m_screenDefaultNbr);

    initXinputExtension();
    initXfixesExtension();

    uint32_t mask = XCB_EVENT_MASK_STRUCTURE_NOTIFY;
    xcb_change_window_attributes(m_conn, m_screen->root, XCB_CW_EVENT_MASK, &mask);

    handleScreenSizeChange(m_screen->width_in_pixels, m_screen->height_in_pixels);
}

xcb_screen_t *X11::screenOfDisplay(int screen) {
    xcb_screen_iterator_t iter = xcb_setup_roots_iterator(xcb_get_setup(m_conn));
    for (; iter.rem; --screen, xcb_screen_next(&iter)) {
        if (screen == 0) {
            return iter.data;
        }
    }

    return nullptr;
}

void X11::initRandrExtension() {
    const xcb_query_extension_reply_t *reply = xcb_get_extension_data(m_conn, &xcb_randr_id);
    if (!reply->present) {
        throw std::runtime_error("randr extension not found");
    }
}

void X11::initXinputExtension() {
    {
        const char *extname = "XInputExtension";
        auto reply = XCB_REPLY(xcb_query_extension, m_conn, strlen(extname), extname);
        if (!reply->present) {
            throw std::runtime_error("XInput extension is not avaliable");
        }
        m_xinput2OPCode = reply->major_opcode;
    }

    {
        auto reply = XCB_REPLY(xcb_input_xi_query_version, m_conn, 2, 0);
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

void X11::initXfixesExtension() {
    xcb_xfixes_query_version_cookie_t cookie = xcb_xfixes_query_version(m_conn, 4, 0);
    xcb_generic_error_t *err = nullptr;
    xcb_xfixes_query_version_reply_t *reply = xcb_xfixes_query_version_reply(m_conn, cookie, &err);
    if (err != nullptr) {
        spdlog::warn("xcb_xfixes_query_version: {}", err->error_code);
        return;
    }

    spdlog::debug("Xfixes version {}.{}", reply->major_version, reply->minor_version);
}

void X11::start() {
    xcb_generic_event_t *event;
    while ((event = xcb_wait_for_event(m_conn))) {
        switch (event->response_type) {
        case XCB_CONFIGURE_NOTIFY: {
            xcb_configure_notify_event_t *cne = reinterpret_cast<xcb_configure_notify_event_t *>(
                event);
            handleScreenSizeChange(cne->width, cne->height);
            break;
        }
        case XCB_GE_GENERIC: {
            xcb_ge_generic_event_t *ge = reinterpret_cast<xcb_ge_generic_event_t *>(event);

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

        free(event);
    }
}

void X11::hideMouse(bool hide) {
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
}

void X11::moveMouse(uint16_t x, uint16_t y) {
    xcb_void_cookie_t
        cookie = xcb_warp_pointer_checked(m_conn, XCB_NONE, m_screen->root, 0, 0, 0, 0, x, y);
    xcb_generic_error_t *err = xcb_request_check(m_conn, cookie);
    if (err != nullptr) {
        spdlog::error("failed to move cursor: {}", err->error_code);
    }
}

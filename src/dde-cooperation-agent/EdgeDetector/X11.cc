#include "X11.h"

#include <stdexcept>

#include <cstdlib>
#include <cstdio>

#include <xcb/xcb.h>
#include <xcb/randr.h>
#include <xcb/xcb_aux.h>
#include <X11/extensions/XInput2.h>

#include <fmt/core.h>
#include <spdlog/spdlog.h>

#include "utils/ptr.h"

X11::X11()
    : m_dpy(make_handle(XOpenDisplay(nullptr), &XCloseDisplay))
    , m_conn(XGetXCBConnection(m_dpy.get())) {
    if (int err = xcb_connection_has_error(m_conn)) {
        throw std::runtime_error(fmt::format("failed to connect to X11: {}", err));
    }

    m_screen = xcb_aux_get_screen(m_conn, DefaultScreen(m_dpy.get()));
    if (m_screen == nullptr) {
        throw std::runtime_error("failed to get screen");
    }

    const xcb_query_extension_reply_t *reply = xcb_get_extension_data(m_conn, &xcb_randr_id);
    if (!reply->present) {
        throw std::runtime_error("randr extension not found");
    }

    int xiEventBase;
    int xiErrorBase;
    if (!XQueryExtension(m_dpy.get(),
                         "XInputExtension",
                         &m_xinput2OPCode,
                         &xiEventBase,
                         &xiErrorBase)) {
        throw std::runtime_error("XInput extension is not avaliable");
    }

    int xiMajor = 2;
    int xiMinor = 2;
    int res = XIQueryVersion(m_dpy.get(), &xiMajor, &xiMinor);
    if (res != Success) {
        if (res == BadRequest) {
            SPDLOG_ERROR("X server does not support XInput 2");
        }
        SPDLOG_ERROR("internal error");
    }

    SPDLOG_DEBUG("XInput version {}.{}", xiMajor, xi2Minor);

    uint32_t mask = XCB_EVENT_MASK_STRUCTURE_NOTIFY;
    xcb_change_window_attributes(m_conn, m_screen->root, XCB_CW_EVENT_MASK, &mask);

    handleScreenSizeChange(m_screen->width_in_pixels, m_screen->height_in_pixels);
}

void X11::start() {
    XIEventMask mask;
    mask.deviceid = XIAllDevices;
    mask.mask_len = XIMaskLen(XI_LASTEVENT);
    unsigned char maskM[XIMaskLen(XI_LASTEVENT)] = {0};
    mask.mask = maskM;

    XISetMask(mask.mask, XI_HierarchyChanged);
    XISetMask(mask.mask, XI_Motion);

    XISelectEvents(m_dpy.get(), DefaultRootWindow(m_dpy.get()), &mask, 1);
    XSync(m_dpy.get(), false);

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

            if (ge->extension == m_xinput2OPCode && ge->event_type == XI_Motion) {
                xcb_generic_error_t *err = nullptr;
                auto cookie = xcb_query_pointer(m_conn, m_screen->root);
                auto rep = std::unique_ptr<xcb_query_pointer_reply_t>(
                    xcb_query_pointer_reply(m_conn, cookie, &err));
                if (err != nullptr) {
                    SPDLOG_WARN("failed to query pointer: {}", err->error_code);
                    break;
                }

                handleMotion(rep->root_x, rep->root_y);
            }
            break;
        }
        }
    }
}

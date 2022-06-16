#include "InputEvent.h"

#include <cstring>

#include <spdlog/spdlog.h>
#include <libevdev/libevdev.h>
#include <libevdev/libevdev-uinput.h>

#include <utils/ptr.h>

InputEvent::InputEvent()
    : m_dev(make_handle(libevdev_new(), &libevdev_free))
    , m_uidev(nullptr, &libevdev_uinput_destroy) {
    libevdev_set_name(m_dev.get(), "DDE Cooperation");

    int rc;
    rc = libevdev_enable_event_type(m_dev.get(), EV_SYN);
    if (rc != 0) {
        SPDLOG_WARN("failed to enable event type EV_SYN");
    }

    rc = libevdev_enable_event_type(m_dev.get(), EV_KEY);
    if (rc != 0) {
        SPDLOG_WARN("failed to enable event type EV_KEY");
    }

    rc = libevdev_enable_event_type(m_dev.get(), EV_LED);
    if (rc != 0) {
        SPDLOG_WARN("failed to enable event type EV_LED");
    }

    for (unsigned int code = 1; code < KEY_MAX; code++) {
        rc = libevdev_enable_event_code(m_dev.get(), EV_KEY, code, nullptr);
        if (rc != 0) {
            SPDLOG_WARN("failed to enable event code: type: EV_KEY, code: {}",
                        libevdev_event_code_get_name(EV_KEY, code));
        }
    }

    libevdev_uinput *uidev;
    rc = libevdev_uinput_create_from_device(m_dev.get(), LIBEVDEV_UINPUT_OPEN_MANAGED, &uidev);
    if (rc != 0) {
        SPDLOG_ERROR("failed to create uinput device: {}", strerror(-rc));
        return;
    }

    m_uidev = make_handle(uidev, &libevdev_uinput_destroy);
}

bool InputEvent::emit(const InputEventRequest &event) {
    int rc = libevdev_uinput_write_event(m_uidev.get(), event.type(), event.code(), event.value());
    if (rc != 0) {
        SPDLOG_WARN("failed to write event: {}", strerror(-rc));
        return false;
    }

    return true;
}

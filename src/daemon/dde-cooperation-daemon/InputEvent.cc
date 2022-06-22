#include "InputEvent.h"

#include <cstring>

#include <spdlog/spdlog.h>
#include <libevdev/libevdev.h>
#include <libevdev/libevdev-uinput.h>

#include "utils/ptr.h"

extern std::shared_ptr<spdlog::logger> logger;

InputEvent::InputEvent(DeviceType type)
    : m_dev(make_handle(libevdev_new(), &libevdev_free))
    , m_uidev(nullptr, &libevdev_uinput_destroy) {
    std::string name("DDE Cooperation ");

    enableEventType(EV_SYN);
    enableEventType(EV_KEY);

    switch (type) {
    case DeviceType::Keyboard: {
        name.append("Keyboard");
        enableEventType(EV_LED);
        enableEventType(EV_REP);

        for (unsigned int code = 1; code < KEY_MAX; code++) {
            enableEventCode(EV_KEY, code);
        }

        for (unsigned int code = 0; code < REP_MAX; code++) {
            enableEventCode(EV_REP, code);
        }
        break;
    }
    case DeviceType::Mouse: {
        name.append("Mouse");
        enableEventType(EV_REL);
        enableEventCode(EV_REL, REL_X);
        enableEventCode(EV_REL, REL_Y);
        enableEventCode(EV_REL, REL_HWHEEL);
        enableEventCode(EV_REL, REL_WHEEL);

        enableEventCode(EV_KEY, BTN_LEFT);
        enableEventCode(EV_KEY, BTN_RIGHT);
        enableEventCode(EV_KEY, BTN_MIDDLE);
        enableEventCode(EV_KEY, BTN_SIDE);
        enableEventCode(EV_KEY, BTN_EXTRA);
        break;
    }
    default:
        SPDLOG_WARN("unknown device type: {}", type);
        break;
    }

    libevdev_set_name(m_dev.get(), name.c_str());

    libevdev_uinput *uidev;
    int rc = libevdev_uinput_create_from_device(m_dev.get(), LIBEVDEV_UINPUT_OPEN_MANAGED, &uidev);
    if (rc != 0) {
        logger->error("failed to create uinput device: {}", strerror(-rc));
        return;
    }

    m_uidev = make_handle(uidev, &libevdev_uinput_destroy);
}

void InputEvent::enableEventType(unsigned int type) {
    int rc = libevdev_enable_event_type(m_dev.get(), type);
    if (rc != 0) {
        logger->warn("failed to enable event type {}", libevdev_event_type_get_name(type));
    }
}

void InputEvent::enableEventCode(unsigned int type, unsigned int code, const void *data) {
    int rc = libevdev_enable_event_code(m_dev.get(), type, code, data);
    if (rc != 0) {
        logger->warn("failed to enable event code: type: {}, code: {}",
                     libevdev_event_type_get_name(type),
                     libevdev_event_code_get_name(type, code));
    }
}

bool InputEvent::emit(const InputEventRequest &event) {
    int rc = libevdev_uinput_write_event(m_uidev.get(), event.type(), event.code(), event.value());
    if (rc != 0) {
        logger->warn("failed to write event: {}", strerror(-rc));
        return false;
    }

    return true;
}

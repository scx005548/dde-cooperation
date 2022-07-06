#include "InputDevice.h"

#include <filesystem>

#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>

#include <libevdev/libevdev.h>
#include <spdlog/spdlog.h>

#include "Machine.h"
#include "uvxx/Poll.h"

namespace fs = std::filesystem;

InputDevice::InputDevice(const fs::path &path, const std::shared_ptr<uvxx::Loop> &uvLoop)
    : m_path(path)
    , m_fd(open(m_path.c_str(), O_RDONLY | O_NONBLOCK))
    , m_uvLoop(uvLoop) {

    int rc = libevdev_new_from_fd(m_fd, &m_dev);
    if (rc < 0) {
        spdlog::error("failed to init libevdev {}", strerror(-rc));
        exit(1);
    }

    m_name = libevdev_get_name(m_dev);

    if (libevdev_has_event_type(m_dev, EV_KEY) && libevdev_has_event_type(m_dev, EV_REP)) {
        m_type = DeviceType::Keyboard;
    } else if (libevdev_has_event_type(m_dev, EV_REL) &&
               libevdev_has_event_code(m_dev, EV_KEY, BTN_LEFT)) {
        m_type = DeviceType::Mouse;
    } else if (libevdev_has_event_type(m_dev, EV_ABS) &&
               !libevdev_has_property(m_dev, INPUT_PROP_DIRECT)) {
        m_type = DeviceType::Touchpad;
    }
}

InputDevice::~InputDevice() {
    spdlog::debug("InputDevice::~InputDevice() {}", m_name);
    libevdev_free(m_dev);
    close(m_fd);
}

bool InputDevice::shouldIgnore() {
    if (m_name.rfind("DDE Cooperation", 0) == 0) {
        return true;
    }

    // TODO: allow touchpad devices
    if (m_type == DeviceType::Mouse || m_type == DeviceType::Keyboard) {
        return false;
    }

    return true;
}

void InputDevice::init() {
    m_uvPoll = std::make_shared<uvxx::Poll>(m_uvLoop, m_fd);
    m_uvPoll->onEvent(uvxx::memFunc(this, &InputDevice::handlePollEvent));
}

void InputDevice::start() {
    m_uvPoll->start(UV_READABLE);

    int rc = libevdev_grab(m_dev, LIBEVDEV_GRAB);
    if (rc != 0) {
        spdlog::error("failed to grab device {}", strerror(-rc));
        return;
    }
}

void InputDevice::stop() {
    spdlog::debug("stopping input device: {}", m_name);
    m_uvPoll->stop();
    libevdev_grab(m_dev, LIBEVDEV_UNGRAB);
    spdlog::debug("stopped");
}

void InputDevice::handlePollEvent([[maybe_unused]] int events) {
    input_event ev;
    int rc;
    do {
        rc = libevdev_next_event(m_dev, LIBEVDEV_READ_FLAG_NORMAL, &ev);
        if (rc == LIBEVDEV_READ_STATUS_SUCCESS) {
            spdlog::debug("event: {}, {}, {}",
                          libevdev_event_type_get_name(ev.type),
                          libevdev_event_code_get_name(ev.type, ev.code),
                          ev.value);

            InputEventRequest event;
            event.set_devicetype(m_type);
            event.set_type(ev.type);
            event.set_code(ev.code);
            event.set_value(ev.value);
            if (auto machine = m_machine.lock()) {
                machine->handleInputEvent(event);
            } else {
                spdlog::warn("no machine");
            }
        }
    } while (rc != -EAGAIN);
}

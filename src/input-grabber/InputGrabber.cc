#include "InputGrabber.h"

#include <filesystem>

#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>

#include <libevdev/libevdev.h>
#include <spdlog/spdlog.h>

#include "uvxx/Poll.h"

namespace fs = std::filesystem;

InputGrabber::InputGrabber(const std::shared_ptr<uvxx::Loop> &uvLoop, const fs::path &path)
    : m_path(path)
    , m_fd(open(m_path.c_str(), O_RDONLY | O_NONBLOCK))
    , m_uvLoop(uvLoop)
    , m_uvPoll(std::make_shared<uvxx::Poll>(m_uvLoop, m_fd)) {

    int rc = libevdev_new_from_fd(m_fd, &m_dev);
    if (rc < 0) {
        spdlog::error("failed to init libevdev {}", strerror(-rc));
        exit(1);
    }

    m_name = libevdev_get_name(m_dev);

    if (libevdev_has_event_type(m_dev, EV_KEY) && libevdev_has_event_type(m_dev, EV_REP)) {
        m_type = InputDeviceType::KEYBOARD;
    } else if (libevdev_has_event_type(m_dev, EV_REL) &&
               libevdev_has_event_code(m_dev, EV_KEY, BTN_LEFT)) {
        m_type = InputDeviceType::MOUSE;
    } else if (libevdev_has_event_type(m_dev, EV_ABS) &&
               !libevdev_has_property(m_dev, INPUT_PROP_DIRECT)) {
        m_type = InputDeviceType::TOUCHPAD;
    }

    m_uvPoll->onEvent(uvxx::memFunc(this, &InputGrabber::handlePollEvent));
}

InputGrabber::~InputGrabber() {
    spdlog::debug("InputDevice::~InputDevice() {}", m_name);
    libevdev_free(m_dev);
    close(m_fd);
}

bool InputGrabber::shouldIgnore() {
    if (m_name.rfind("DDE Cooperation", 0) == 0) {
        return true;
    }

    // TODO: allow touchpad devices
    if (m_type == InputDeviceType::KEYBOARD || m_type == InputDeviceType::MOUSE ||
        m_type == InputDeviceType::TOUCHPAD) {
        return false;
    }

    return true;
}

void InputGrabber::start() {
    // clear already existed events
    input_event ev;
    while (-EAGAIN != libevdev_next_event(m_dev, LIBEVDEV_READ_FLAG_NORMAL, &ev));

    m_uvPoll->start(UV_READABLE);

    int rc = libevdev_grab(m_dev, LIBEVDEV_GRAB);
    if (rc != 0) {
        spdlog::error("failed to grab device {}", strerror(-rc));
        return;
    }
}

void InputGrabber::stop() {
    spdlog::debug("stopping input device: {}", m_name);
    m_uvPoll->stop();
    libevdev_grab(m_dev, LIBEVDEV_UNGRAB);
    spdlog::debug("stopped");
}

void InputGrabber::handlePollEvent([[maybe_unused]] int events) {
    input_event ev;
    int rc;
    do {
        rc = libevdev_next_event(m_dev, LIBEVDEV_READ_FLAG_NORMAL, &ev);
        if (rc == LIBEVDEV_READ_STATUS_SUCCESS) {
            spdlog::debug("event: {}, {}, {}",
                          libevdev_event_type_get_name(ev.type),
                          libevdev_event_code_get_name(ev.type, ev.code),
                          ev.value);

            m_onEvent(ev.type, ev.code, ev.value);
        }
    } while (rc != -EAGAIN);
}

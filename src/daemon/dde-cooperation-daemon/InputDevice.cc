#include "InputDevice.h"

#include <filesystem>

#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>

#include <libevdev/libevdev.h>
#include <spdlog/spdlog.h>

#include "Machine.h"

namespace fs = std::filesystem;

InputDevice::InputDevice(const fs::path &path)
    : m_path(path) {
    m_fd = open(m_path.c_str(), O_RDONLY | O_NONBLOCK);

    int rc = libevdev_new_from_fd(m_fd, &m_dev);
    if (rc < 0) {
        spdlog::error("failed to init libevdev {}", strerror(-rc));
        exit(1);
    }

    if (libevdev_has_event_type(m_dev, EV_KEY) && libevdev_has_event_type(m_dev, EV_REP)) {
        m_type = DeviceType::Keyboard;
    } else if (libevdev_has_event_type(m_dev, EV_REL) &&
               libevdev_has_event_code(m_dev, EV_KEY, BTN_LEFT)) {
        m_type = DeviceType::Mouse;
    }
}

InputDevice::~InputDevice() {
    libevdev_free(m_dev);
    close(m_fd);
}

void InputDevice::start() {
    m_stop = false;
    int rc = libevdev_grab(m_dev, LIBEVDEV_GRAB);
    if (rc != 0) {
        spdlog::error("failed to grab device {}", strerror(-rc));
        return;
    }

    // TODO: epoll
    m_thread = std::thread([this] {
        int rc = 0;
        do {
            input_event ev;
            rc = libevdev_next_event(m_dev, LIBEVDEV_READ_FLAG_NORMAL, &ev);
            if (rc == 0) {
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
                    m_stop = true;
                }
            }
        } while (m_stop == false || rc == 1 || rc == 0 || rc == -EAGAIN);

        libevdev_grab(m_dev, LIBEVDEV_UNGRAB);
    });
}

void InputDevice::stop() {
    m_stop = true;
    m_thread.join();
}

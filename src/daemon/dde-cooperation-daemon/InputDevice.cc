#include "InputDevice.h"

#include <filesystem>

#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>

#include <libevdev/libevdev.h>
#include <spdlog/spdlog.h>

extern std::shared_ptr<spdlog::logger> logger;

namespace fs = std::filesystem;

InputDevice::InputDevice(const fs::path &path)
    : m_path(path) {
    m_fd = open(m_path.c_str(), O_RDONLY | O_NONBLOCK);

    int rc = libevdev_new_from_fd(m_fd, &m_dev);
    if (rc < 0) {
        logger->error("failed to init libevdev {}", strerror(-rc));
        exit(1);
    }

    printf("Input device name: \"%s\"\n", libevdev_get_name(m_dev));
    printf("Input device ID: bus %#x vendor %#x product %#x\n",
           libevdev_get_id_bustype(m_dev),
           libevdev_get_id_vendor(m_dev),
           libevdev_get_id_product(m_dev));
    if (libevdev_has_event_type(m_dev, EV_REL) &&
        libevdev_has_event_code(m_dev, EV_KEY, BTN_LEFT)) {
        m_isMouse = true;
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
        logger->error("failed to grab device {}", strerror(-rc));
        return;
    }

    // TODO: epoll
    m_thread = std::thread([this] {
        int rc = 0;
        do {
            input_event ev;
            rc = libevdev_next_event(m_dev, LIBEVDEV_READ_FLAG_NORMAL, &ev);
            if (rc == 0) {
                logger->debug("event: {}, {}, {}",
                              libevdev_event_type_get_name(ev.type),
                              libevdev_event_code_get_name(ev.type, ev.code),
                              ev.value);

                InputEventRequest event;
                event.set_type(ev.type);
                event.set_code(ev.code);
                event.set_value(ev.value);
                m_signal_inputEvent.emit(event);
            }
        } while (m_stop == false || rc == 1 || rc == 0 || rc == -EAGAIN);

        libevdev_grab(m_dev, LIBEVDEV_UNGRAB);
    });
}

void InputDevice::stop() {
    m_stop = true;
    m_thread.join();
}

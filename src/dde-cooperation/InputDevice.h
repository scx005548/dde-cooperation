#ifndef INPUT_DEVICE_H
#define INPUT_DEVICE_H

#include <filesystem>
#include <thread>

#include <libevdev/libevdev.h>
#include <giomm.h>

#include "protocol/input_event.pb.h"

class InputDevice {
public:
    explicit InputDevice(const std::filesystem::path &path);
    ~InputDevice();

    void start();
    void stop();

    using type_signal_inputEvent = sigc::signal<void(const InputEventRequest &)>;
    type_signal_inputEvent inputEvent() { return m_signal_inputEvent; }

private:
    int m_fd;

    type_signal_inputEvent m_signal_inputEvent;

    const std::filesystem::path &m_path;
    libevdev *m_dev;

    bool m_isMouse;

    std::thread m_thread;
    bool m_stop;
};

#endif // !INPUT_DEVICE_H

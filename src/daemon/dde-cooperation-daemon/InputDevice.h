#ifndef INPUT_DEVICE_H
#define INPUT_DEVICE_H

#include <filesystem>
#include <thread>

#include <libevdev/libevdev.h>
#include <giomm.h>

#include "protocol/input_event.pb.h"

class Machine;

class InputDevice {
public:
    explicit InputDevice(const std::filesystem::path &path);
    ~InputDevice();

    void start();
    void stop();

    DeviceType type() { return m_type; };

    void setMachine(const std::weak_ptr<Machine> &machine) { m_machine = machine; }

private:
    std::weak_ptr<Machine> m_machine;

    int m_fd;

    const std::filesystem::path &m_path;
    libevdev *m_dev;

    DeviceType m_type;

    std::thread m_thread;
    bool m_stop;
};

#endif // !INPUT_DEVICE_H

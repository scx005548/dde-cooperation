#ifndef INPUT_DEVICE_H
#define INPUT_DEVICE_H

#include <filesystem>
#include <thread>

#include <libevdev/libevdev.h>
#include <giomm.h>

#include "protocol/cooperation.pb.h"

namespace uvxx {
class Loop;
class Poll;
} // namespace uvxx

class Machine;

class InputDevice {
public:
    explicit InputDevice(const std::filesystem::path &path,
                         const std::shared_ptr<uvxx::Loop> &uvLoop);
    ~InputDevice();

    void init();
    void start();
    void stop();

    bool shouldIgnore();
    DeviceType type() { return m_type; };

    void setMachine(const std::weak_ptr<Machine> &machine) { m_machine = machine; }

private:
    std::weak_ptr<Machine> m_machine;

    const std::filesystem::path &m_path;

    int m_fd;

    std::shared_ptr<uvxx::Loop> m_uvLoop;
    std::shared_ptr<uvxx::Poll> m_uvPoll;

    libevdev *m_dev;

    std::string m_name;
    DeviceType m_type;

    void handlePollEvent(int events);
};

#endif // !INPUT_DEVICE_H

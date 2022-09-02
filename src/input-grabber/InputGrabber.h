#ifndef INPUTEGRABBER_H
#define INPUTEGRABBER_H

#include <filesystem>
#include <thread>
#include <functional>

#include <libevdev/libevdev.h>

#include "common.h"

namespace uvxx {
class Loop;
class Poll;
} // namespace uvxx

class Machine;

class InputGrabber {
public:
    explicit InputGrabber(const std::shared_ptr<uvxx::Loop> &uvLoop,
                          const std::filesystem::path &path);
    ~InputGrabber();

    bool shouldIgnore();

    void onEvent(const std::function<void(unsigned int type, unsigned int code, int value)> &cb) {
        m_onEvent = cb;
    }

    void start();
    void stop();

    InputDeviceType type() { return m_type; };

private:
    const std::filesystem::path &m_path;
    int m_fd;

    std::shared_ptr<uvxx::Loop> m_uvLoop;
    std::shared_ptr<uvxx::Poll> m_uvPoll;

    libevdev *m_dev;

    std::string m_name;
    InputDeviceType m_type;

    std::function<void(unsigned int type, unsigned int code, int value)> m_onEvent;

    void handlePollEvent(int events);
};

#endif // !INPUTEGRABBER_H

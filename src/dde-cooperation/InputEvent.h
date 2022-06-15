#ifndef INPUT_EVENT_H
#define INPUT_EVENT_H

#include <libevdev/libevdev-uinput.h>

#include "protocol/input_event.pb.h"

class InputEvent {
public:
    InputEvent();

    bool emit(const InputEventRequest &event);

private:
    std::unique_ptr<libevdev, decltype(&libevdev_free)> m_dev;
    std::unique_ptr<libevdev_uinput, decltype(&libevdev_uinput_destroy)> m_uidev;
};

#endif // !INPUT_EVENT_H

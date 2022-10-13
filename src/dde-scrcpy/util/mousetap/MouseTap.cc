#include <QtGlobal>

#include "MouseTap.h"
#include "XMouseTap.h"

MouseTap *MouseTap::s_instance = Q_NULLPTR;
MouseTap *MouseTap::getInstance() {
    if (s_instance == Q_NULLPTR) {
        s_instance = new XMouseTap();
    }
    return s_instance;
}

MouseTap::MouseTap() {
}

MouseTap::~MouseTap() {
}

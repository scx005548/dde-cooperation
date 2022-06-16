#include "EdgeDetector.h"

#include <spdlog/spdlog.h>

void EdgeDetector::handleScreenSizeChange(int16_t w, int16_t h) {
    m_screenWidth = w;
    m_screenHeight = h;
}

void EdgeDetector::handleMotion(int16_t x, int16_t y) {
    if (m_lastX == x) {
        if (x == 0) {
            // left flow
            SPDLOG_INFO("left flow");
        }
        if (x == m_screenWidth - 1) {
            // right flow
            SPDLOG_INFO("right flow");
        }
    }

    if (m_lastY == y) {
        if (y == 0) {
            // top flow
            SPDLOG_INFO("top flow");
        }

        if (y == m_screenHeight - 1) {
            // bottom flow
            SPDLOG_INFO("bottom flow");
        }
    }

    m_lastX = x;
    m_lastY = y;
}

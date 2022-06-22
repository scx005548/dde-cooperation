#include "EdgeDetector.h"

#include <spdlog/spdlog.h>

extern std::shared_ptr<spdlog::logger> logger;

void EdgeDetector::handleScreenSizeChange(int16_t w, int16_t h) {
    m_screenWidth = w;
    m_screenHeight = h;
}

void EdgeDetector::handleMotion(int16_t x, int16_t y) {
    if (m_lastX == x) {
        if (x == 0) {
            // left flow
            logger->info("left flow");
        }
        if (x == m_screenWidth - 1) {
            // right flow
            logger->info("right flow");
        }
    }

    if (m_lastY == y) {
        if (y == 0) {
            // top flow
            logger->info("top flow");
        }

        if (y == m_screenHeight - 1) {
            // bottom flow
            logger->info("bottom flow");
        }
    }

    m_lastX = x;
    m_lastY = y;
}

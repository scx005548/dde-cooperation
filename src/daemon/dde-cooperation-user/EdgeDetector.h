#ifndef DDE_COOPERATION_USER_EDGE_DETECTOR_H
#define DDE_COOPERATION_USER_EDGE_DETECTOR_H

#include <vector>

#include <cstdint>

class EdgeDetector {
public:
    virtual ~EdgeDetector() = default;

    virtual void start() = 0;

protected:
    void handleScreenSizeChange(int16_t w, int16_t h);
    void handleMotion(int16_t x, int16_t y);

private:
    uint16_t m_screenWidth;
    uint16_t m_screenHeight;

    uint16_t m_lastX;
    uint16_t m_lastY;
};

#endif // !DDE_COOPERATION_USER_EDGE_DETECTOR_H

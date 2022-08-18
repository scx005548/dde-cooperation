#ifndef DDE_COOPERATION_DEAMON_DISPLAYBASE_H
#define DDE_COOPERATION_DEAMON_DISPLAYBASE_H

#include <vector>

#include <cstdint>

class Manager;

class DisplayBase {
public:
    explicit DisplayBase(Manager *manager);
    virtual ~DisplayBase() = default;

    void startEdgeDetection() { m_startEdgeDetection = true; };
    void stopEdgeDetection() { m_startEdgeDetection = false; };
    void flowBack(uint16_t direction, uint16_t x, uint16_t y);

protected:
    virtual void hideMouse(bool hide) = 0;
    virtual void moveMouse(uint16_t x, uint16_t y) = 0;

    bool edgeDetectionStarted() { return m_startEdgeDetection; }

    void handleScreenSizeChange(int16_t w, int16_t h);
    void handleMotion(int16_t x, int16_t y);

private:
    Manager *m_manager;

    uint16_t m_screenWidth;
    uint16_t m_screenHeight;

    bool m_startEdgeDetection;

    uint16_t m_lastX;
    uint16_t m_lastY;

    void flowOut(uint16_t direction, uint16_t x, uint16_t y);
};

#endif // !DDE_COOPERATION_DEAMON_DISPLAYBASE_H

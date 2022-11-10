#ifndef DECODER_H
#define DECODER_H
#include <QObject>

extern "C" {
#include "libavcodec/avcodec.h"
}

#include <functional>

class QVideoFrame;

class VideoBuffer;
class Decoder : public QObject {
    Q_OBJECT
public:
    Decoder(std::function<void(const QVideoFrame &frame)> onFrame, QObject *parent = Q_NULLPTR);
    virtual ~Decoder();

    bool open();
    void close();
    bool push(const AVPacket *packet);
    void peekFrame(std::function<void(int width, int height, uint8_t *dataRGB32)> onFrame);

signals:
    void updateFPS(quint32 fps);

private slots:
    void onNewFrame();

signals:
    void newFrame();

private:
    void pushFrame();

private:
    VideoBuffer *m_vb = Q_NULLPTR;
    AVCodecContext *m_codecCtx = Q_NULLPTR;
    bool m_isCodecCtxOpen = false;
    std::function<void(const QVideoFrame &frame)> m_onFrame = Q_NULLPTR;

    bool renderFrame(const AVFrame *frame);
};

#endif // DECODER_H

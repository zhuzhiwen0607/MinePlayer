#ifndef DECODER_H
#define DECODER_H

#include <QThread>
#include <QQueue>
#include <QTimer>
#include <QMutex>
#include "reader.h"

extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavutil/imgutils.h"
#include "libswscale/swscale.h"
}

class VideoDecoder : public QThread
{
    Q_OBJECT

public:
    typedef struct
    {
        int taskId;
        Reader *reader;

    } CONFIG;

public:
    VideoDecoder();
    ~VideoDecoder();

    bool Init(CONFIG &config);
    void Start();
    bool WriteHeader();

    AVFrame* GetFrame();
    void FreeFrame(AVFrame *frame);

    int GetVideoWidth() const;
    int GetVideoHeight() const;
    int GetPixelFormat() const;

signals:
    void SigRenderVideo();

public slots:
//    void OnNewPacket();
    void OnDecode();

protected:
    virtual void run() override;

private:
//    WriteH264Header();

private:
    CONFIG mConfig;
    QTimer mDecodeTimer;

    const AVCodec *mCodec = nullptr;
    AVCodecContext *mCodecCtx = nullptr;
    AVFormatContext *mFmtCtx = nullptr;
    SwsContext *mSwsContext = nullptr;
    AVBufferRef *mHWContext = nullptr;

    int mVideoStreamIndex;

    QMutex mFramesMutex;
    QQueue<AVFrame*> mFrames;


    AVFormatContext *mOutFmtCtx = nullptr;    // test
    AVCodecParserContext *mParser = nullptr;
};

#endif // DECODER_H

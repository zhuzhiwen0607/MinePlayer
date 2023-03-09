#ifndef AUDIODECODER_H
#define AUDIODECODER_H

#include "reader.h"

#include <QThread>
#include <QTimer>
#include <QAudioDeviceInfo>


extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"

//#include "libavutil/channel_layout.h"
}
class AudioDecoder : public QThread
{
    Q_OBJECT

public:
    typedef struct
    {
        int taskId;
        Reader *reader;
    } CONFIG;

public:
    AudioDecoder();
    ~AudioDecoder();

    bool Init(CONFIG &config);
    void Start();
    AVCodecContext* GetDecoderContext();

    AVFrame* GetFrame();
    void FreeFrame(AVFrame *frame);



signals:
    void SigPlayback();

public slots:
    void OnDecode();

protected:
    virtual void run() override;

private:
    CONFIG mConfig;

    QTimer mDecodeTimer;

    QAudioDeviceInfo mDeviceInfo;

    int mAudioStreamIndex;
    AVFormatContext *mFmtCtx = nullptr;
    AVCodecContext *mCodecCtx = nullptr;
    const AVCodec *mCodec = nullptr;
//    SwrContext *mSwrCtx = nullptr;

    AVPacket *mPacket = nullptr;

    QMutex mFramesMutex;
    QQueue<AVFrame*> mFrames;



};

#endif // AUDIODECODER_H

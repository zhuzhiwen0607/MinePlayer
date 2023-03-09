#ifndef AUDIOPLAYBACK_H
#define AUDIOPLAYBACK_H

#include <QThread>
#include <QAudioOutput>

#include "audiodecoder.h"

extern "C"
{
#include "libavutil/opt.h"
#include "libswresample/swresample.h"
}

class AudioPlayback : public QThread
{
    Q_OBJECT

public:
    typedef struct
    {
        int taskId;
        AudioDecoder *decoder;
    } CONFIG;

public:
    AudioPlayback();
    ~AudioPlayback();
    bool Init(CONFIG &config);
    void Start();

public slots:
    void OnPlayback();

protected:
    virtual void run() override;

private:
    void Resample(AVFrame *frame, uint8_t ***outData, int &outSize);

private:
    CONFIG mConfig;

//    QAudioDeviceInfo mDeviceInfo;
    QAudioOutput *mAudioOutput = nullptr;
    QIODevice *mAudioDevice = nullptr;


    SwrContext *mSwrCtx = nullptr;
    int mOutChannels;
    int mOutSampleRate;
    AVSampleFormat mOutFormat;

};

#endif // AUDIOPLAYBACK_H

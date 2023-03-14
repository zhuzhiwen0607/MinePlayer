#ifndef PLAYENGINE_H
#define PLAYENGINE_H

#include <windows.h>
//#include <QThread>
//#include <QTimer>
//#include <QQueue>
//#include <QMutex>
#include "renderview.h"
#include "reader.h"
#include "videodecoder.h"
#include "videorender.h"
#include "audiodecoder.h"
#include "audioplayback.h"
#include "basetime.h"

//extern "C"
//{
//#include "libavformat/avformat.h"
//}

class PlayEngine
{
//    Q_OBJECT

public:
    typedef struct
    {
        QString fileName;
        RenderView *renderView;
    } CONFIG;



public:
    PlayEngine();
    ~PlayEngine();

    bool Init(CONFIG &config);

    void Play();
    void Pause();
    void Stop();


private:
    CONFIG mConfig;

//    BaseTime *mBaseTime;

    Reader mReader;
    VideoDecoder mVideoDecoder;
    VideoRender mVideoRender;
    AudioDecoder mAudioDecoder;
    AudioPlayback mAudioPlayback;
};

#endif // PLAYENGINE_H

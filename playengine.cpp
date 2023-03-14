#include "playengine.h"
#include <QDebug>

PlayEngine::PlayEngine()
{

}

PlayEngine::~PlayEngine()
{

}

bool PlayEngine::Init(PlayEngine::CONFIG &config)
{

    mConfig = config;

    static int taskId = 0;

    do
    {
        Reader::CONFIG readerConfig = { };
        readerConfig.taskId = taskId;
        readerConfig.fileName = mConfig.fileName;
        if (false == mReader.Init(readerConfig))
            break;

        VideoDecoder::CONFIG videoDecoderConfig = { };
        videoDecoderConfig.taskId = taskId;
        videoDecoderConfig.reader = &mReader;

        if (false == mVideoDecoder.Init(videoDecoderConfig))
            break;

        VideoRender::CONFIG videoRenderConfig = { };
        videoRenderConfig.taskId = taskId;
        videoRenderConfig.refreshRate = 60;
        videoRenderConfig.renderView = mConfig.renderView;
        videoRenderConfig.decoder = &mVideoDecoder;
        if (false == mVideoRender.Init(videoRenderConfig))
            break;


        AudioDecoder::CONFIG audioDecoderConfig = { };
        audioDecoderConfig.taskId = taskId;
        audioDecoderConfig.reader = &mReader;
        if (false == mAudioDecoder.Init(audioDecoderConfig))
            break;

        AudioPlayback::CONFIG audioPlaybackConfig = { };
        audioPlaybackConfig.taskId = taskId;
        audioPlaybackConfig.decoder = &mAudioDecoder;
        if (false == mAudioPlayback.Init(audioPlaybackConfig))
            break;


        qDebug() << QString::asprintf("playengine finish init: [%d]reader=%p, decoder=%p, render=%p",
                                      taskId, &mReader, &mVideoDecoder, &mVideoRender);

        ++taskId;

//        mBaseTime = BaseTime::GetInstance();


        return true;

    } while (0);

    return false;
}

void PlayEngine::Play()
{
    mVideoRender.Start();
    mVideoDecoder.Start();
    mAudioPlayback.Start();
    mAudioDecoder.Start();
    mReader.Start();

//    mBaseTime->start();
    BaseTime::GetInstance()->start();
}

void PlayEngine::Pause()
{

}

void PlayEngine::Stop()
{

}

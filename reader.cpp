#include "reader.h"
#include <QDebug>

#include "utils.h"

Reader::Reader()
{

}


Reader::~Reader()
{
    disconnect(&mReaderTimer, &QTimer::timeout, this, &Reader::OnReadFile);
    CloseFile();

    quit();
    wait();

    for (auto videoPacket : mVideoPackets)
    {
        FreeVideoPacket(videoPacket);
    }

    mVideoPackets.clear();

}

bool Reader::Init(CONFIG &config)
{
    moveToThread(this);

    mConfig = config;

    mReaderTimer.moveToThread(this);
    connect(&mReaderTimer, &QTimer::timeout, this, &Reader::OnReadFile);

    if (false == OpenFile())
        return false;

    if (false == AnalyzeStreams())
        return false;

    return true;

}


void Reader::Start()
{
    this->start();
}

AVStream* Reader::GetVideoStream() const
{
    return mVideoStream;
}
AVStream* Reader::GetAudioStream() const
{
    return mAudioStream;
}

AVPacket* Reader::GetVideoPacket()
{
    QMutexLocker locker(&mVideoPacketsMutex);
    if (mVideoPackets.isEmpty())
        return nullptr;
    else
        return mVideoPackets.head();
}

void Reader::FreeVideoPacket(AVPacket *packet)
{
    QMutexLocker locker(&mVideoPacketsMutex);


    if (!mVideoPackets.isEmpty())
    {
        if (false == mVideoPackets.removeOne(packet))
        {
            qWarning() << "mPackets.removeOne error";
        }
//        av_packet_free(&packet);
        av_packet_unref(packet);
    }
}

AVPacket* Reader::GetAudioPacket()
{
    QMutexLocker locker(&mAudioPacketsMutex);
    if (mAudioPackets.isEmpty())
        return nullptr;
    else
        return mAudioPackets.head();
}
void Reader::FreeAudioPacket(AVPacket *packet)
{
    QMutexLocker locker(&mAudioPacketsMutex);

    if (!mAudioPackets.isEmpty())
    {
        if (false == mAudioPackets.removeOne(packet))
        {
            qWarning() << "mPackets.removeOne error";
        }

        av_packet_unref(packet);
    }
}



void Reader::OnReadFile()
{

    AVPacket *packet = av_packet_alloc();

    int ret = av_read_frame(mFmtCtx, packet);
    if (ret < 0)
    {
        qWarning() << QString("av_read_frame error: ret %1").arg(ret);
        av_packet_free(&packet);
        return;
    }

    if (packet->stream_index == mVideoStreamId)
    {
        QMutexLocker locker(&mVideoPacketsMutex);
        mVideoPackets.enqueue(packet);
        emit SigDecodeVideo();

//        av_write_frame(mOutFmtCtx, packet);
    }
    else if (packet->stream_index == mAudioStreamId)
    {
        QMutexLocker locker(&mAudioPacketsMutex);
        mAudioPackets.enqueue(packet);
        emit SigDecodeAudio();
    }
    else
    {
        ;
    }

}


void Reader::run()
{
    mReaderTimer.start(10);

    exec();

    mReaderTimer.stop();
}

bool Reader::OpenFile()
{
    int ret = avformat_open_input(&mFmtCtx, mConfig.fileName.toStdString().c_str(), nullptr, nullptr);
    if (ret < 0)
    {
        qWarning() << QString("avformat_open_input %1 error: ret %2").arg(mConfig.fileName).arg(ret);
        return false;
    }

    qDebug() << QString("avformat context %1").arg(mConfig.taskId);


    return true;
}


void Reader::CloseFile()
{
    avformat_close_input(&mFmtCtx);
}

bool Reader::AnalyzeStreams()
{
    int ret = avformat_find_stream_info(mFmtCtx, nullptr);
    if (ret < 0)
    {
        qWarning() << QString("avformat_find_stream_info error: ret %1").arg(ret);
        return false;
    }

    mVideoStreamId = av_find_best_stream(mFmtCtx, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
    if (mVideoStreamId < 0)
    {
        qWarning() << QString("av_find_best_stream(AVMEDIA_TYPE_VIDEO) error: ret %1").arg(mVideoStreamId);
        return false;
    }

    mVideoStream = mFmtCtx->streams[mVideoStreamId];

    mAudioStreamId = av_find_best_stream(mFmtCtx, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0);
    if (mAudioStreamId < 0)
    {
        qWarning() << QString("av_find_best_stream(AVMEDIA_TYPE_AUDIO) error: ret %1").arg(mAudioStreamId);
        return false;
    }

    mAudioStream = mFmtCtx->streams[mAudioStreamId];

    if (!mVideoStream || !mAudioStream)
    {
        qWarning("video_stream or audio_stream is null");
        return false;
    }

    return true;
}


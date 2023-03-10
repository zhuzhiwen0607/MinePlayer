#include "audiodecoder.h"
#include <QDebug>

AudioDecoder::AudioDecoder()
{

}

AudioDecoder::~AudioDecoder()
{

    disconnect(&mDecodeTimer, &QTimer::timeout, this, &AudioDecoder::OnDecode);
    disconnect(mConfig.reader, &Reader::SigDecodeAudio, this, &AudioDecoder::OnDecode);

    quit();
    wait();


}



bool AudioDecoder::Init(CONFIG &config)
{
    moveToThread(this);

    mConfig = config;

    mDecodeTimer.moveToThread(this);

    connect(&mDecodeTimer, &QTimer::timeout, this, &AudioDecoder::OnDecode);
    connect(mConfig.reader, &Reader::SigDecodeAudio, this, &AudioDecoder::OnDecode);

    mStream = mConfig.reader->GetAudioStream();

    mCodec = avcodec_find_decoder(mStream->codecpar->codec_id);

    mCodecCtx = avcodec_alloc_context3(mCodec);
    if (!mCodecCtx)
    {
        qWarning() << QString("avcodec_alloc_context3 error");
        return false;
    }


    int ret = avcodec_parameters_to_context(mCodecCtx, mStream->codecpar);
    if (ret < 0)
    {
        qWarning() << QString("avcodec_parameters_to_context error: ret %1").arg(ret);
        return false;
    }

    ret = avcodec_open2(mCodecCtx, mCodec, nullptr);
    if (ret < 0)
    {
        qWarning() << QString("avcodec_open2 error: ret %1").arg(ret);
        return false;
    }

    qInfo("audio decoder finish init");

    return true;
}

void AudioDecoder::Start()
{
    this->start();
}

AVCodecContext* AudioDecoder::GetDecoderContext()
{
    return mCodecCtx;
}

AVFrame* AudioDecoder::GetFrame()
{
    QMutexLocker locker(&mFramesMutex);
    if (mFrames.isEmpty())
        return nullptr;
    else
        return mFrames.head();
}
void AudioDecoder::FreeFrame(AVFrame *frame)
{
    if (!frame)
        return;

    QMutexLocker locker(&mFramesMutex);
    if (!mFrames.isEmpty())
    {
        if (false == mFrames.removeOne(frame))
        {
            qWarning() << "mFrames.removeOne error";
        }
        av_frame_free(&frame);
    }
}

void AudioDecoder::OnDecode()
{
    AVPacket *packet = mConfig.reader->GetAudioPacket();
    if (!packet)
        return;

    int ret = avcodec_send_packet(mCodecCtx, packet);
    if (ret < 0 && ret != AVERROR(EAGAIN))
    {
        qWarning() << QString("avcodec_send_packet error: ret %1").arg(ret);
        return;
    }

    mConfig.reader->FreeAudioPacket(packet);

    AVFrame *frame = av_frame_alloc();
    ret = avcodec_receive_frame(mCodecCtx, frame);
    if (ret < 0)
    {
        av_frame_free(&frame);
        return;
    }

    {
        QMutexLocker locker(&mFramesMutex);
        mFrames.enqueue(frame);
    }

    emit SigPlayback();

}

void AudioDecoder::run()
{
    mDecodeTimer.start(100);
    exec();
    mDecodeTimer.stop();
}

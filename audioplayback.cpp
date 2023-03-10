#include "audioplayback.h"
#include <QDebug>

AudioPlayback::AudioPlayback()
{

}

AudioPlayback::~AudioPlayback()
{
    disconnect(mConfig.decoder, &AudioDecoder::SigPlayback, this, &AudioPlayback::OnPlayback);

    quit();
    wait();

    swr_free(&mSwrCtx);
}

bool AudioPlayback::Init(CONFIG &config)
{
    moveToThread(this);

    mConfig = config;

    AVCodecContext *decoderContext = nullptr;
    decoderContext = mConfig.decoder->GetDecoderContext();
    if (!decoderContext)
    {
        return false;
    }

//    int mOutChannels;
//    int mOutSamples;
//    AVSampleFormat mOutFormat;
    const QAudioDeviceInfo deviceInfo = QAudioDeviceInfo::defaultOutputDevice();
    int inChannels = decoderContext->channels;
    int inSampleRate = decoderContext->sample_rate;
    AVSampleFormat inFormat = decoderContext->sample_fmt;

    mOutChannels = inChannels;
    mOutSampleRate = inSampleRate;
    // always swr in to out(QAudioFormat::Float - AV_SAMPLE_FMT_FLT)
    mOutFormat = AV_SAMPLE_FMT_FLT;     // ffmpeg out sample format
    QAudioFormat::SampleType outDeviceFormat = QAudioFormat::Float;  // QAudioOutput sample format
    int outSampleSize = 32;

//    qDebug() << QString("in audio format: channels=%1, sample_rate=%2, sample_fmt=%3")
//                .arg(in_channels).arg(in_sample_rate).arg(in_sample_fmt);

    QAudioFormat outAudioFormat;
    outAudioFormat.setCodec("audio/pcm");
    outAudioFormat.setSampleRate(mOutSampleRate);
    outAudioFormat.setChannelCount(mOutChannels);
    outAudioFormat.setSampleType(outDeviceFormat);
    outAudioFormat.setSampleSize(outSampleSize);

    if (!deviceInfo.isFormatSupported(outAudioFormat))
    {
        qWarning() << QString("outAudioFormat is not supported: channelCount=%1, sampleRate=%2, sampleSize=%3, sampleType=%4")
                      .arg(outAudioFormat.channelCount())
                      .arg(outAudioFormat.sampleRate())
                      .arg(outAudioFormat.sampleSize())
                      .arg(outAudioFormat.sampleType());
        return false;
    }

    mAudioOutput = new QAudioOutput(deviceInfo, outAudioFormat);
    int bufferSize = outAudioFormat.sampleRate() * outAudioFormat.sampleSize() * outAudioFormat.channelCount() / 8;
    mAudioOutput->setBufferSize(bufferSize);


    mSwrCtx = swr_alloc();


    av_opt_set_chlayout(mSwrCtx, "in_chlayout", /*(const AVChannelLayout*)*/&(decoderContext->ch_layout), 0);
    av_opt_set_int(mSwrCtx, "in_sample_rate", inSampleRate, 0);
    av_opt_set_sample_fmt(mSwrCtx, "in_sample_fmt", inFormat, 0);

    av_opt_set_chlayout(mSwrCtx, "out_chlayout", /*(const AVChannelLayout*)*/&(decoderContext->ch_layout), 0);
    av_opt_set_int(mSwrCtx, "out_sample_rate", mOutSampleRate, 0);
    av_opt_set_sample_fmt(mSwrCtx, "out_sample_fmt", mOutFormat, 0);

    int ret = swr_init(mSwrCtx);
    if (ret < 0)
    {
        qWarning() << QString("swr_init error, ret=%1").arg(ret);
        return false;
    }

    connect(mConfig.decoder, &AudioDecoder::SigPlayback, this, &AudioPlayback::OnPlayback);

    qInfo("audio playback finish init");
    return true;
}

void AudioPlayback::Start()
{
    this->start();
}

void AudioPlayback::OnPlayback()
{
    AVFrame *frame = mConfig.decoder->GetFrame();
    if (!frame)
        return;

    // todo: resample audio data
    uint8_t **outData;
    int bufSize = 0;
    Resample(frame, &outData, bufSize);


    qint64 nBytes = mAudioDevice->write((const char*)outData[0], bufSize);
//    qDebug("size %ld, write %ld bytes", size, nBytes);

    mConfig.decoder->FreeFrame(frame);
}

void AudioPlayback::run()
{

    mAudioDevice = mAudioOutput->start();
    mAudioOutput->setVolume(1.0);

    exec();
    mAudioOutput->stop();

}

void AudioPlayback::Resample(AVFrame *frame, uint8_t ***outData, int &outSize)
{
    if (!frame)
        return;

    int line_size;
    int64_t delay = swr_get_delay(mSwrCtx, frame->sample_rate) + frame->nb_samples;
    int64_t outNbSamples = av_rescale_rnd(delay,
                                      mOutSampleRate,
                                      frame->sample_rate,
                                      AV_ROUND_UP);


    int ret = av_samples_alloc_array_and_samples(outData,
                                                 &line_size,
                                                 mOutChannels,
                                                 outNbSamples,
                                                 mOutFormat,
                                                 0);
    if (ret < 0)
    {
        qWarning() << QString("av_samples_alloc_array_and_samples error(%1): "
                              "nb_channels=%1, nb_samples=%2, format=%3")
                      .arg(ret).arg(mOutChannels).arg(outNbSamples).arg(mOutFormat);
        return;
    }

    int samples = swr_convert(mSwrCtx,
                              *outData,
                              outNbSamples,
                              (const uint8_t**)frame->extended_data,
                              frame->nb_samples);
    if (samples < 0)
    {
        qWarning() << QString("swr_convert error(%1)").arg(samples);
        return;
    }

    outSize = av_samples_get_buffer_size(&line_size,
                                         mOutChannels,
                                         samples,
                                         mOutFormat,
                                         1);


}

#include "videodecoder.h"
#include <QDebug>
#include "utils.h"

extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavutil/time.h"
}



VideoDecoder::VideoDecoder()
{

}

VideoDecoder::~VideoDecoder()
{
    disconnect(&mDecodeTimer, &QTimer::timeout, this, &VideoDecoder::OnDecode);
    disconnect(mConfig.reader, &Reader::SigDecodeVideo, this, &VideoDecoder::OnDecode);
    this->quit();
    this->wait();

    for (auto frame : mFrames)
        FreeFrame(frame);

    mFrames.clear();

}

bool VideoDecoder::Init(VideoDecoder::CONFIG &config)
{
    moveToThread(this);
    mDecodeTimer.moveToThread(this);

    mConfig = config;

    connect(&mDecodeTimer, &QTimer::timeout, this, &VideoDecoder::OnDecode);
    connect(mConfig.reader, &Reader::SigDecodeVideo, this, &VideoDecoder::OnDecode);

    mStream = mConfig.reader->GetVideoStream();

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

    ret = av_hwdevice_ctx_create(&mHWContext, AV_HWDEVICE_TYPE_DXVA2, nullptr, nullptr, 0);
    if (mHWContext)
    {
        mCodecCtx->hw_device_ctx = av_buffer_ref(mHWContext);
    }


//    av_log_set_level(AV_LOG_DEBUG);

    ret = avcodec_open2(mCodecCtx, mCodec, nullptr);
    if (ret < 0)
    {
        qWarning() << QString("avcodec_open2 error: ret %1").arg(ret);
        return false;
    }

    qDebug() << QString("avcodec_open2[%1] mCodecCtx thread_count=%2, thread_type=%3, active_thread_type=%4")
                .arg(mConfig.taskId).arg(mCodecCtx->thread_count).arg(mCodecCtx->thread_type).arg(mCodecCtx->active_thread_type);

    int width = mCodecCtx->width;
    int height = mCodecCtx->height;
    AVPixelFormat srcFormat = mCodecCtx->pix_fmt;
    AVPixelFormat dstFormat = AV_PIX_FMT_NV12;
    qDebug() << QString("width=%1, height=%2, srcFormat=%3, dstFormat=%4")
                .arg(width).arg(height).arg(srcFormat).arg(dstFormat);
    mSwsContext = sws_getCachedContext(nullptr,
                                       width,
                                       height,
                                       srcFormat,
                                       width,
                                       height,
                                       dstFormat,
                                       SWS_FAST_BILINEAR,
                                       nullptr,
                                       nullptr,
                                       nullptr);

    qInfo() << QString("loglevel = %1").arg(av_log_get_level());

    qInfo() << "video deocder finish init";

//    if (false == WriteHeader())
//        return false;

    return true;

}

void VideoDecoder::Start()
{
    this->start();
}


AVFrame* VideoDecoder::GetFrame()
{
    QMutexLocker locker(&mFramesMutex);
    if (mFrames.isEmpty())
        return nullptr;
    else
        return mFrames.head();
}

void VideoDecoder::FreeFrame(AVFrame *frame)
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

int VideoDecoder::GetVideoWidth() const
{
    return mStream->codecpar->width;
}

int VideoDecoder::GetVideoHeight() const
{
    return mStream->codecpar->height;
}

int VideoDecoder::GetPixelFormat() const
{
    return mStream->codecpar->format;
}

AVRational& VideoDecoder::GetTimeBase() const
{
    return mStream->time_base;
}

void VideoDecoder::OnDecode()
{

    AVPacket *packet = mConfig.reader->GetVideoPacket();
    if (!packet)
        return;




//    {
//        int threadId = (int)this->currentThread();
////        Utils::AppendBytesToFile(QString("D:\\readfile_%1.h264").arg(threadId), (const char*)packet->data, packet->size);
//        Utils::AppendBytesToFile(QString("D:\\out.h264"), (const char*)packet->data, packet->size);
//    }

//    qDebug("[video] OnDecode(packet): dts(%ld) pts(%ld) time_base(%d,%d) duration(%ld)",
//           packet->dts, packet->pts, /*packet->time_base.num, packet->time_base.den,*/
//           mCodecCtx->time_base.num, mCodecCtx->time_base.den,
//           packet->duration);

    int ret = avcodec_send_packet(mCodecCtx, packet);
    if (ret < 0 && ret != AVERROR(EAGAIN))
    {
        qWarning() << QString("avcodec_send_packet error: ret %1").arg(ret);
        return;
    }

    mConfig.reader->FreeVideoPacket(packet);

    AVFrame *frame = av_frame_alloc();
    ret = avcodec_receive_frame(mCodecCtx, frame);
    if (ret < 0)
    {
        av_frame_free(&frame);
        return;
    }

//    qDebug("[video] OnDecode(frame): pts(%ld) time_base(%d,%d) duration(%ld)",
//           frame->pts, /*packet->time_base.num, packet->time_base.den,*/
//           mStream->time_base.num, mStream->time_base.den,
//           frame->duration);

    AVFrame *dstFrame = av_frame_alloc();
    dstFrame->format = AV_PIX_FMT_NV12;
    dstFrame->width = frame->width;
    dstFrame->height = frame->height;
    ret = av_frame_get_buffer(dstFrame, 0);
    if (ret < 0)
    {
        ;
    }

    if (frame->hw_frames_ctx)
    {
        ret = av_hwframe_transfer_data(dstFrame, frame, 0);
//        qDebug() << QString("av_hwframe_transfer_data: ret=%1").arg(ret);
    }
    else
    {
        sws_scale(mSwsContext, frame->data, frame->linesize, 0, frame->height,
                  dstFrame->data, dstFrame->linesize);
    }

    dstFrame->pts = frame->pts;
    dstFrame->duration = frame->duration;
    dstFrame->repeat_pict = frame->repeat_pict;


    av_frame_free(&frame);

    int64_t curTime = av_gettime();
    int64_t curTime2 = av_gettime_relative();
    double pts2 = dstFrame->pts * av_q2d(mStream->time_base);
    double duration2 = dstFrame->duration * av_q2d(mStream->time_base);

//    qDebug("[video] OnDecode(frame): pts(%lld) time_base(%d,%d) duration(%ld), "
//           "repeat_pict(%d), pts2=%f, duration2=%f, curTime=%lld, curTime2=%lld",
//           dstFrame->pts, /*packet->time_base.num, packet->time_base.den,*/
//           mStream->time_base.num, mStream->time_base.den,
//           dstFrame->duration, dstFrame->repeat_pict,
//           pts2,duration2, curTime, curTime2);


    {

        QMutexLocker locker(&mFramesMutex);

        mFrames.enqueue(dstFrame);

    }

/*
    {
        QByteArray frameBytes;
        int size = av_image_get_buffer_size((AVPixelFormat)dstFrame->format, dstFrame->width, dstFrame->height, 1);
        frameBytes.resize(size);
        int copyBytes = av_image_copy_to_buffer((uint8_t*)frameBytes.data(),
                                                size,
                                                (const uint8_t* const*)dstFrame->data,
                                                (const int*)dstFrame->linesize,
                                                (AVPixelFormat)dstFrame->format,
                                                dstFrame->width,
                                                dstFrame->height,
                                                1);
        if (copyBytes < 0)
        {
            qWarning() << QString("av_image_copy_to_buffer 2 error: ret=%1").arg(copyBytes);
//            return;
        }

        int threadId = (int)this->currentThreadId();

        Utils::AppendBytesToFile(QString("D:\\decodefile_%1.yuv").arg(threadId), frameBytes);

    }
*/
    emit SigRenderVideo();

}

void VideoDecoder::run()
{
    mDecodeTimer.start(20);

    exec();

    mDecodeTimer.stop();
}

bool VideoDecoder::WriteHeader()
{
    QString fileName = QString("D:\\out_%1.h264").arg(mConfig.taskId);

    int ret = avformat_alloc_output_context2(&mOutFmtCtx, NULL, NULL, fileName.toStdString().c_str());
    if (ret < 0)
    {
        qWarning() << QString("[WriteHeader] avformat_alloc_output_context2 error: ret=%1").arg(ret);
        return false;
    }

    AVStream *stream = avformat_new_stream(mOutFmtCtx, NULL);
    if (!stream)
    {
        qWarning() << QString("[WriteHeader] avformat_new_stream error");
        return false;
    }

    ret = avcodec_parameters_from_context(stream->codecpar, mCodecCtx);
    if (ret < 0)
    {
        qWarning() << QString("[WriteHeader] avcodec_parameters_from_context error: ret=%1").arg(ret);
        return false;
    }

    ret = avio_open(&mOutFmtCtx->pb, fileName.toStdString().c_str(), AVIO_FLAG_WRITE);
    if (ret < 0)
    {
        qWarning() << QString("[WriteHeader] avio_open error: ret=%1").arg(ret);
        return false;
    }

    ret = avformat_write_header(mOutFmtCtx, NULL);
    if (ret < 0)
    {
        qWarning() << QString("[WriteHeader] avformat_write_header error: ret=%1").arg(ret);
        return false;
    }

    return true;
}

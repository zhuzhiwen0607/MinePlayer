#include "videodecoder.h"
#include <QDebug>
#include "utils.h"

extern "C"
{
#include "libavcodec/avcodec.h"

}
/*
Decoder::Decoder(Decoder::CONFIG &config)
{
    mConfig = config;

    moveToThread(this);

    mDecodeTimer.moveToThread(this);

    connect(&mDecodeTimer, &QTimer::timeout, this, &Decoder::OnNewPacket);

    if (false == Init())
        return;


//    this->start();
}
*/


VideoDecoder::VideoDecoder()
{

}

VideoDecoder::~VideoDecoder()
{
    disconnect(&mDecodeTimer, &QTimer::timeout, this, &VideoDecoder::OnDecode);
//    disconnect(mConfig.reader, SIGNAL(SigDecodeVideo(int)), this, SLOT(OnDecode(int)));
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

    mFmtCtx = mConfig.reader->GetFormatContext();

    mVideoStreamIndex = mConfig.reader->GetVideoStreamId();
    mCodec = mConfig.reader->GetVideoCodec();

    mCodecCtx = avcodec_alloc_context3(mCodec);
    if (!mCodecCtx)
    {
        qWarning() << QString("avcodec_alloc_context3 error");
        return false;
    }

    qDebug() << QString("video stream index=%1").arg(mVideoStreamIndex);

    int ret = avcodec_parameters_to_context(mCodecCtx, mFmtCtx->streams[mVideoStreamIndex]->codecpar);
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

#if 0
bool VideoDecoder::WriteHeader()
{
    do
    {
        int threadId = (int)this->currentThreadId();
        QString outFile = QString("E:\\out_%1.h264").arg(threadId);

        int ret = avformat_alloc_output_context2(&mOutFmtCtx, NULL, NULL, outFile.toStdString().c_str());
        if (ret < 0)
            qWarning() << QString("[test] avformat_alloc_output_context2 error: ret=%1").arg(ret);

        if (NULL == avformat_new_stream(mOutFmtCtx, NULL))
            qWarning() << QString("[test] avformat_new_stream error");

        ret = avcodec_parameters_from_context(mOutFmtCtx->streams[0]->codecpar, mCodecCtx);
        if (ret < 0)
            qWarning() << QString("[test] avcodec_parameters_from_context error: ret=%1").arg(ret);

        AVIOContext *ioCtx = NULL;
        ret = avio_open2(&ioCtx, outFile.toStdString().c_str(), 0, NULL, NULL);
        if (ret < 0)
            qWarning() << QString("[test] avio_open2 error: ret=%1").arg(ret);

        ret = avformat_write_header(mOutFmtCtx, NULL);
        qWarning() << QString("[test] avformat_write_header: ret=%1").arg(ret);

        return true;
    } while (0);

    return false;
}
#endif
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
    return mFmtCtx->streams[mVideoStreamIndex]->codecpar->width;
}

int VideoDecoder::GetVideoHeight() const
{
    return mFmtCtx->streams[mVideoStreamIndex]->codecpar->height;
}

int VideoDecoder::GetPixelFormat() const
{
    return mFmtCtx->streams[mVideoStreamIndex]->codecpar->format;
}
#if 0
void VideoDecoder::OnDecode(int taskId)
{
    if (taskId != mConfig.taskId)
        return;

    QByteArray rawBytes = mConfig.reader->GetRawBytes();
    if (rawBytes.isEmpty())
        return;

    {
        int threadId = (int)this->currentThreadId();
        Utils::AppendBytesToFile(QString("D:\\readfile11_%1.h264").arg(threadId), rawBytes);
    }

    mConfig.reader->FreeRawBytes(rawBytes);

    return;

    uint8_t *data = (uint8_t*)rawBytes.data();
    int dataSize = rawBytes.size();

    AVPacket *packet = av_packet_alloc();

    while (dataSize > 0)
    {
        int ret = av_parser_parse2(mParser, mCodecCtx, &packet->data, &packet->size,
                                   data, dataSize,
                                   AV_NOPTS_VALUE, AV_NOPTS_VALUE, 0);
        if (ret < 0)
        {
            qWarning() << QString("av_parser_parse2 error: ret=%1").arg(ret);
        }

        data += ret;
        dataSize -= ret;

        if (packet->size > 0)
        {
            int ret2 = avcodec_send_packet(mCodecCtx, packet);
            if (ret2 < 0 && ret2 != AVERROR(EAGAIN))
            {
                qWarning() << QString("avcodec_send_packet error: ret %1").arg(ret);
                return;
            }

            while (ret2 >= 0)
            {
                AVFrame *frame = av_frame_alloc();
                ret2 = avcodec_receive_frame(mCodecCtx, frame);
                if (ret2 == AVERROR(EAGAIN) || ret2 == AVERROR_EOF)
                {
                    av_frame_free(&frame);
                    break;
                }
                else if (ret2 < 0)
                {
                    qWarning() << QString("avcodec_receive_frame error: ret %1").arg(ret2);
                    return;
                }

                AVFrame *dstFrame = av_frame_alloc();
                dstFrame->format = AV_PIX_FMT_NV12;
                dstFrame->width = frame->width;
                dstFrame->height = frame->height;
                ret = av_frame_get_buffer(dstFrame, 0);
                if (ret < 0)
                {
                    ;
                }
                sws_scale(mSwsContext, frame->data, frame->linesize, 0, frame->height,
                          dstFrame->data, dstFrame->linesize);

                av_frame_free(&frame);

                {

                    QMutexLocker locker(&mFramesMutex);

                    mFrames.enqueue(dstFrame);

                }

                emit SigRenderVideo(mConfig.taskId);


            }


        }


    }

    av_packet_free(&packet);

    mConfig.reader->FreeRawBytes(rawBytes);


}
#endif

void VideoDecoder::OnDecode()
{
//    if (taskId != mConfig.taskId)
//        return;

    AVPacket *packet = mConfig.reader->GetVideoPacket();
    if (!packet)
        return;

    if (packet->stream_index != mVideoStreamIndex)
    {
//        qWarning() << QString("packet stream index(%1) is not equal to mVideoStreamIndex(%2)")
//                      .arg(packet->stream_index).arg(mVideoStreamIndex);
        mConfig.reader->FreeVideoPacket(packet);
        return;
    }
/*
    {
        int ret = av_interleaved_write_frame(mOutFmtCtx, packet);
        qDebug() << "av_write_frame ret =" << ret;
        mConfig.reader->FreeVideoPacket(packet);
    }
    return;
*/

//    {
//        int threadId = (int)this->currentThread();
////        Utils::AppendBytesToFile(QString("D:\\readfile_%1.h264").arg(threadId), (const char*)packet->data, packet->size);
//        Utils::AppendBytesToFile(QString("D:\\out.h264"), (const char*)packet->data, packet->size);
//    }

//    {
//        int av_ret = av_interleaved_write_frame(mOutFmtCtx, packet);
//        if (av_ret < 0)
//        {
//            qWarning() << QString("av_interleaved_write_frame error: av_ret=%1").arg(av_ret);
//        }
//    }


//    static int pktSndCnt = 0;
//    qDebug() << QString("avcodec_send_packet %1").arg(pktSndCnt++);

    int ret = avcodec_send_packet(mCodecCtx, packet);
    if (ret < 0 && ret != AVERROR(EAGAIN))
//    if (ret < 0)
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

//    mConfig.reader->FreeVideoPacket(packet);
#if 0
    {
        static QByteArray frameBytes;
        int size = av_image_get_buffer_size((AVPixelFormat)frame->format, frame->width, frame->height, 1);
        frameBytes.resize(size);
        int copyBytes = av_image_copy_to_buffer((uint8_t*)frameBytes.data(),
                                                size,
                                                (const uint8_t* const*)frame->data,
                                                (const int*)frame->linesize,
                                                (AVPixelFormat)frame->format,
                                                frame->width,
                                                frame->height,
                                                1);
        if (copyBytes < 0)
        {
            qWarning() << QString("av_image_copy_to_buffer 2 error: ret=%1").arg(copyBytes);
//            return;
        }

        int threadId = (int)this->currentThreadId();

//        qDebug() << QString("frame info: (%1,%2),format=%3, size=%4")
//                    .arg(frame->width)
//                    .arg(frame->height)
//                    .arg(frame->format)
//                    .arg(frameBytes.size());
        Utils::AppendBytesToFile(QString("D:\\decodefile1_%1.yuv").arg(threadId), frameBytes);

    }
#endif



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

    av_frame_free(&frame);



//    qDebug() << "avcodec_receive_frame";
    {

        QMutexLocker locker(&mFramesMutex);

        mFrames.enqueue(dstFrame);

    }

#if 0
    {
        static QByteArray frameBytes;
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
#endif
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

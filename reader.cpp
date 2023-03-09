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
//    CloseFile3();

    quit();
    wait();

    for (auto videoPacket : mVideoPackets)
    {
//        av_packet_free(&packet);
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


//    if (false == OpenFile3())
//        return false;

    if (false == AnalyzeStreams())
        return false;

    return true;

}

bool Reader::WriteHeader()
{
    int ret = avformat_alloc_output_context2(&mOutFmtCtx, NULL, NULL, "D:\\out.h264");
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

//    ret = avcodec_parameters_from_context(stream->codecpar, mVideoCodec);


    return true;
#if 0
    do
    {
        int ret = avformat_alloc_output_context2(&mOutFmtCtx, NULL, NULL, "E:\\out.h264");
        if (ret < 0)
            qWarning() << QString("[test] avformat_alloc_output_context2 error: ret=%1").arg(ret);

        if (NULL == avformat_new_stream(mOutFmtCtx, NULL))
            qWarning() << QString("[test] avformat_new_stream error");

        ret = avcodec_parameters_from_context(mOutFmtCtx->streams[0]->codecpar, mCodecCtx);
        if (ret < 0)
            qWarning() << QString("[test] avcodec_parameters_from_context error: ret=%1").arg(ret);

        AVIOContext *ioCtx = NULL;
        ret = avio_open2(&ioCtx, "E:\\out.h264", 0, NULL, NULL);
        if (ret < 0)
            qWarning() << QString("[test] avio_open2 error: ret=%1").arg(ret);

        ret = avformat_write_header(mOutFmtCtx, NULL);
        qWarning() << QString("[test] avformat_write_header: ret=%1").arg(ret);

        return true;
    } while (0);

    return false;
#endif
}

void Reader::Start()
{
    this->start();
}

AVFormatContext* Reader::GetFormatContext() const
{
    return mFmtCtx;
}

const AVCodec* Reader::GetVideoCodec() const
{
    return mVideoCodec;
}

const AVCodec* Reader::GetAudioCodec() const
{
    return mAudioCodec;
}

int Reader::GetVideoStreamId() const
{
    return mVideoStreamId;
}

int Reader::GetAudioStreamId() const
{
    return mAudioStreamId;
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

bool Reader::GetRawBytes(QByteArray &rawBytes)
{
    QMutexLocker locker(&mRawBytesMutex);
    if (mRawBytes.isEmpty())
        return false;

    rawBytes = mRawBytes.first();
    return true;

}
/*
QByteArray& Reader::GetRawBytes()
{
    QMutexLocker locker(&mRawBytesMutex);
    if (mRawBytes.isEmpty())
        return QByteArray();

    return mRawBytes.first();
}
*/
void Reader::FreeRawBytes(QByteArray &rawBytes)
{
    QMutexLocker locker(&mRawBytesMutex);
    if (mRawBytes.isEmpty())
        return;

    mRawBytes.removeOne(rawBytes);
}
#if 0
void Reader::OnReadFile()
{




    static int blocksize = 4196;

    static char* buffer = new char[blocksize];
    memset(buffer, 0, blocksize);

    size_t readBytes = fread(buffer, 1, blocksize, mFp);
    if (readBytes <= 0)
        return;

    QByteArray rawBytes(buffer, readBytes);


#if 0
    QByteArray rawBytes;
    rawBytes.reserve(blocksize);

    mDataStream >> rawBytes;

    qDebug() << QString("rawBytes size %1").arg(rawBytes.size());

    QByteArray rawBytes = mFile.read(blocksize);

//    {
//        QMutexLocker locker(&mRawBytesMutex);
//        mRawBytes.push_back(rawBytes);
//    }

    QByteArray rawBytes;
    size_t readBytes = fread(rawBytes.data(), 1, blocksize, mFp);
    rawBytes.resize(readBytes);
#endif
//    qDebug() << QString("read bytes %1").arg(readBytes);

    {
        QMutexLocker locker(&mRawBytesMutex);
        mRawBytes.push_back(rawBytes);
    }


    emit SigDecodeVideo(mConfig.taskId);


    Utils::AppendBytesToFile(QString("D:\\readfile012.mp4"), rawBytes);
}
#endif


void Reader::OnReadFile()
{
//    static int readCounts = 0;
//    qDebug() << QString("OnReadFile %1").arg(readCounts++);
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
//        qDebug() << QString("[%1]mVideoPackets size %2").arg(mConfig.taskId).arg(mVideoPackets.size());


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

    ret = av_find_best_stream(mFmtCtx, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
    if (ret < 0)
    {
        qWarning() << QString("av_find_best_stream error: ret %1").arg(ret);
        return false;
    }
    mVideoStreamId = ret;


    AVStream *stream = mFmtCtx->streams[mVideoStreamId];
//    qDebug() << QString("codec_id = %1").arg(stream->codecpar->codec_id);

    mVideoCodec = avcodec_find_decoder(stream->codecpar->codec_id);

    ret = av_find_best_stream(mFmtCtx, AVMEDIA_TYPE_AUDIO, -1, -1, &mAudioCodec, 0);
    if (ret < 0)
    {
        qWarning() << QString("av_find_best_stream error: ret %1").arg(ret);
        return false;
    }
    mAudioStreamId = ret;

    stream = mFmtCtx->streams[mAudioStreamId];
    mAudioCodec = avcodec_find_decoder(stream->codecpar->codec_id);

    return true;
}

bool Reader::OpenFile2()
{
    mFile.setFileName(mConfig.fileName);
    if (false == mFile.open(QIODevice::ReadOnly))
    {
        qWarning() << QString("open file %1 error").arg(mConfig.fileName);
        return false;
    }

    mDataStream.setDevice(&mFile);


    return true;
}


void Reader::CloseFile2()
{
    mFile.close();
}

bool Reader::OpenFile3()
{
    mFp = fopen(mConfig.fileName.toStdString().c_str(), "rb");
    if (!mFp)
    {
        qWarning() << "fopen error " << mConfig.fileName;
        return false;
    }

    return true;
}
void Reader::CloseFile3()
{
    fclose(mFp);
    mFp = nullptr;
}

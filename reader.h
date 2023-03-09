#ifndef READER_H
#define READER_H

#include <QThread>
#include <QFile>
#include <QTimer>
//#include <QByteArrayList>
#include <QQueue>
#include <QMutex>
#include <QDataStream>

extern "C"
{
//#include "libavfilter/avfilter.h"
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
}

class Reader : public QThread
{
    Q_OBJECT

public:
    typedef struct
    {
//        qint32 blockSize;
        int taskId;
        QString fileName;
    } CONFIG;

public:
//    explicit Reader(CONFIG &config);
    Reader();
    ~Reader();

    bool Init(CONFIG &config);
    bool WriteHeader();    // test
    void Start();

    AVFormatContext* GetFormatContext() const;
    const AVCodec* GetVideoCodec() const;
    const AVCodec* GetAudioCodec() const;
    int GetVideoStreamId() const;
    int GetAudioStreamId() const;
//    qint32 GetVideoWidth() const;
//    qint32 GetVideoHeight() const;

    AVPacket* GetVideoPacket();
    void FreeVideoPacket(AVPacket *packet);
    AVPacket* GetAudioPacket();
    void FreeAudioPacket(AVPacket *packet);

    bool GetRawBytes(QByteArray& rawBytes);
//    QByteArray& GetRawBytes();
    void FreeRawBytes(QByteArray &rawBytes);

signals:
//    void SigNewPacket();
    void SigDecodeVideo();
    void SigDecodeAudio();

public slots:
    void OnReadFile();

protected:
    virtual void run() override;

private:
    bool OpenFile();
    void CloseFile();
    bool AnalyzeStreams();

    bool OpenFile2();
    void CloseFile2();

    bool OpenFile3();
    void CloseFile3();

private:
    CONFIG mConfig;
//    QFile mFile;
    QTimer mReaderTimer;

//    QByteArrayList mRawBytesList;
    QMutex mVideoPacketsMutex;
    QQueue<AVPacket*> mVideoPackets;
    QMutex mAudioPacketsMutex;
    QQueue<AVPacket*> mAudioPackets;

    AVFormatContext *mFmtCtx = nullptr;
    const AVCodec *mVideoCodec = nullptr;
    const AVCodec *mAudioCodec = nullptr;
    int mVideoStreamId = -1;
    int mAudioStreamId = -1;
//    AVPacket *mPacket = nullptr;
//    AVCodec *mCodec = nullptr;

    AVFormatContext *mOutFmtCtx = nullptr;


    QFile mFile;
    QMutex mRawBytesMutex;
    QByteArrayList mRawBytes;

    QDataStream mDataStream;

    FILE *mFp = nullptr;

//    char *mBuffer = nullptr;
};


#endif // READER_H

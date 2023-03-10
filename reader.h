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
        int taskId;
        QString fileName;
    } CONFIG;

public:
    Reader();
    ~Reader();

    bool Init(CONFIG &config);

    void Start();

    AVStream* GetVideoStream() const;
    AVStream* GetAudioStream() const;

    AVPacket* GetVideoPacket();
    void FreeVideoPacket(AVPacket *packet);
    AVPacket* GetAudioPacket();
    void FreeAudioPacket(AVPacket *packet);

signals:
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

    QTimer mReaderTimer;

    AVFormatContext *mFmtCtx = nullptr;

    // video
    QMutex mVideoPacketsMutex;
    QQueue<AVPacket*> mVideoPackets;

    AVStream *mVideoStream = nullptr;
    int mVideoStreamId = -1;

    // audio
    QMutex mAudioPacketsMutex;
    QQueue<AVPacket*> mAudioPackets;

    AVStream *mAudioStream = nullptr;
    int mAudioStreamId = -1;

    AVFormatContext *mOutFmtCtx = nullptr;


    QFile mFile;
    QMutex mRawBytesMutex;
    QByteArrayList mRawBytes;

    QDataStream mDataStream;

    FILE *mFp = nullptr;


};


#endif // READER_H

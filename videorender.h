#ifndef RENDER_H
#define RENDER_H


#include <QThread>
#include <QTimer>
#include <QTime>
#include <windows.h>
#include <wrl/client.h>
#include <d3d11.h>
#include <DirectXMath.h>
#include <d3dcompiler.h>
#include "renderview.h"
#include "videodecoder.h"
#include "basetime.h"

class VideoRender : public QThread
{
    Q_OBJECT

public:
    typedef struct
    {
        int taskId;
        int refreshRate;
        RenderView *renderView;
        VideoDecoder *decoder;
    } CONFIG;

    typedef struct
    {
        DirectX::XMFLOAT4 Pos;
        DirectX::XMFLOAT2 TexCoord;
    } VERTEX_FMT;


public:
    VideoRender();
    ~VideoRender();

    bool Init(CONFIG &config);
    bool InitD3D();

    void Start();

public slots:
    void OnRender();

protected:
    virtual void run() override;

private:
    bool CompileShaderFromFile(QString fileName, QString entryPoint, QString shaderModel, ID3DBlob** ppBlobOut);

    void DoRender(QByteArray &frameBytes, int width, int height);

private:
    CONFIG mConfig;

    unsigned long mDelay = 0;
    double mNextPTS = -1.0;
    QTimer mNextFrameTimer;

//    QTime mBaseTime;
    BaseTime *mBaseTime;

    int mVerticesNum = 0;
    IDXGISwapChain *mSwapChain = nullptr;
    ID3D11Device *mDevice = nullptr;
    ID3D11DeviceContext *mDeviceContext = nullptr;
    ID3DBlob *mVSBlob = nullptr;
    ID3DBlob *mPSBlob = nullptr;
    ID3D11RenderTargetView *mRenderTargetView = nullptr;
    ID3D11Buffer *mVertexBuffer;
    ID3D11VertexShader *mVertexShader = nullptr;
    ID3D11PixelShader *mPixelShader = nullptr;
    ID3D11InputLayout *mVertexLayout = nullptr;
    ID3D11Texture2D *mTexture = nullptr;
    ID3D11ShaderResourceView *mTextureSRV[2];
};

#endif // RENDER_H

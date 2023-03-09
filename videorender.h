#ifndef RENDER_H
#define RENDER_H


#include <QThread>

#include <windows.h>
#include <wrl/client.h>
#include <d3d11.h>
#include <DirectXMath.h>
#include <d3dcompiler.h>
//#include "reader.h"
#include "renderview.h"
#include "videodecoder.h"

class VideoRender : public QThread
{
    Q_OBJECT

public:
    typedef struct
    {
//        HWND winId;
        int taskId;
        int refreshRate;
        RenderView *renderView;
//        Reader *reader;
        VideoDecoder *decoder;
    } CONFIG;

    typedef struct
    {
        DirectX::XMFLOAT4 Pos;
        DirectX::XMFLOAT2 TexCoord;
    } VERTEX_FMT;


public:
//    explicit Render(CONFIG &config);
    VideoRender();
    ~VideoRender();

    bool Init(CONFIG &config);
    bool InitD3D();

    void Start();

public slots:
//    void OnNewFrame();
    void OnRender();

protected:
    virtual void run() override;

private:
//    bool Init();
    bool CompileShaderFromFile(QString fileName, QString entryPoint, QString shaderModel, ID3DBlob** ppBlobOut);

private:
    CONFIG mConfig;

//    Microsoft::WRL::ComPtr<IDXGISwapChain> mSwapChain;
//    Microsoft::WRL::ComPtr<ID3D11Device> mDevice;
//    Microsoft::WRL::ComPtr<ID3D11DeviceContext> mDeviceContext;
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

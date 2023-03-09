#include "videorender.h"
#include <QMap>
#include <QDebug>
#include "utils.h"

template <typename T>
static inline void RELEASE(T x)
{
    if (x)
    {
        x->Release();
        x = nullptr;
    }
}

VideoRender::VideoRender()
{
    mTextureSRV[0] = nullptr;
    mTextureSRV[1] = nullptr;
}

VideoRender::~VideoRender()
{
//    disconnect(mConfig.decoder, SIGNAL(SigRenderVideo(int)), this, SLOT(OnRender(int)));
    disconnect(mConfig.decoder, &VideoDecoder::SigRenderVideo, this, &VideoRender::OnRender);
    this->quit();
    this->wait();

    RELEASE(mSwapChain);
    RELEASE(mDevice);
    RELEASE(mDeviceContext);
    RELEASE(mVSBlob);
    RELEASE(mPSBlob);
    RELEASE(mRenderTargetView);
    RELEASE(mVertexBuffer);
    RELEASE(mVertexShader);
    RELEASE(mPixelShader);
    RELEASE(mVertexLayout);
    RELEASE(mTexture);
    RELEASE(mTextureSRV[0]);
    RELEASE(mTextureSRV[1]);
}

void VideoRender::Start()
{
    this->start();

}

void VideoRender::OnRender()
{
//    qDebug() << "OnRender";

//    if (taskId != mConfig.taskId)
//        return;

    AVFrame *frame = mConfig.decoder->GetFrame();
    if (!frame)
        return;

//    static QByteArray frameBytes;
    QByteArray frameBytes;
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
//    qDebug() << QString("av_image_copy_to_buffer copyBytes=%1").arg(copyBytes);
    if (copyBytes < 0)
    {
        qWarning() << QString("av_image_copy_to_buffer error: ret=%1").arg(copyBytes);
        return;
    }

    int width = frame->width;
    int height = frame->height;

    mConfig.decoder->FreeFrame(frame);

//    qDebug() << QString("frameBytes size=%1, copyBytes=%2").arg(frameBytes.size()).arg(copyBytes);
    int threadId = (int)this->currentThreadId();

//    Utils::AppendBytesToFile(QString("E:\\renderfile_%1.yuv").arg(threadId), frameBytes.data(), copyBytes);

    RELEASE(mTexture);
    RELEASE(mTextureSRV[0]);
    RELEASE(mTextureSRV[1]);

    D3D11_TEXTURE2D_DESC textureDesc = { };
    textureDesc.Width = width;
    textureDesc.Height = height;
    textureDesc.MipLevels = 1;
    textureDesc.ArraySize = 1;
    textureDesc.Format = DXGI_FORMAT_NV12;
    textureDesc.SampleDesc.Count = 1;
    textureDesc.SampleDesc.Quality = 0;
    textureDesc.Usage = D3D11_USAGE_DEFAULT;
    textureDesc.BindFlags = 0;
    textureDesc.MiscFlags = 0;
    textureDesc.BindFlags |= D3D11_BIND_SHADER_RESOURCE;

    D3D11_SUBRESOURCE_DATA initData = { };
    initData.pSysMem = frameBytes.data();
    initData.SysMemPitch = width;

    HRESULT ret = mDevice->CreateTexture2D(&textureDesc, &initData, &mTexture);
    if (FAILED(ret))
    {
        qWarning() << QString("mDevice->CreateTexture2D error: ret=%1").arg(ret);
        return;
    }

    D3D11_SHADER_RESOURCE_VIEW_DESC srvDescY = { };
    srvDescY.Format = DXGI_FORMAT_R8_UINT;
    srvDescY.ViewDimension = D3D_SRV_DIMENSION_TEXTURE2D;
    srvDescY.Texture2D.MipLevels = 1;
    srvDescY.Texture2D.MostDetailedMip = 0;
    ret = mDevice->CreateShaderResourceView(mTexture, &srvDescY, &mTextureSRV[0]);
    if (FAILED(ret))
    {
        qWarning() << QString("mDevice->CreateShaderResourceView(Y) error: ret=%1").arg(ret);
        return;
    }

    D3D11_SHADER_RESOURCE_VIEW_DESC srvDescUV = { };
    srvDescUV.Format = DXGI_FORMAT_R8G8_UINT;
    srvDescUV.ViewDimension = D3D_SRV_DIMENSION_TEXTURE2D;
    srvDescUV.Texture2D.MipLevels = 1;
    srvDescUV.Texture2D.MostDetailedMip = 0;
    ret = mDevice->CreateShaderResourceView(mTexture, &srvDescUV, &mTextureSRV[1]);
    if (FAILED(ret))
    {
        qWarning() << QString("mDevice->CreateShaderResourceView(UV) error: ret=%1").arg(ret);
        return;
    }

    mDeviceContext->PSSetShaderResources(0, 2, mTextureSRV);
    mDeviceContext->OMSetRenderTargets(1, &mRenderTargetView, nullptr);
    mDeviceContext->Draw(mVerticesNum, 0);
    mSwapChain->Present(0, 0);
}

void VideoRender::run()
{
    exec();
}

bool VideoRender::CompileShaderFromFile(QString fileName, QString entryPoint, QString shaderModel, ID3DBlob **ppBlobOut)
{
    HRESULT hr = S_OK;

    DWORD dwShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;

#ifdef QT_DEBUG
    dwShaderFlags |= D3DCOMPILE_DEBUG;
    dwShaderFlags |= D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

    ID3DBlob* pErrorBlob = nullptr;
    hr = D3DCompileFromFile(fileName.toStdWString().c_str(), NULL, NULL, entryPoint.toStdString().c_str(), shaderModel.toStdString().c_str(), dwShaderFlags, 0,  ppBlobOut, &pErrorBlob);

    if (FAILED(hr))
    {

        qWarning() << QString::asprintf("D3DCompileFromFile error:%x  %s ", hr, (const char*)pErrorBlob->GetBufferPointer());
        if (pErrorBlob)
            pErrorBlob->Release();

        //qInfo() << QString::asprintf("D3DCompileFromFile failed %x", hr);
        return false;
    }

    if (pErrorBlob)
        pErrorBlob->Release();

    return true;

}

bool VideoRender::Init(VideoRender::CONFIG &config)
{
    moveToThread(this);

    mConfig = config;

    if (false == InitD3D())
        return false;

    connect(mConfig.decoder, &VideoDecoder::SigRenderVideo, this, &VideoRender::OnRender);
//    connect(mConfig.decoder, SIGNAL(SigRenderVideo(int)), this, SLOT(OnRender(int)));

    qInfo() << "video render finish init";
    return true;
}

bool VideoRender::InitD3D()
{

    UINT flags = 0;
#ifdef QT_DEBUG
    flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    static D3D_FEATURE_LEVEL featureLevels[] = {
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0
    };

    UINT nFeatureLevels = sizeof(featureLevels) / sizeof(featureLevels[0]);

    DXGI_SWAP_CHAIN_DESC swapChainDesc = { };
    swapChainDesc.BufferDesc.Width = mConfig.decoder->GetVideoWidth();
    swapChainDesc.BufferDesc.Height = mConfig.decoder->GetVideoHeight();
    swapChainDesc.BufferDesc.RefreshRate.Numerator = mConfig.refreshRate;
    swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
    swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.SampleDesc.Quality = 0;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.BufferCount = 2;
    swapChainDesc.OutputWindow = (HWND)mConfig.renderView->winId();
    swapChainDesc.Windowed = true;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

    D3D_FEATURE_LEVEL outFeatureLevel;

    HRESULT ret = D3D11CreateDeviceAndSwapChain(nullptr,
                                                D3D_DRIVER_TYPE_HARDWARE,
                                                nullptr,
                                                flags,
                                                featureLevels,
                                                nFeatureLevels,
                                                D3D11_SDK_VERSION,
                                                &swapChainDesc,
//                                                mSwapChain.GetAddressOf(),
//                                                mDevice.GetAddressOf(),
                                                &mSwapChain,
                                                &mDevice,
                                                &outFeatureLevel,
//                                                mDeviceContext.GetAddressOf()
                                                &mDeviceContext
                                                );
    if (FAILED(ret))
    {
        qWarning() << QString("D3D11CreateDeviceAndSwapChain error: ret=%1").arg(ret);
        return false;
    }

    if (false == CompileShaderFromFile("shader.hlsl", "VS", "vs_5_0", &mVSBlob))
    {
        return false;
    }

    if (false == CompileShaderFromFile("shader.hlsl", "PS", "ps_5_0", &mPSBlob))
    {
        return false;
    }

    ret = mSwapChain->ResizeBuffers(0,
                                    mConfig.renderView->width(),
                                    mConfig.renderView->height(),
                                    DXGI_FORMAT_UNKNOWN,
                                    0);
    if (FAILED(ret))
    {
        qWarning() << QString("mSwapChain->ResizeBuffers error: ret=%1").arg(ret);
        return false;
    }

    ID3D11Texture2D *backBuffer = nullptr;
    ret = mSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&backBuffer);
    if (FAILED(ret) || !backBuffer)
    {
        qWarning() << QString("mSwapChain->GetBuffer error: ret=%1").arg(ret);
        return false;
    }
    mDevice->CreateRenderTargetView(backBuffer, nullptr, &mRenderTargetView);
    backBuffer->Release();

    D3D11_VIEWPORT viewPort = { };
    viewPort.Width = mConfig.renderView->width();
    viewPort.Height = mConfig.renderView->height();
    viewPort.MinDepth = 0.0f;
    viewPort.MaxDepth = 1.0f;
    viewPort.TopLeftX = 0;
    viewPort.TopLeftY = 0;
    mDeviceContext->RSSetViewports(1, &viewPort);

    D3D11_RECT scissor = { };
    scissor.left = 0;
    scissor.top = 0;
    scissor.right = mConfig.renderView->width();
    scissor.bottom = mConfig.renderView->height();
    mDeviceContext->RSSetScissorRects(1, &scissor);

    VERTEX_FMT vertices[] =
    {
        { DirectX::XMFLOAT4(-1.0f, -1.0f, 0, 1.0f), DirectX::XMFLOAT2(0, 1.0) },
        { DirectX::XMFLOAT4(-1.0f, 1.0f, 0, 1.0f), DirectX::XMFLOAT2(0, 0) },
        { DirectX::XMFLOAT4(1.0f, 1.0f, 0, 1.0f), DirectX::XMFLOAT2(1.0, 0) },
        { DirectX::XMFLOAT4(1.0f, 1.0f, 0, 1.0f), DirectX::XMFLOAT2(1.0, 0) },
        { DirectX::XMFLOAT4(1.0f, -1.0f, 0, 1.0f), DirectX::XMFLOAT2(1.0, 1.0) },
        { DirectX::XMFLOAT4(-1.0f, -1.0f, 0, 1.0f), DirectX::XMFLOAT2(0, 1.0) }
    };

    mVerticesNum = sizeof(vertices) / sizeof(VERTEX_FMT);

    D3D11_BUFFER_DESC vertexDesc = { };
    vertexDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vertexDesc.ByteWidth = sizeof(vertices);
    vertexDesc.CPUAccessFlags = 0;
    vertexDesc.MiscFlags = 0;
    vertexDesc.StructureByteStride = 0;
    vertexDesc.Usage = D3D11_USAGE_DEFAULT;

    D3D11_SUBRESOURCE_DATA vertexData = { };
    vertexData.pSysMem = vertices;
    vertexData.SysMemPitch = 0;
    vertexData.SysMemSlicePitch = 0;

    ret = mDevice->CreateBuffer(&vertexDesc, &vertexData, &mVertexBuffer);
    if (FAILED(ret))
    {
        qWarning() << QString("mDevice->CreateBuffer error: ret=%1").arg(ret);
        return false;
    }

    UINT stride = sizeof(VERTEX_FMT);
    UINT offset = 0;
    mDeviceContext->IASetVertexBuffers(0, 1, &mVertexBuffer, &stride, &offset);
    mDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    ret = mDevice->CreateVertexShader(mVSBlob->GetBufferPointer(), mVSBlob->GetBufferSize(), nullptr, &mVertexShader);
    if (FAILED(ret))
    {
        qWarning() << QString("mDevice->CreateVertexShader error: ret=%1").arg(ret);
        return false;
    }
    mDeviceContext->VSSetShader(mVertexShader, nullptr, 0);

    D3D11_INPUT_ELEMENT_DESC layout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 16, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };
    UINT numElements = ARRAYSIZE(layout);

    ret = mDevice->CreateInputLayout(layout, numElements, mVSBlob->GetBufferPointer(), mVSBlob->GetBufferSize(), &mVertexLayout);
    if (FAILED(ret))
    {
        qWarning() << QString("mDevice->CreateInputLayout error: ret=%1").arg(ret);
        return false;
    }
    mDeviceContext->IASetInputLayout(mVertexLayout);

    ret = mDevice->CreatePixelShader(mPSBlob->GetBufferPointer(), mPSBlob->GetBufferSize(), nullptr, &mPixelShader);
    if (FAILED(ret))
    {
        qWarning() << QString("mDevice->CreatePixelShader error: ret=%1").arg(ret);
        return false;
    }
    mDeviceContext->PSSetShader(mPixelShader, nullptr, 0);

    mDeviceContext->OMSetRenderTargets(1, &mRenderTargetView, nullptr);

    float colorBlack[4] = {0.0, 0.0, 0.0, 1.0};
    mDeviceContext->ClearRenderTargetView(mRenderTargetView, colorBlack);

    mSwapChain->Present(1, 0);

    qInfo() << "video render D3D finish init";
    return true;
}

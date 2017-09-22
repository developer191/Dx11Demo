#include <windows.h>
#include <vector>
#include <iostream>

#include "DxCommon.h"
#include "DDSTextureLoader.h"
#include "bmp.h"
// Include shader
#include "VertexShader.h"
#include "PixelShader.h"

#include "resource.h"

using namespace std;

void ProcessCapture(ID3D11Device* pDev, ID3D11DeviceContext* pDevCtx, ID3D11Texture2D *pData)
{
    BOOL isSuccess = TRUE;
    HRESULT hr = S_OK;

    BYTE* pBits = NULL;
    DWORD pitch = 0;

    D3D11_TEXTURE2D_DESC Desc;
    pData->GetDesc(&Desc);

    IDXGISurface2* pCaptureSurf = nullptr;
    ID3D11Texture2D* pCaptureTex = nullptr;

    D3D11_TEXTURE2D_DESC DescCapture;
    DescCapture = Desc;
    DescCapture.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    DescCapture.Usage = D3D11_USAGE_STAGING;
    DescCapture.MipLevels = 1;
    DescCapture.MiscFlags = 0;
    DescCapture.BindFlags = 0;

    hr = pDev->CreateTexture2D(&DescCapture, nullptr, &pCaptureTex);

    SYSTEMTIME curTime = { 0 };
    GetLocalTime(&curTime);
    wchar_t path[MAX_PATH * 2];
    ZeroMemory(path, MAX_PATH * 2 * sizeof(wchar_t));
    GetCurrentDirectory(MAX_PATH * 2, path);

    wchar_t buf[MAX_PATH * 2];
    ZeroMemory(buf, MAX_PATH * 2 * sizeof(wchar_t));

    for (UINT resIdx = 0; (SUCCEEDED(hr)) && (resIdx < Desc.MipLevels); resIdx++)
    {
        swprintf_s(buf, MAX_PATH * 2, L"%s\\%04d%02d%02d_%02d%02d%02d-%03d_%d.bmp",
            path, curTime.wYear, curTime.wMonth, curTime.wDay, curTime.wHour, curTime.wMinute, curTime.wSecond, curTime.wMilliseconds, resIdx);
        pDevCtx->CopySubresourceRegion(pCaptureTex, 0, 0, 0, 0, pData, resIdx, 0);
        pCaptureTex->QueryInterface(__uuidof(IDXGISurface2), (reinterpret_cast<void**>(&pCaptureSurf)));

        DXGI_MAPPED_RECT rawDesktop;
        RtlZeroMemory(&rawDesktop, sizeof(DXGI_MAPPED_RECT));
        pCaptureSurf->Map(&rawDesktop, DXGI_MAP_READ);
        pBits = rawDesktop.pBits;
        pitch = rawDesktop.Pitch;

        BITMAP_FILE outBmp;
        RtlZeroMemory(&outBmp, sizeof(BITMAP_FILE));
        outBmp.bitmapheader.bfType = 0x4D42;
        outBmp.bitmapheader.bfSize = sizeof(outBmp.bitmapheader)
            + sizeof(outBmp.bitmapinfoheader)
            + DescCapture.Width * DescCapture.Height * 4;
        outBmp.bitmapheader.bfOffBits = sizeof(outBmp.bitmapheader)
            + sizeof(outBmp.bitmapinfoheader);

        outBmp.bitmapinfoheader.biSize = sizeof(BITMAPINFOHEADER);
        outBmp.bitmapinfoheader.biWidth = DescCapture.Width;
        outBmp.bitmapinfoheader.biHeight = -((int)DescCapture.Height);// RGB raw data is inversted in BMP
        outBmp.bitmapinfoheader.biPlanes = 1;
        outBmp.bitmapinfoheader.biBitCount = 32;//Should always be DXGI_FORMAT_B8G8R8A8_UNORM
        outBmp.bitmapinfoheader.biCompression = 0;
        outBmp.bitmapinfoheader.biSizeImage = DescCapture.Width * DescCapture.Height * 4;
        outBmp.bitmapinfoheader.biXPelsPerMeter = 0xED8;
        outBmp.bitmapinfoheader.biYPelsPerMeter = 0xED8;
        outBmp.bitmapinfoheader.biClrUsed = 0;
        outBmp.bitmapinfoheader.biClrImportant = 0;

        FILE *pBmpFile = NULL;
        if (_wfopen_s(&pBmpFile, buf, L"wb") == 0)
        {
            // Write header
            fwrite((void*)&outBmp, sizeof(outBmp.bitmapheader) + sizeof(outBmp.bitmapinfoheader), 1, pBmpFile);

            // Write raw data
            if (DescCapture.Width * 4 == pitch) // Write in burst
            {
                fwrite(pBits, DescCapture.Width * DescCapture.Height * 4, 1, pBmpFile);
            }
            else // Write line by line
            {
                for (unsigned int line = 0; line < DescCapture.Height; line++)
                {
                    fwrite(pBits + line * pitch, DescCapture.Width * 4, 1, pBmpFile);
                }
            }
            fclose(pBmpFile);
        }

        if (pCaptureSurf)
        {
            pCaptureSurf->Unmap();
            pCaptureSurf->Release();
            pCaptureSurf = nullptr;
        }
    }
    if (pCaptureTex)
    {
        pCaptureTex->Release();
    }
}

//--------------------------------------------------------------------------------------
// Global Variables
//--------------------------------------------------------------------------------------
HINSTANCE                           g_hInst = NULL;
HWND                                g_hMainWnd = NULL;
D3D_DRIVER_TYPE                     g_driverType = D3D_DRIVER_TYPE_NULL;
D3D_FEATURE_LEVEL                   g_featureLevel = D3D_FEATURE_LEVEL_11_0;
ID3D11Device*                       g_pd3dDevice = NULL;
ID3D11DeviceContext*                g_pImmediateContext = NULL;
IDXGISwapChain*                     g_pSwapChain = NULL;
ID3D11Texture2D*                    g_pBackBuffer = NULL;
ID3D11RenderTargetView*             g_pRenderTargetView = NULL;
ID3D11Texture2D*                    g_pDepthStencil = NULL;
ID3D11DepthStencilView*             g_pDepthStencilView = NULL;
ID3D11VertexShader*                 g_pVertexShader = NULL;
ID3D11PixelShader*                  g_pPixelShader = NULL;
ID3D11InputLayout*                  g_pVertexLayout = NULL;
ID3D11Buffer*                       g_pVertexBuffer = NULL;
ID3D11Buffer*                       g_pIndexBuffer = NULL;
ID3D11Buffer*                       g_pCBNeverChanges = NULL;
ID3D11Buffer*                       g_pCBChangeOnResize = NULL;
ID3D11Buffer*                       g_pCBChangesEveryFrame = NULL;
ID3D11Resource*                     g_pTexture = NULL;
ID3D11ShaderResourceView*           g_pTextureRV = NULL;
ID3D11SamplerState*                 g_pSamplerLinear = NULL;
XMMATRIX                            g_World;
XMMATRIX                            g_View;
XMMATRIX                            g_Projection;
XMFLOAT4                            g_vMeshColor( 0.7f, 0.7f, 0.7f, 1.0f );

ID3D11VideoDevice                   *g_pVideoDevice = NULL;
ID3D11VideoContext                  *g_pVideoContext = NULL;
ID3D11VideoProcessor                *g_pVideoProcessor = NULL;
ID3D11VideoProcessorInputView       *g_pInputView = NULL;
ID3D11VideoProcessorOutputView      *g_pOutputView = NULL;
ID3D11Texture2D*                    g_pTexRGB = NULL;
ID3D11Texture2D*                    g_pTexYUV = NULL;
ID3D11Texture2D*                    g_pTexYUVCPU = NULL;
D3D11_TEXTURE2D_DESC                g_TexDesc = {0};

//--------------------------------------------------------------------------------------
// Forward declarations
//--------------------------------------------------------------------------------------
HRESULT InitWindow(HINSTANCE, int);
HRESULT InitDevice();
void CleanupDevice();
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
void Render();

//--------------------------------------------------------------------------------------
// Entry point to the program. Initializes everything and goes into a message processing 
// loop. Idle time is used to render the scene.
//--------------------------------------------------------------------------------------
int WINAPI wWinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow )
{
    UNREFERENCED_PARAMETER( hPrevInstance );
    UNREFERENCED_PARAMETER( lpCmdLine );

    if( FAILED( InitWindow( hInstance, nCmdShow ) ) )
        return 0;

    if( FAILED( InitDevice() ) )
    {
        CleanupDevice();
        return 0;
    }
	

    // Main message loop
    MSG msg = {0};
    while( WM_QUIT != msg.message )
    {
        if( PeekMessage( &msg, NULL, 0, 0, PM_REMOVE ) )
        {
            TranslateMessage( &msg );
            DispatchMessage( &msg );
        }
        else
        {
            Render();
        }
    }

    CleanupDevice();

    return ( int )msg.wParam;
}

//--------------------------------------------------------------------------------------
// Register class and create window
//--------------------------------------------------------------------------------------
HRESULT InitWindow( HINSTANCE hInstance, int nCmdShow )
{
    // Register class
    WNDCLASSEX wcex;
    wcex.cbSize = sizeof( WNDCLASSEX );
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon( hInstance, ( LPCTSTR )IDI_APPICON);
    wcex.hCursor = LoadCursor( NULL, IDC_ARROW );
    wcex.hbrBackground = ( HBRUSH )( COLOR_WINDOW + 1 );
    wcex.lpszMenuName = NULL;
    wcex.lpszClassName = L"Dx11DemoWindowClass";
    wcex.hIconSm = LoadIcon( wcex.hInstance, ( LPCTSTR )IDI_APPICON);
    if( !RegisterClassEx( &wcex ) )
        return E_FAIL;

    // Create window
    g_hInst = hInstance;
    RECT rc = { 0, 0, 640, 480 };
    AdjustWindowRect( &rc, WS_OVERLAPPEDWINDOW, FALSE );
    g_hMainWnd = CreateWindow( L"Dx11DemoWindowClass", L"Dx11Demo", WS_OVERLAPPEDWINDOW,
                           CW_USEDEFAULT, CW_USEDEFAULT, rc.right - rc.left, rc.bottom - rc.top, NULL, NULL, hInstance,
                           NULL );
    if (!g_hMainWnd)
    {
        return E_FAIL;
    }

    ShowWindow(g_hMainWnd, nCmdShow);
    UpdateWindow(g_hMainWnd);

    return S_OK;
}

//--------------------------------------------------------------------------------------
// Create Direct3D device and swap chain
//--------------------------------------------------------------------------------------
HRESULT InitDevice()
{
    HRESULT hr = S_OK;

    RECT rc;
    GetClientRect(g_hMainWnd, &rc);
    UINT width = rc.right - rc.left;
    UINT height = rc.bottom - rc.top;

    UINT createDeviceFlags = 0;
#ifdef _DEBUG
    createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    D3D_DRIVER_TYPE driverTypes[] =
    {
        D3D_DRIVER_TYPE_HARDWARE,
        D3D_DRIVER_TYPE_WARP,
        D3D_DRIVER_TYPE_REFERENCE,
    };
    UINT numDriverTypes = ARRAYSIZE( driverTypes );

    D3D_FEATURE_LEVEL featureLevels[] =
    {
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0,
    };
    UINT numFeatureLevels = ARRAYSIZE( featureLevels );

    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory( &sd, sizeof( sd ) );
    sd.BufferCount = 1;
    sd.BufferDesc.Width = width;
    sd.BufferDesc.Height = height;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = g_hMainWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    //sd.SwapEffect = DXGI_SWAP_EFFECT_SEQUENTIAL;

    for( UINT driverTypeIndex = 0; driverTypeIndex < numDriverTypes; driverTypeIndex++ )
    {
        g_driverType = driverTypes[driverTypeIndex];
        hr = D3D11CreateDeviceAndSwapChain( NULL, g_driverType, NULL, createDeviceFlags, featureLevels, numFeatureLevels,
                                            D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &g_featureLevel, &g_pImmediateContext );
        if( SUCCEEDED( hr ) )
            break;
    }
    if( FAILED( hr ) )
        return hr;

    // Create a render target view
    hr = g_pSwapChain->GetBuffer( 0, __uuidof( ID3D11Texture2D ), ( LPVOID* )&g_pBackBuffer);
    if( FAILED( hr ) )
        return hr;

    hr = g_pd3dDevice->CreateRenderTargetView(g_pBackBuffer, NULL, &g_pRenderTargetView );
    if( FAILED( hr ) )
        return hr;

    // Create depth stencil texture
    D3D11_TEXTURE2D_DESC descDepth;
    ZeroMemory( &descDepth, sizeof(descDepth) );
    descDepth.Width = width;
    descDepth.Height = height;
    descDepth.MipLevels = 1;
    descDepth.ArraySize = 1;
    descDepth.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    descDepth.SampleDesc.Count = 1;
    descDepth.SampleDesc.Quality = 0;
    descDepth.Usage = D3D11_USAGE_DEFAULT;
    descDepth.BindFlags = D3D11_BIND_DEPTH_STENCIL;
    descDepth.CPUAccessFlags = 0;
    descDepth.MiscFlags = 0;
    hr = g_pd3dDevice->CreateTexture2D( &descDepth, NULL, &g_pDepthStencil );
    if( FAILED( hr ) )
        return hr;

    // Create the depth stencil view
    D3D11_DEPTH_STENCIL_VIEW_DESC descDSV;
    ZeroMemory( &descDSV, sizeof(descDSV) );
    descDSV.Format = descDepth.Format;
    descDSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
    descDSV.Texture2D.MipSlice = 0;
    hr = g_pd3dDevice->CreateDepthStencilView( g_pDepthStencil, &descDSV, &g_pDepthStencilView );
    if( FAILED( hr ) )
        return hr;

    g_pImmediateContext->OMSetRenderTargets( 1, &g_pRenderTargetView, g_pDepthStencilView );

    // Setup the viewport
    D3D11_VIEWPORT vp;
    vp.Width = (FLOAT)width;
    vp.Height = (FLOAT)height;
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    vp.TopLeftX = 0;
    vp.TopLeftY = 0;
    g_pImmediateContext->RSSetViewports( 1, &vp );

    // Compile the vertex shader
    hr = g_pd3dDevice->CreateVertexShader(g_VS, ARRAYSIZE(g_VS), nullptr, &g_pVertexShader);

    // Define the input layout
    D3D11_INPUT_ELEMENT_DESC layout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };
    UINT numElements = ARRAYSIZE( layout );

    // Create the input layout
    hr = g_pd3dDevice->CreateInputLayout( layout, numElements, g_VS, ARRAYSIZE(g_VS), &g_pVertexLayout );
    if (FAILED(hr))
    {
        return hr;
    }

    // Set the input layout
    g_pImmediateContext->IASetInputLayout( g_pVertexLayout );

    // Compile the pixel shader
    hr = g_pd3dDevice->CreatePixelShader(g_PS, ARRAYSIZE(g_PS), nullptr, &g_pPixelShader);
    if (FAILED(hr))
    {
        return hr;
    }

    // Create vertex buffer
    VERTEX vertices[] =
    {
        { XMFLOAT3( -1.0f, 1.0f, -1.0f ), XMFLOAT2( 0.0f, 0.0f ) },
        { XMFLOAT3( 1.0f, 1.0f, -1.0f ), XMFLOAT2( 1.0f, 0.0f ) },
        { XMFLOAT3( 1.0f, 1.0f, 1.0f ), XMFLOAT2( 1.0f, 1.0f ) },
        { XMFLOAT3( -1.0f, 1.0f, 1.0f ), XMFLOAT2( 0.0f, 1.0f ) },

        { XMFLOAT3( -1.0f, -1.0f, -1.0f ), XMFLOAT2( 0.0f, 0.0f ) },
        { XMFLOAT3( 1.0f, -1.0f, -1.0f ), XMFLOAT2( 1.0f, 0.0f ) },
        { XMFLOAT3( 1.0f, -1.0f, 1.0f ), XMFLOAT2( 1.0f, 1.0f ) },
        { XMFLOAT3( -1.0f, -1.0f, 1.0f ), XMFLOAT2( 0.0f, 1.0f ) },

        { XMFLOAT3( -1.0f, -1.0f, 1.0f ), XMFLOAT2( 0.0f, 0.0f ) },
        { XMFLOAT3( -1.0f, -1.0f, -1.0f ), XMFLOAT2( 1.0f, 0.0f ) },
        { XMFLOAT3( -1.0f, 1.0f, -1.0f ), XMFLOAT2( 1.0f, 1.0f ) },
        { XMFLOAT3( -1.0f, 1.0f, 1.0f ), XMFLOAT2( 0.0f, 1.0f ) },

        { XMFLOAT3( 1.0f, -1.0f, 1.0f ), XMFLOAT2( 0.0f, 0.0f ) },
        { XMFLOAT3( 1.0f, -1.0f, -1.0f ), XMFLOAT2( 1.0f, 0.0f ) },
        { XMFLOAT3( 1.0f, 1.0f, -1.0f ), XMFLOAT2( 1.0f, 1.0f ) },
        { XMFLOAT3( 1.0f, 1.0f, 1.0f ), XMFLOAT2( 0.0f, 1.0f ) },

        { XMFLOAT3( -1.0f, -1.0f, -1.0f ), XMFLOAT2( 0.0f, 0.0f ) },
        { XMFLOAT3( 1.0f, -1.0f, -1.0f ), XMFLOAT2( 1.0f, 0.0f ) },
        { XMFLOAT3( 1.0f, 1.0f, -1.0f ), XMFLOAT2( 1.0f, 1.0f ) },
        { XMFLOAT3( -1.0f, 1.0f, -1.0f ), XMFLOAT2( 0.0f, 1.0f ) },

        { XMFLOAT3( -1.0f, -1.0f, 1.0f ), XMFLOAT2( 0.0f, 0.0f ) },
        { XMFLOAT3( 1.0f, -1.0f, 1.0f ), XMFLOAT2( 1.0f, 0.0f ) },
        { XMFLOAT3( 1.0f, 1.0f, 1.0f ), XMFLOAT2( 1.0f, 1.0f ) },
        { XMFLOAT3( -1.0f, 1.0f, 1.0f ), XMFLOAT2( 0.0f, 1.0f ) },
    };

    D3D11_BUFFER_DESC bd;
    ZeroMemory( &bd, sizeof(bd) );
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = sizeof(VERTEX) * 24;
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    bd.CPUAccessFlags = 0;
    D3D11_SUBRESOURCE_DATA InitData;
    ZeroMemory( &InitData, sizeof(InitData) );
    InitData.pSysMem = vertices;
    hr = g_pd3dDevice->CreateBuffer( &bd, &InitData, &g_pVertexBuffer );
    if( FAILED( hr ) )
        return hr;

    // Set vertex buffer
    UINT stride = sizeof(VERTEX);
    UINT offset = 0;
    g_pImmediateContext->IASetVertexBuffers( 0, 1, &g_pVertexBuffer, &stride, &offset );

    // Create index buffer
    // Create vertex buffer
    WORD indices[] =
    {
        3,1,0,
        2,1,3,

        6,4,5,
        7,4,6,

        11,9,8,
        10,9,11,

        14,12,13,
        15,12,14,

        19,17,16,
        18,17,19,

        22,20,21,
        23,20,22
    };

    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = sizeof( WORD ) * 36;
    bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
    bd.CPUAccessFlags = 0;
    InitData.pSysMem = indices;
    hr = g_pd3dDevice->CreateBuffer( &bd, &InitData, &g_pIndexBuffer );
    if( FAILED( hr ) )
        return hr;

    // Set index buffer
    g_pImmediateContext->IASetIndexBuffer( g_pIndexBuffer, DXGI_FORMAT_R16_UINT, 0 );

    // Set primitive topology
    g_pImmediateContext->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST );

    // Create the constant buffers
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = sizeof(CBNeverChanges);
    bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    bd.CPUAccessFlags = 0;
    hr = g_pd3dDevice->CreateBuffer( &bd, NULL, &g_pCBNeverChanges );
    if (FAILED(hr))
    {
        return hr;
    }
    
    bd.ByteWidth = sizeof(CBChangeOnResize);
    hr = g_pd3dDevice->CreateBuffer( &bd, NULL, &g_pCBChangeOnResize );
    if (FAILED(hr))
    {
        return hr;
    }
    
    bd.ByteWidth = sizeof(CBChangesEveryFrame);
    hr = g_pd3dDevice->CreateBuffer( &bd, NULL, &g_pCBChangesEveryFrame );
    if (FAILED(hr))
    {
        return hr;
    }

    // Load texture from resource
    HRSRC texRes = ::FindResource(NULL, MAKEINTRESOURCE(IDR_TEXTURE1), MAKEINTRESOURCE(RC_TEXTURE));
    unsigned int texResSize = ::SizeofResource(g_hInst, texRes);
    HGLOBAL texResData = ::LoadResource(NULL, texRes);
    void* pTexResBinary = ::LockResource(texResData);

    hr = CreateDDSTextureFromMemory(g_pd3dDevice, (const uint8_t*)pTexResBinary, (size_t)texResSize, &g_pTexture, &g_pTextureRV);

    /*TexMetadata mdata;
    hr = GetMetadataFromDDSMemory(pTexResBinary, texResSize, DDS_FLAGS_NONE, mdata);
    ScratchImage image;
    hr = LoadFromDDSMemory(pTexResBinary, texResSize, DDS_FLAGS_NONE, &mdata, image);
    hr = CreateShaderResourceView(g_pd3dDevice, image.GetImages(), image.GetImageCount(), mdata, &g_pTextureRV);*/
    if (FAILED(hr))
    {
        return hr;
    }

    g_pBackBuffer->GetDesc(&g_TexDesc);

    g_TexDesc.Format = DXGI_FORMAT_NV12;
    g_TexDesc.Usage = D3D11_USAGE_STAGING;
    g_TexDesc.BindFlags = 0;
    g_TexDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    hr = g_pd3dDevice->CreateTexture2D(&g_TexDesc, NULL, &g_pTexYUVCPU);
    if (FAILED(hr))
    {
        return hr;
    }

    g_TexDesc.Format = DXGI_FORMAT_NV12;
    g_TexDesc.Usage = D3D11_USAGE_DEFAULT;
    g_TexDesc.BindFlags = 0;
    g_TexDesc.CPUAccessFlags = 0;
    hr = g_pd3dDevice->CreateTexture2D(&g_TexDesc, NULL, &g_pTexYUV);
    if (FAILED(hr))
    {
        return hr;
    }

    g_pImmediateContext->CopyResource(g_pTexYUV, g_pTexYUVCPU);

    g_TexDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    g_TexDesc.Usage = D3D11_USAGE_DEFAULT;
    g_TexDesc.BindFlags = D3D11_BIND_RENDER_TARGET;
    g_TexDesc.CPUAccessFlags = 0;
    hr = g_pd3dDevice->CreateTexture2D(&g_TexDesc, NULL, &g_pTexRGB);
    if (FAILED(hr))
    {
        return hr;
    }

    hr = g_pd3dDevice->QueryInterface(__uuidof(ID3D11VideoDevice), (void **)&g_pVideoDevice);
    if (FAILED(hr))
    {
        return hr;
    }

    hr = g_pImmediateContext->QueryInterface(__uuidof(ID3D11VideoContext), (void **)&g_pVideoContext);
    if (FAILED(hr))
    {
        return hr;
    }

    D3D11_VIDEO_PROCESSOR_CONTENT_DESC contentDesc = {
        D3D11_VIDEO_FRAME_FORMAT_PROGRESSIVE,
        { 1, 1 }, g_TexDesc.Width, g_TexDesc.Height,
        { 1, 1 }, g_TexDesc.Width, g_TexDesc.Height,
        D3D11_VIDEO_USAGE_PLAYBACK_NORMAL
    };
    ID3D11VideoProcessorEnumerator *pVideoProcessorEnumerator = NULL;
    hr = g_pVideoDevice->CreateVideoProcessorEnumerator(&contentDesc, &pVideoProcessorEnumerator);
    if (FAILED(hr))
    {
        return hr;
    }

    hr = g_pVideoDevice->CreateVideoProcessor(pVideoProcessorEnumerator, 0, &g_pVideoProcessor);
    if (FAILED(hr))
    {
        return hr;
    }

    D3D11_VIDEO_PROCESSOR_INPUT_VIEW_DESC inputViewDesc;
    inputViewDesc.FourCC = 0;
    inputViewDesc.ViewDimension = D3D11_VPIV_DIMENSION_TEXTURE2D;
    inputViewDesc.Texture2D.ArraySlice = 0;
    inputViewDesc.Texture2D.MipSlice = 0;
    hr = g_pVideoDevice->CreateVideoProcessorInputView(g_pTexYUV, pVideoProcessorEnumerator, &inputViewDesc, &g_pInputView);
    if (FAILED(hr))
    {
        return hr;
    }

    D3D11_VIDEO_PROCESSOR_OUTPUT_VIEW_DESC outputViewDesc;
    outputViewDesc.ViewDimension = D3D11_VPOV_DIMENSION_TEXTURE2D;
    outputViewDesc.Texture2D.MipSlice = 0;
    hr = g_pVideoDevice->CreateVideoProcessorOutputView(g_pTexRGB, pVideoProcessorEnumerator, &outputViewDesc, &g_pOutputView);
    if (FAILED(hr))
    {
        return hr;
    }

    pVideoProcessorEnumerator->Release();

    // Create the sample state
    D3D11_SAMPLER_DESC sampDesc;
    ZeroMemory( &sampDesc, sizeof(sampDesc) );
    sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    sampDesc.MinLOD = 0;
    sampDesc.MaxLOD = D3D11_FLOAT32_MAX;
    hr = g_pd3dDevice->CreateSamplerState( &sampDesc, &g_pSamplerLinear );
    if (FAILED(hr))
    {
        return hr;
    }

    // Initialize the world matrices
    g_World = XMMatrixIdentity();

    // Initialize the view matrix
    XMVECTOR Eye = XMVectorSet( 0.0f, 3.0f, -6.0f, 0.0f );
    XMVECTOR At = XMVectorSet( 0.0f, 1.0f, 0.0f, 0.0f );
    XMVECTOR Up = XMVectorSet( 0.0f, 1.0f, 0.0f, 0.0f );
    g_View = XMMatrixLookAtLH( Eye, At, Up );

    CBNeverChanges cbNeverChanges;
    cbNeverChanges.mView = XMMatrixTranspose( g_View );
    g_pImmediateContext->UpdateSubresource( g_pCBNeverChanges, 0, NULL, &cbNeverChanges, 0, 0 );

    // Initialize the projection matrix
    g_Projection = XMMatrixPerspectiveFovLH( XM_PIDIV4, width / (FLOAT)height, 0.01f, 100.0f );
    
    CBChangeOnResize cbChangesOnResize;
    cbChangesOnResize.mProjection = XMMatrixTranspose( g_Projection );
    g_pImmediateContext->UpdateSubresource( g_pCBChangeOnResize, 0, NULL, &cbChangesOnResize, 0, 0 );

    return S_OK;
}


//--------------------------------------------------------------------------------------
// Clean up the objects we've created
//--------------------------------------------------------------------------------------
void CleanupDevice()
{
    if (g_pTexRGB) g_pTexRGB->Release();
    if (g_pTexYUVCPU) g_pTexYUVCPU->Release();
    if( g_pTexYUV ) g_pTexYUV->Release();
    if (g_pOutputView) g_pOutputView->Release();
    if (g_pInputView) g_pInputView->Release();
    if (g_pVideoProcessor) g_pVideoProcessor->Release();
    if (g_pVideoContext) g_pVideoContext->Release();
    if (g_pVideoDevice) g_pVideoDevice->Release();
    if( g_pImmediateContext ) g_pImmediateContext->ClearState();
    if( g_pSamplerLinear ) g_pSamplerLinear->Release();
    if( g_pTextureRV ) g_pTextureRV->Release();
    if( g_pCBNeverChanges ) g_pCBNeverChanges->Release();
    if( g_pCBChangeOnResize ) g_pCBChangeOnResize->Release();
    if( g_pCBChangesEveryFrame ) g_pCBChangesEveryFrame->Release();
    if( g_pVertexBuffer ) g_pVertexBuffer->Release();
    if( g_pIndexBuffer ) g_pIndexBuffer->Release();
    if( g_pVertexLayout ) g_pVertexLayout->Release();
    if( g_pVertexShader ) g_pVertexShader->Release();
    if( g_pPixelShader ) g_pPixelShader->Release();
    if( g_pDepthStencil ) g_pDepthStencil->Release();
    if( g_pDepthStencilView ) g_pDepthStencilView->Release();
    if( g_pRenderTargetView ) g_pRenderTargetView->Release();
    if (g_pBackBuffer) g_pBackBuffer->Release();
    if( g_pSwapChain ) g_pSwapChain->Release();
    if( g_pImmediateContext ) g_pImmediateContext->Release();
    if( g_pd3dDevice ) g_pd3dDevice->Release();
}


//--------------------------------------------------------------------------------------
// Called every time the application receives a message
//--------------------------------------------------------------------------------------
LRESULT CALLBACK WndProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
{
    PAINTSTRUCT ps;
    HDC hdc;

    switch( message )
    {
        case WM_PAINT:
            hdc = BeginPaint( hWnd, &ps );
            EndPaint( hWnd, &ps );
            break;

        case WM_DESTROY:
            PostQuitMessage( 0 );
            break;

        case WM_SIZE:
            break;

        case WM_MOVE:
            break;

        default:
            return DefWindowProc( hWnd, message, wParam, lParam );
    }

    return 0;
}

//--------------------------------------------------------------------------------------
// Render a frame
//--------------------------------------------------------------------------------------
void Render()
{
    // Update our time
    static float t = 0.0f;
    if( g_driverType == D3D_DRIVER_TYPE_REFERENCE )
    {
        t += ( float )XM_PI * 0.00125f;
    }
    else
    {
        static DWORD dwTimeStart = 0;
        DWORD dwTimeCur = GetTickCount();
        if( dwTimeStart == 0 )
            dwTimeStart = dwTimeCur;
        t = ( dwTimeCur - dwTimeStart ) / 300.0f;
    }

    // Rotate cube around the origin
    g_World = XMMatrixRotationY( t );
    //Debug On
    D3D11_TEXTURE2D_DESC bbDesc = { 0 };
    g_pBackBuffer->GetDesc(&bbDesc);
    HRESULT hr = S_OK;
    ID3D11Texture2D* pRT = NULL;
    ID3D11RenderTargetView *pRTV = NULL;
    D3D11_TEXTURE2D_DESC texDesc = { 0 };
    texDesc.Width = bbDesc.Width;
    texDesc.Height = bbDesc.Height;
    texDesc.MipLevels = 1;
    texDesc.ArraySize = 1;
    texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    texDesc.SampleDesc.Count = 1;
    texDesc.SampleDesc.Quality = 0;
    texDesc.Usage = D3D11_USAGE_DEFAULT;
    texDesc.CPUAccessFlags = 0;
    texDesc.MiscFlags = 0;
    texDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
    hr = g_pd3dDevice->CreateTexture2D(&texDesc, NULL, &pRT);
    D3D11_RENDER_TARGET_VIEW_DESC rtvDesc;
    rtvDesc.Format = texDesc.Format;
    rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
    rtvDesc.Texture2D.MipSlice = 0;
    hr = g_pd3dDevice->CreateRenderTargetView(pRT, &rtvDesc, &pRTV);

    //Debug Off

    // Modify the color
    //g_vMeshColor.x = ( sinf( t * 1.0f ) + 1.0f ) * 0.5f;
    //g_vMeshColor.y = ( cosf( t * 3.0f ) + 1.0f ) * 0.5f;
    //g_vMeshColor.z = ( sinf( t * 5.0f ) + 1.0f ) * 0.5f;
    g_pImmediateContext->OMSetRenderTargets(1, &g_pRenderTargetView, NULL);

    //
    // Clear the back buffer
    //
    float ClearColor[4] = { 0.0f, 0.125f, 0.3f, 1.0f }; // red, green, blue, alpha
    g_pImmediateContext->ClearRenderTargetView( g_pRenderTargetView, ClearColor );

    //
    // Clear the depth buffer to 1.0 (max depth)
    //
    //g_pImmediateContext->ClearDepthStencilView( g_pDepthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0 );

    //
    // Update variables that change once per frame
    //
    CBChangesEveryFrame cb;
    cb.mWorld = XMMatrixTranspose( g_World );
    cb.vMeshColor = g_vMeshColor;
    cb.fBrightnessParam.x = 0.011f;
    cb.fBrightnessParam.y = 1.0f;
    g_pImmediateContext->UpdateSubresource( g_pCBChangesEveryFrame, 0, NULL, &cb, 0, 0 );

    //
    // Render the cube
    //
    g_pImmediateContext->VSSetShader( g_pVertexShader, NULL, 0 );
    g_pImmediateContext->VSSetConstantBuffers( 0, 1, &g_pCBNeverChanges );
    g_pImmediateContext->VSSetConstantBuffers( 1, 1, &g_pCBChangeOnResize );
    g_pImmediateContext->VSSetConstantBuffers( 2, 1, &g_pCBChangesEveryFrame );
    g_pImmediateContext->PSSetShader( g_pPixelShader, NULL, 0 );
    //g_pImmediateContext->PSSetConstantBuffers( 2, 1, &g_pCBChangesEveryFrame );
    g_pImmediateContext->PSSetShaderResources( 0, 1, &g_pTextureRV );
    g_pImmediateContext->PSSetSamplers( 0, 1, &g_pSamplerLinear );
    g_pImmediateContext->DrawIndexed( 36, 0, 0 );

    g_pImmediateContext->CopyResource(g_pTexRGB, g_pBackBuffer);
    //ProcessCapture(g_pd3dDevice, g_pImmediateContext, pRT);
    D3D11_VIDEO_PROCESSOR_STREAM stream = { TRUE, 0, 0, 0, 0, NULL, g_pInputView, NULL };
    hr = S_OK;
    hr = g_pVideoContext->VideoProcessorBlt(g_pVideoProcessor, g_pOutputView, 0, 1, &stream);

    //
    // Present our back buffer to our front buffer
    //
    g_pSwapChain->Present( 0, 0 );
}

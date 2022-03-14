#ifndef UNICODE
#define UNICODE
#endif

#include <chrono>
#include <thread>
#include <stdlib.h>

#include <windows.h>
#include <windowsx.h>
#include <iostream>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <filesystem>
#include <wrl/client.h>
using Microsoft::WRL::ComPtr;

#pragma comment(lib,"d3dcompiler.lib")
#pragma comment (lib, "d3d11.lib")

IDXGISwapChain* m_swapchain;
ID3D11Device* m_dev;
ID3D11DeviceContext* m_devcon;
ID3D11RenderTargetView* m_backbuffer;
ID3D11InputLayout* m_pLayout;
ID3D11VertexShader* m_pVs;
ID3D11PixelShader* m_pPs;
ID3D11Buffer* m_pVBuffer;

struct COLOR {
    FLOAT r, g, b, a;
};

struct VERTEX {
    FLOAT x, y, z;
    COLOR color;
};

#define SCREEN_WIDTH 1920
#define SCREEN_HEIGHT 1080

int m_defaultWidth = 1920;
int m_defaultHeight = 1080;
bool FULLSCREEN = FALSE;

const wchar_t CLASS_NAME[] = L"SAMPLECLASSNAME";

HINSTANCE m_hInstance;

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
HRESULT CompileShader(_In_ LPCWSTR srcFile, _In_ LPCSTR entryPoint, _In_ LPCSTR profile, _Outptr_ ID3DBlob** blob);
void InitD3D(HWND hwnd);
void RenderFrame(void);
void CleanD3D(void);
void InitGraphics(void);
void InitPipeline(void);

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow) {

    WNDCLASS wc = { };

    wc.style = CS_DBLCLKS;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    m_hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;

    RegisterClass(&wc);

    RECT wr = { 0, 0, m_defaultWidth, m_defaultHeight };
    AdjustWindowRect(&wr, WS_OVERLAPPEDWINDOW, FALSE);

    HWND hwnd = CreateWindowEx(
        0,//WS_EX_NOACTIVATE | WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TRANSPARENT,                              // Optional window styles.
        CLASS_NAME,                     // Window class
        L"Sample Window",                 // Window text
        WS_OVERLAPPEDWINDOW,//WS_POPUP            // Window style

        // Size and position
        SCREEN_WIDTH / 2 - (m_defaultWidth / 2),
        SCREEN_HEIGHT / 2 - (m_defaultHeight / 2),
        wr.right - wr.left,
        wr.bottom - wr.top,
        NULL,       // Parent window    
        NULL,       // Menu
        hInstance,  // Instance handle
        NULL        // Additional application data
    );

    if (hwnd == NULL) {
        return 0;
    }

    ShowWindow(hwnd, nCmdShow);

    //-------DEBUG CONSOLE--------//
    /*FILE* fDummy;
    AllocConsole();
    freopen_s(&fDummy, "CONOUT$", "w", stdout);*/
    //----------------------------//

    bool bGotMsg;
    MSG  msg;
    msg.message = WM_NULL;

    InitD3D(hwnd);
    
    bool bFadeDir = TRUE;

    while (TRUE) {
        bGotMsg = (PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE) != 0);

        if (bGotMsg) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            
            if (WM_QUIT == msg.message) {
                break;
            }

        } else {
            RenderFrame();
            //renderer->Update();

            //renderer->Render();

            //deviceResources->Present();
        }
    }

    CleanD3D();

    return 0;

}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_CLOSE:
    {
        HMENU hMenu;
        hMenu = GetMenu(hwnd);
        if (hMenu != NULL) {
            DestroyMenu(hMenu);
        }
        DestroyWindow(hwnd);
        UnregisterClass(
            CLASS_NAME,
            m_hInstance
        );
        return 0;
    }

    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    }


    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

void InitD3D(HWND hwnd) {
    DXGI_SWAP_CHAIN_DESC scd;

    ZeroMemory(&scd, sizeof(DXGI_SWAP_CHAIN_DESC));

    scd.BufferCount = 1;
    scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    scd.BufferDesc.Width = SCREEN_WIDTH;
    scd.BufferDesc.Height = SCREEN_HEIGHT;
    scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    scd.OutputWindow = hwnd;
    scd.SampleDesc.Count = 4;
    scd.Windowed = TRUE;
    scd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

    D3D11CreateDeviceAndSwapChain(NULL,
        D3D_DRIVER_TYPE_HARDWARE,
        NULL,
        NULL,
        NULL,
        NULL,
        D3D11_SDK_VERSION,
        &scd,
        &m_swapchain,
        &m_dev,
        NULL,
        &m_devcon);

    ID3D11Texture2D* pBackBuffer;
    m_swapchain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);

    if (FULLSCREEN) {
        m_swapchain->SetFullscreenState(TRUE, NULL);
    }
   

    m_dev->CreateRenderTargetView(pBackBuffer, NULL, &m_backbuffer);
    pBackBuffer->Release();

    m_devcon->OMSetRenderTargets(1, &m_backbuffer, NULL);

    D3D11_VIEWPORT viewport;
    ZeroMemory(&viewport, sizeof(D3D11_VIEWPORT));

    viewport.TopLeftX = 0;
    viewport.TopLeftY = 0;
    viewport.Width = m_defaultWidth;
    viewport.Height = m_defaultHeight;
    
    m_devcon->RSSetViewports(1, &viewport);

    InitPipeline();
    InitGraphics();
}

void RenderFrame(void) {
    FLOAT bgcolor[4] = { 0.0f, 0.2f, 0.4f, 1.0f };
    m_devcon->ClearRenderTargetView(m_backbuffer, bgcolor);

    UINT stride = sizeof(VERTEX);
    UINT offset = 0;

    m_devcon->IASetVertexBuffers(0, 1, &m_pVBuffer, &stride, &offset);
    m_devcon->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    m_devcon->Draw(3, 0);

    m_swapchain->Present(0, 0);
}

void CleanD3D() {
    m_swapchain->SetFullscreenState(FALSE, NULL);

    m_pLayout->Release();
    m_pVs->Release();
    m_pPs->Release();
    m_pVBuffer->Release();
    m_swapchain->Release();
    m_backbuffer->Release();
    m_dev->Release();
    m_devcon->Release();
}

void InitGraphics() {
    VERTEX triangleVertices[] = {
        {0.0f, 0.5f, 0.0f, {1.0f,0.0f,0.0f,1.0f}},
        {0.45f, -0.5f, 0.0f, {0.0f,1.0f,0.0f,1.0f}},
        {-0.45f, -0.5f, 0.0f, {0.0f,0.0f,1.0f,1.0f}}
    };

    D3D11_BUFFER_DESC bd;
    ZeroMemory(&bd, sizeof(bd));

    bd.Usage = D3D11_USAGE_DYNAMIC;
    bd.ByteWidth = sizeof(VERTEX) * 3;
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    m_dev->CreateBuffer(&bd, NULL, &m_pVBuffer);

    D3D11_MAPPED_SUBRESOURCE ms;
    m_devcon->Map(m_pVBuffer, NULL, D3D11_MAP_WRITE_DISCARD, NULL, &ms);
    memcpy(ms.pData, triangleVertices, sizeof(triangleVertices));
    m_devcon->Unmap(m_pVBuffer, NULL);

}

void InitPipeline() {
    ComPtr<ID3D10Blob> VS, PS, Errors;

    D3DCompileFromFile(L"D:\\Projekty\\C++\\SimpleDirectX11Rendering\\SimpleDirectX11Rendering\\x64\\Debug\\shaders.shader", nullptr, nullptr, "VShader", "vs_4_0", 0, 0, &VS, &Errors);
    if (Errors) {
        OutputDebugStringA(reinterpret_cast<const char*>(Errors->GetBufferPointer()));
    }
    D3DCompileFromFile(L"D:\\Projekty\\C++\\SimpleDirectX11Rendering\\SimpleDirectX11Rendering\\x64\\Debug\\shaders.shader", nullptr, nullptr, "PShader", "ps_4_0", 0, 0, &PS, &Errors);
    if (Errors) {
        OutputDebugStringA(reinterpret_cast<const char*>(Errors->GetBufferPointer()));
    }

    m_dev->CreateVertexShader(VS->GetBufferPointer(), VS->GetBufferSize(), nullptr, &m_pVs);
    m_dev->CreatePixelShader(PS->GetBufferPointer(), PS->GetBufferSize(), nullptr, &m_pPs);

    m_devcon->VSSetShader(m_pVs, 0, 0);
    m_devcon->PSSetShader(m_pPs, 0, 0);

    
    D3D11_INPUT_ELEMENT_DESC ied[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0}
    };

    m_dev->CreateInputLayout(ied, 2, VS->GetBufferPointer(), VS->GetBufferSize(), &m_pLayout);
    m_devcon->IASetInputLayout(m_pLayout);

}
#ifndef UNICODE
#define UNICODE
#endif

#include <chrono>
#include <thread>

#include <windows.h>
#include <windowsx.h>
#include <iostream>
#include <d3d11.h>

#pragma comment (lib, "d3d11.lib")

IDXGISwapChain* m_swapchain;
ID3D11Device* m_dev;
ID3D11DeviceContext* m_devcon;

int mDefaultWidth = 640;
int mDefaultHeight = 480;

const wchar_t CLASS_NAME[] = L"SAMPLECLASSNAME";

HINSTANCE m_hInstance;

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void InitD3D(HWND hwnd);
void RenderFrame(FLOAT color[4]);
void CleanD3D(void);

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

    RECT wr = { 0, 0, mDefaultWidth, mDefaultHeight };
    AdjustWindowRect(&wr, WS_OVERLAPPEDWINDOW, FALSE);

    HWND hwnd = CreateWindowEx(
        0,//WS_EX_NOACTIVATE | WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TRANSPARENT,                              // Optional window styles.
        CLASS_NAME,                     // Window class
        L"Sample Window",                 // Window text
        WS_OVERLAPPEDWINDOW,//WS_POPUP            // Window style

        // Size and position
        1920 / 2 - (mDefaultWidth / 2),
        1080 / 2 - (mDefaultHeight / 2),
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
    
    FLOAT color[] = { 0.0f, 0.2f, 0.4f, 1.0f };
    bool bFadeDir = TRUE;

    while (TRUE) {
        bGotMsg = (PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE) != 0);

        if (bGotMsg) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            
            if (WM_QUIT == msg.message) {
                break;
            }

            //std::cout << color[2] << std::endl;
           
            
        }
        else {
            std::chrono::milliseconds timespan(10);
            std::this_thread::sleep_for(timespan);

            if (bFadeDir) {
                color[2] -= 0.1f;
            }
            else {
                color[2] += 0.1f;
            }

            if (color[2] >= 1.0f || color[2] <= 0.0f) {
                bFadeDir = !bFadeDir;
            }

            RenderFrame(color);
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

ID3D11RenderTargetView* m_backbuffer;

void InitD3D(HWND hwnd) {
    DXGI_SWAP_CHAIN_DESC scd;

    ZeroMemory(&scd, sizeof(DXGI_SWAP_CHAIN_DESC));

    scd.BufferCount = 1;
    scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    scd.OutputWindow = hwnd;
    scd.SampleDesc.Count = 4;
    scd.Windowed = TRUE;

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

    m_dev->CreateRenderTargetView(pBackBuffer, NULL, &m_backbuffer);
    pBackBuffer->Release();

    m_devcon->OMSetRenderTargets(1, &m_backbuffer, NULL);

    D3D11_VIEWPORT viewport;
    ZeroMemory(&viewport, sizeof(D3D11_VIEWPORT));

    viewport.TopLeftX = 0;
    viewport.TopLeftY = 0;
    viewport.Width = 640;
    viewport.Height = 480;
    
    m_devcon->RSSetViewports(1, &viewport);
}

void RenderFrame(FLOAT color[4]) {
    m_devcon->ClearRenderTargetView(m_backbuffer, color);

    m_swapchain->Present(0, 0);
}

void CleanD3D() {
    m_swapchain->Release();
    m_backbuffer->Release();
    m_dev->Release();
    m_devcon->Release();
}


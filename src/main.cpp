#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include "UI/UI.h"
#include "resource.h"
#include "Area/AreaFile.h"

#include <d3d11.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <wrl/client.h>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>
#include <windowsx.h>

#include "stb_image.h"

using Microsoft::WRL::ComPtr;

// Global D3D11 objects
ID3D11Device* gDevice = nullptr;
ID3D11DeviceContext* gContext = nullptr;
HWND gHwnd = nullptr;

static ComPtr<IDXGISwapChain> gSwapChain;
static ComPtr<ID3D11RenderTargetView> gMainRenderTargetView;
static ComPtr<ID3D11DepthStencilView> gDepthStencilView;
static ComPtr<ID3D11Texture2D> gDepthStencilBuffer;

bool gCharacterIconLoaded = false;
ID3D11ShaderResourceView* gCharacterIconTexture = nullptr;

// Forward declarations
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

static bool CreateDeviceD3D(HWND hWnd);
static void CleanupDeviceD3D();
static void CreateRenderTarget();
static void CleanupRenderTarget();

static AppState* gAppStatePtr = nullptr;

static bool GetResourceData(int resourceId, const void** outData, DWORD* outSize)
{
    HRSRC hRes = FindResource(nullptr, MAKEINTRESOURCE(resourceId), RT_RCDATA);
    if (!hRes) return false;
    HGLOBAL hMem = LoadResource(nullptr, hRes);
    if (!hMem) return false;
    *outSize = SizeofResource(nullptr, hRes);
    *outData = LockResource(hMem);
    return *outData != nullptr;
}

static bool LoadCharacterIcon()
{
    if (!gDevice) return false;

    int w, h;
    unsigned char* imageData = nullptr;

    const void* data = nullptr;
    DWORD dataSize = 0;
    if (GetResourceData(IDR_ICON_CHARACTER, &data, &dataSize))
    {
        imageData = stbi_load_from_memory(
            static_cast<const unsigned char*>(data),
            static_cast<int>(dataSize),
            &w, &h, nullptr, 4);
    }

    if (!imageData)
        return false;

    // Invert colors
    for (int i = 0; i < w * h * 4; i += 4)
    {
        imageData[i + 0] = 255 - imageData[i + 0];
        imageData[i + 1] = 255 - imageData[i + 1];
        imageData[i + 2] = 255 - imageData[i + 2];
    }

    D3D11_TEXTURE2D_DESC texDesc = {};
    texDesc.Width = w;
    texDesc.Height = h;
    texDesc.MipLevels = 1;
    texDesc.ArraySize = 1;
    texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    texDesc.SampleDesc.Count = 1;
    texDesc.Usage = D3D11_USAGE_IMMUTABLE;
    texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

    D3D11_SUBRESOURCE_DATA initData = {};
    initData.pSysMem = imageData;
    initData.SysMemPitch = w * 4;

    ID3D11Texture2D* texture = nullptr;
    HRESULT hr = gDevice->CreateTexture2D(&texDesc, &initData, &texture);
    stbi_image_free(imageData);

    if (FAILED(hr) || !texture)
        return false;

    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MostDetailedMip = 0;
    srvDesc.Texture2D.MipLevels = 1;

    hr = gDevice->CreateShaderResourceView(texture, &srvDesc, &gCharacterIconTexture);
    texture->Release();

    return SUCCEEDED(hr);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.lpszClassName = L"HamSandwichClass";
    RegisterClassExW(&wc);

    gHwnd = CreateWindowW(
        wc.lpszClassName,
        L"The Ham Sandwich",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        1280, 720,
        nullptr, nullptr, hInstance, nullptr);

    if (!CreateDeviceD3D(gHwnd))
    {
        CleanupDeviceD3D();
        UnregisterClassW(wc.lpszClassName, hInstance);
        return 1;
    }

    ShowWindow(gHwnd, nCmdShow);
    UpdateWindow(gHwnd);

    gCharacterIconLoaded = LoadCharacterIcon();

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    ImGui_ImplWin32_Init(gHwnd);
    ImGui_ImplDX11_Init(gDevice, gContext);

    AppState appState;
    gAppStatePtr = &appState;
    InitUI(appState);
    InitGrid(appState);

    ImVec4 clear_color = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);

    bool running = true;
    while (running)
    {
        MSG msg;
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            if (msg.message == WM_QUIT)
                running = false;
        }

        if (!running)
            break;

        UpdateCamera(gHwnd, appState);

        RECT rect;
        GetClientRect(gHwnd, &rect);
        int display_w = rect.right - rect.left;
        int display_h = rect.bottom - rect.top;

        // Set viewport
        D3D11_VIEWPORT vp = {};
        vp.Width = static_cast<float>(display_w);
        vp.Height = static_cast<float>(display_h);
        vp.MinDepth = 0.0f;
        vp.MaxDepth = 1.0f;
        vp.TopLeftX = 0;
        vp.TopLeftY = 0;
        gContext->RSSetViewports(1, &vp);

        // Clear render target and depth buffer
        const float clearColor[4] = { clear_color.x, clear_color.y, clear_color.z, clear_color.w };
        gContext->ClearRenderTargetView(gMainRenderTargetView.Get(), clearColor);
        gContext->ClearDepthStencilView(gDepthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

        // Set render targets
        ID3D11RenderTargetView* rtvs[] = { gMainRenderTargetView.Get() };
        gContext->OMSetRenderTargets(1, rtvs, gDepthStencilView.Get());

        // Render 3D content
        RenderAreas(appState, display_w, display_h);

        // Start ImGui frame
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        RenderUI(appState);

        ImGui::Render();
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        gSwapChain->Present(1, 0);
    }

    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    if (gCharacterIconTexture)
    {
        gCharacterIconTexture->Release();
        gCharacterIconTexture = nullptr;
    }

    CleanupDeviceD3D();
    DestroyWindow(gHwnd);
    UnregisterClassW(wc.lpszClassName, hInstance);

    return 0;
}

bool CreateDeviceD3D(HWND hWnd)
{
    RECT rect;
    GetClientRect(hWnd, &rect);
    UINT width = rect.right - rect.left;
    UINT height = rect.bottom - rect.top;

    DXGI_SWAP_CHAIN_DESC sd = {};
    sd.BufferCount = 2;
    sd.BufferDesc.Width = width;
    sd.BufferDesc.Height = height;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    UINT createDeviceFlags = 0;
#ifdef _DEBUG
    createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0 };

    ID3D11Device* device = nullptr;
    ID3D11DeviceContext* context = nullptr;
    IDXGISwapChain* swapChain = nullptr;

    HRESULT hr = D3D11CreateDeviceAndSwapChain(
        nullptr,
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr,
        createDeviceFlags,
        featureLevelArray,
        2,
        D3D11_SDK_VERSION,
        &sd,
        &swapChain,
        &device,
        &featureLevel,
        &context);

    if (FAILED(hr))
        return false;

    gDevice = device;
    gContext = context;
    gSwapChain.Attach(swapChain);

    CreateRenderTarget();
    return true;
}

void CleanupDeviceD3D()
{
    CleanupRenderTarget();
    gSwapChain.Reset();
    if (gContext) { gContext->Release(); gContext = nullptr; }
    if (gDevice) { gDevice->Release(); gDevice = nullptr; }
}

void CreateRenderTarget()
{
    ID3D11Texture2D* pBackBuffer = nullptr;
    gSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    if (pBackBuffer)
    {
        gDevice->CreateRenderTargetView(pBackBuffer, nullptr, &gMainRenderTargetView);

        D3D11_TEXTURE2D_DESC backBufferDesc;
        pBackBuffer->GetDesc(&backBufferDesc);
        pBackBuffer->Release();

        // Create depth stencil buffer
        D3D11_TEXTURE2D_DESC depthDesc = {};
        depthDesc.Width = backBufferDesc.Width;
        depthDesc.Height = backBufferDesc.Height;
        depthDesc.MipLevels = 1;
        depthDesc.ArraySize = 1;
        depthDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
        depthDesc.SampleDesc.Count = 1;
        depthDesc.SampleDesc.Quality = 0;
        depthDesc.Usage = D3D11_USAGE_DEFAULT;
        depthDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;

        gDevice->CreateTexture2D(&depthDesc, nullptr, &gDepthStencilBuffer);

        D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
        dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
        dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
        dsvDesc.Texture2D.MipSlice = 0;

        gDevice->CreateDepthStencilView(gDepthStencilBuffer.Get(), &dsvDesc, &gDepthStencilView);
    }
}

void CleanupRenderTarget()
{
    gMainRenderTargetView.Reset();
    gDepthStencilView.Reset();
    gDepthStencilBuffer.Reset();
}

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg)
    {
    case WM_SIZE:
        if (gDevice && wParam != SIZE_MINIMIZED)
        {
            CleanupRenderTarget();
            gSwapChain->ResizeBuffers(0, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam), DXGI_FORMAT_UNKNOWN, 0);
            CreateRenderTarget();
        }
        return 0;

    case WM_MOUSEWHEEL:
        if (gAppStatePtr)
        {
            short delta = GET_WHEEL_DELTA_WPARAM(wParam);
            scroll_callback(hWnd, delta, gAppStatePtr);
        }
        return 0;

    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU)
            return 0;
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProc(hWnd, msg, wParam, lParam);
}
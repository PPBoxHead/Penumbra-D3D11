#include "RenderDeviceD3D11.hpp"

#include "../Utils/ConsoleLogger.hpp"

#include <vector>


// This line indicates to hybrid graphics system to prefer the discrete part by default
extern "C" {
    _declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
    _declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}

using namespace Microsoft::WRL;

RenderDeviceD3D11::RenderDeviceD3D11(int t_window_width, int t_window_height, HWND t_hwnd) {
    m_windowWidth = t_window_width;
    m_windowHeight = t_window_height;

    m_hwnd = t_hwnd;

    InitializeD3D11();
}

RenderDeviceD3D11::~RenderDeviceD3D11() {
    if (m_deviceContext) {
        m_deviceContext->ClearState(); // Break bindings to avoid reference cycles
    }

    ComPtr<ID3D11Debug> debugDevice;
    if (SUCCEEDED(m_device->QueryInterface(IID_PPV_ARGS(&debugDevice)))) {
        debugDevice->ReportLiveDeviceObjects(D3D11_RLDO_DETAIL);
    }
}

// Frame lifecycle
void RenderDeviceD3D11::StartFrame(const std::array<float, 4>& clearColor) {
    // Bind the render target view and depth/stencil view
    m_deviceContext->OMSetRenderTargets(1, m_renderTargetView.GetAddressOf(), m_depthStencilView.Get());

    // Clear the Render Target View
    m_deviceContext->ClearRenderTargetView(m_renderTargetView.Get(), clearColor.data());
    // Clear the Depth-Stencil View
    m_deviceContext->ClearDepthStencilView(m_depthStencilView.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);

    // Start GPU frame timing
    m_deviceContext->Begin(m_disjointQuery.Get());
    m_deviceContext->End(m_startQuery.Get());
}
void RenderDeviceD3D11::PresentFrame() {
    // End GPU frame timing
    m_deviceContext->End(m_endQuery.Get());
    m_deviceContext->End(m_disjointQuery.Get());

    // Retrieve and calculate GPU frame time
    D3D11_QUERY_DATA_TIMESTAMP_DISJOINT disjointData;
    while (m_deviceContext->GetData(m_disjointQuery.Get(), &disjointData, sizeof(disjointData), 0) == S_FALSE);

    if (!disjointData.Disjoint) {
        UINT64 startTime = 0, endTime = 0;
        while (m_deviceContext->GetData(m_startQuery.Get(), &startTime, sizeof(startTime), 0) == S_FALSE);
        while (m_deviceContext->GetData(m_endQuery.Get(), &endTime, sizeof(endTime), 0) == S_FALSE);

        gpuFrameTime = static_cast<float>(endTime - startTime) / disjointData.Frequency * 1000.0f;
    }
    else {
        gpuFrameTime = -1.0f; // Indicate invalid GPU timing
    }

    // Present the frame
    UINT syncInterval = m_vsyncEnabled ? 1 : 0; // 1 for VSync, 0 for no VSync
    UINT presentFlags = m_vsyncEnabled ? 0 : DXGI_PRESENT_ALLOW_TEARING;

    HRESULT result = m_swapChain->Present(syncInterval, presentFlags);
    if (FAILED(result)) {
        LogHRESULTError(result, "Failed to present the swap chain.");
        return;
    }
}

void RenderDeviceD3D11::Resize(int newWidth, int newHeight) {
    if (newWidth <= 0 || newHeight <= 0) return;

    // Update window dimensions
    m_windowWidth = newWidth;
    m_windowHeight = newHeight;

    // Release current resources
    m_renderTargetView.Reset();
    m_depthStencilView.Reset();
    m_depthStencilBuffer.Reset();
    m_backBuffer.Reset();

    UINT flags = (m_tearingSupported && !m_vsyncEnabled) ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;

    // Resize the swap chain
    HRESULT result = m_swapChain->ResizeBuffers(
        0, // Preserve buffer count
        m_windowWidth,
        m_windowHeight,
        DXGI_FORMAT_UNKNOWN,
        flags
    );
    if (FAILED(result)) {
        LogHRESULTError(result, "Failed to resize swap chain.");
        return;
    }

    // Recreate resources
    CreateRenderTargetView();
    CreateRenderPipeline();
    SetupViewport();
}

void RenderDeviceD3D11::QueryVRAMInfo() {
    HRESULT result = m_adapter->QueryVideoMemoryInfo(0, DXGI_MEMORY_SEGMENT_GROUP_LOCAL, &m_videoMemoryInfo);
    if (FAILED(result)) {
        LogHRESULTError(result, "Failed to query video memory information.");
        return;
    }
}

ID3D11Device* RenderDeviceD3D11::GetDevice() {
    return m_device.Get();
}
ID3D11DeviceContext* RenderDeviceD3D11::GetDeviceContext() {
    return m_deviceContext.Get();
}

IDXGISwapChain* RenderDeviceD3D11::GetSwapchain() {
    return m_swapChain.Get();
}

ID3D11RenderTargetView* RenderDeviceD3D11::GetRenderTargetView() {
    return m_renderTargetView.Get();
}
ID3D11DepthStencilView* RenderDeviceD3D11::GetDepthStencilView() {
    return m_depthStencilView.Get();
}

DXGI_QUERY_VIDEO_MEMORY_INFO RenderDeviceD3D11::GetVideoMemoryInfo() {
    return m_videoMemoryInfo;
}

char* RenderDeviceD3D11::GetVideoCardDescription() {
    return m_videoCardDescription;
}

int RenderDeviceD3D11::GetWindowWidth() {
    return m_windowWidth;
}
int RenderDeviceD3D11::GetWindowHeight() {
    return m_windowHeight;
}

bool RenderDeviceD3D11::IsVSyncEnabled() {
    return m_vsyncEnabled;
}
void RenderDeviceD3D11::SetVSyncEnabled(bool enabled) {
    // Update the internal VSync state
    m_vsyncEnabled = enabled;

    // Release current resources
    m_swapChain.Reset();
    m_renderTargetView.Reset();
    m_depthStencilView.Reset();
    m_depthStencilBuffer.Reset();
    m_backBuffer.Reset();

    // Resize the swap chain
    HRESULT result = CreateSwapChain();
    CreateRenderTargetView();
    CreateRenderPipeline();
    SetupViewport();

    if (FAILED(result)) {
        LogHRESULTError(result, "Failed to resize swap chain during VSync toggle.");
        return;
    }
}

// D3D11 Initialization pipeline
void RenderDeviceD3D11::InitializeD3D11() {
    try {
        CreateFactory();
        CheckTearingSupport();
        SetupHardwareAdapter();
        InitializeDeviceAndContext();
        CreateSwapChain();
        CreateRenderTargetView();
        CreateRenderPipeline();
        SetupViewport();
        InitializeGPUQuery();
        CONSOLE_LOG_INFO("DirectX 11 initialization complete.");
    }
    catch (const std::exception& ex) {
        CONSOLE_LOG_ERROR("DirectX 11 initialization failed: ", ex.what());
        throw;
    }

}
// Creates the DXGI Factory
void RenderDeviceD3D11::CreateFactory() {
    HRESULT result = CreateDXGIFactory1(__uuidof(IDXGIFactory5), (void**)&m_factory);
    if (FAILED(result))
        LogHRESULTError(result, "Failed to create DXGI Factory: ");
}
// Check if OS has Tearing Support for Vsync
void RenderDeviceD3D11::CheckTearingSupport() {
    ComPtr<IDXGIFactory5> factory5;
    HRESULT result = m_factory->QueryInterface(IID_PPV_ARGS(&factory5));
    if (FAILED(result)) {
        CONSOLE_LOG_WARNING("DXGI Factory does not support CheckFeatureSupport.");
        m_tearingSupported = false;
        return;
    }

    BOOL allowTearing = FALSE;
    result = factory5->CheckFeatureSupport(
        DXGI_FEATURE_PRESENT_ALLOW_TEARING,
        &allowTearing,
        sizeof(allowTearing)
    );

    m_tearingSupported = SUCCEEDED(result) && allowTearing;
}
// Enumerate the hardware adapter
void RenderDeviceD3D11::SetupHardwareAdapter() {
    // Enumerate the hardware adapter
    HRESULT result = m_factory->EnumAdapters(0, reinterpret_cast<IDXGIAdapter**>(m_adapter.GetAddressOf()));
    if (FAILED(result)) {
        LogHRESULTError(result, "Failed to enumerate adapters.");
        return;
    }

    // Enumerate the primary adapter output (monitor)
    result = m_adapter->EnumOutputs(0, &m_adapterOutput);
    if (FAILED(result)) {
        LogHRESULTError(result, "Failed to enumerate the adapter output.");
        return;
    }

    // Query the number of supported display modes
    UINT numModes = 0;
    result = m_adapterOutput->GetDisplayModeList(
        DXGI_FORMAT_R8G8B8A8_UNORM,
        DXGI_ENUM_MODES_INTERLACED,
        &numModes,
        nullptr
    );
    if (FAILED(result)) {
        LogHRESULTError(result, "Failed to retrieve display mode list count.");
        return;
    }

    // Allocate space for display modes and retrieve them
    std::vector<DXGI_MODE_DESC> displayModeList(numModes);
    result = m_adapterOutput->GetDisplayModeList(
        DXGI_FORMAT_R8G8B8A8_UNORM,
        DXGI_ENUM_MODES_INTERLACED,
        &numModes,
        displayModeList.data()
    );
    if (FAILED(result)) {
        LogHRESULTError(result, "Failed to retrieve display mode list.");
        return;
    }

    // Select a refresh rate matching the target resolution
    m_numerator = 0;
    m_denominator = 1; // Defaults to uncapped
    for (const auto& mode : displayModeList) {
        if (mode.Width == static_cast<UINT>(m_windowWidth) &&
            mode.Height == static_cast<UINT>(m_windowHeight)) {
            m_numerator = mode.RefreshRate.Numerator;
            m_denominator = mode.RefreshRate.Denominator;
            break;
        }
    }

    // Log a warning if no matching mode was found
    if (m_numerator == 0) {
        CONSOLE_LOG_WARNING("No matching display mode found. Using uncapped refresh rate.");
    }

    // Retrieve adapter description for debugging purposes
    DXGI_ADAPTER_DESC adapterDesc;
    result = m_adapter->GetDesc(&adapterDesc);
    if (FAILED(result)) {
        LogHRESULTError(result, "Failed to retrieve adapter description.");
        return;
    }

    // Store video card information
    videoCardDedicatedMemory = static_cast<int>(adapterDesc.DedicatedVideoMemory / 1024 / 1024);
    videoCardSharedSystemMemory = static_cast<int>(adapterDesc.SharedSystemMemory / 1024 / 1024);
    size_t stringLength = 0;
    if (wcstombs_s(&stringLength, m_videoCardDescription, 128, adapterDesc.Description, 128) != 0) {
        CONSOLE_LOG_ERROR("Failed to convert video card description.");
    }

    // Debug logging for adapter information
#if defined(_DEBUG)
    CONSOLE_LOG_INFO("Video Card dedicated memory (VRAM): ",
        videoCardDedicatedMemory, "MB");
    CONSOLE_LOG_INFO("Video Card shared system memory (RAM): ",
        videoCardSharedSystemMemory, "MB");
    CONSOLE_LOG_INFO("Video Card description: ", m_videoCardDescription);
#endif

}
// Create device and device context
void RenderDeviceD3D11::InitializeDeviceAndContext() {
    UINT deviceFlags = 0;
#if defined(_DEBUG)
    deviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    D3D_FEATURE_LEVEL featureLevels[4] = {
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0
    };
    D3D_FEATURE_LEVEL t_featureLevel = D3D_FEATURE_LEVEL_11_1;

    HRESULT result = D3D11CreateDevice(
        // Use the specific adapter
        m_adapter.Get(),
        // This must be UNKNOWN when specifying an adapter
        D3D_DRIVER_TYPE_UNKNOWN,
        // No software rasterizer
        nullptr,
        // Creation flags
        deviceFlags | D3D11_CREATE_DEVICE_SINGLETHREADED,
        // Array of feature levels
        featureLevels,
        // Number of feature levels, another option could be _countof(featureLevels)
        4u,
        // SDK version
        D3D11_SDK_VERSION,
        // Output device
        m_device.GetAddressOf(),
        // Output feature level
        &t_featureLevel,
        // Output device context
        m_deviceContext.GetAddressOf()
    );
    if (FAILED(result))
        LogHRESULTError(result, "Failed to create Direct3D device: ");
}
// Create the swapchain desc and setup the swapchain
HRESULT RenderDeviceD3D11::CreateSwapChain() {
    DXGI_SWAP_CHAIN_DESC swapChainDesc = {};

    // Double buffering
    swapChainDesc.BufferCount = 2;

    swapChainDesc.BufferDesc.Width = m_windowWidth;
    swapChainDesc.BufferDesc.Height = m_windowHeight;
    // RGBA 32-bit
    swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; 

    // Configure tearing support
    if (m_vsyncEnabled) {
        swapChainDesc.BufferDesc.RefreshRate.Numerator = m_numerator;
        swapChainDesc.BufferDesc.RefreshRate.Denominator = m_denominator;
        swapChainDesc.Flags = 0; // No tearing
    }
    else if (m_tearingSupported) {
        swapChainDesc.BufferDesc.RefreshRate.Numerator = 0;
        swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
        swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
    }
    else {
        CONSOLE_LOG_WARNING("Tearing is not supported. Defaulting to VSync.");
        m_vsyncEnabled = true;
        swapChainDesc.BufferDesc.RefreshRate.Numerator = m_numerator;
        swapChainDesc.BufferDesc.RefreshRate.Denominator = m_denominator;
        swapChainDesc.Flags = 0; // No tearing
    }

    // Set the usage of the back buffer.
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    // Set the scan line ordering and scaling to unspecified.
    swapChainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
    swapChainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;

    swapChainDesc.OutputWindow = m_hwnd;

    swapChainDesc.SampleDesc.Count = 1; // No MSAA
    swapChainDesc.SampleDesc.Quality = 0;

    swapChainDesc.Windowed = TRUE;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

    //swapChainDesc.Flags = 0;

    // Use the factory to create the swap chain
    HRESULT result = m_factory->CreateSwapChain(
        m_device.Get(),
        &swapChainDesc,
        m_swapChain.GetAddressOf()
    );

    return result;
}
// Get the back buffer and create the render target view
void RenderDeviceD3D11::CreateRenderTargetView() {
    HRESULT result = m_swapChain->GetBuffer(0, IID_PPV_ARGS(&m_backBuffer));
    if (FAILED(result))
        LogHRESULTError(result, "Failed to get the back buffer: ");
    result = m_device->CreateRenderTargetView(m_backBuffer.Get(), nullptr, &m_renderTargetView);
    if (FAILED(result))
        LogHRESULTError(result, "Failed to create render target view: ");
}
// Creates the stencil desc and setup the Stencil State, but also render desc and the Rasterizer State
void RenderDeviceD3D11::CreateRenderPipeline() {
    // Create Depth-Stencil Buffer
    D3D11_TEXTURE2D_DESC depthStencilBufferDesc = {};
    depthStencilBufferDesc.Width = m_windowWidth;  // Set width and height of the buffer
    depthStencilBufferDesc.Height = m_windowHeight;

    depthStencilBufferDesc.MipLevels = 1;
    depthStencilBufferDesc.ArraySize = 1;
    depthStencilBufferDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT; // 24-bit depth, 8-bit stencil

    depthStencilBufferDesc.SampleDesc.Count = 1; // No MSAA
    depthStencilBufferDesc.SampleDesc.Quality = 0;

    depthStencilBufferDesc.Usage = D3D11_USAGE_DEFAULT;
    depthStencilBufferDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
    depthStencilBufferDesc.CPUAccessFlags = 0;
    depthStencilBufferDesc.MiscFlags = 0;

    HRESULT result = m_device->CreateTexture2D(&depthStencilBufferDesc, nullptr, &m_depthStencilBuffer);
    if (FAILED(result)) {
        LogHRESULTError(result, "Failed to create Depth-Stencil Buffer: ");
    }

    // Create Depth-Stencil View
    D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc = {};
    depthStencilViewDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    depthStencilViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
    depthStencilViewDesc.Texture2D.MipSlice = 0;

    result = m_device->CreateDepthStencilView(m_depthStencilBuffer.Get(), &depthStencilViewDesc, &m_depthStencilView);
    if (FAILED(result)) {
        LogHRESULTError(result, "Failed to create Depth-Stencil View: ");
    }

    // Create Depth-Stencil State
    D3D11_DEPTH_STENCIL_DESC depthStencilDesc = {};

    // Set up the description of the stencil state.
    depthStencilDesc.DepthEnable = true;
    depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    depthStencilDesc.DepthFunc = D3D11_COMPARISON_LESS;

    depthStencilDesc.StencilEnable = true;
    depthStencilDesc.StencilReadMask = 0xFF;
    depthStencilDesc.StencilWriteMask = 0xFF;

    // Stencil operations if pixel is front-facing.
    depthStencilDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
    depthStencilDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_INCR;
    depthStencilDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
    depthStencilDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

    // Stencil operations if pixel is back-facing.
    depthStencilDesc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
    depthStencilDesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_DECR;
    depthStencilDesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
    depthStencilDesc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

    // Create the Depth-Stencil State.
    result = m_device->CreateDepthStencilState(&depthStencilDesc, &m_depthStencilState);
    if (FAILED(result))
    {
        LogHRESULTError(result, "Failed to create depth stencil state: ");
    }

    // Bind the Depth-Stencil State.
    m_deviceContext->OMSetDepthStencilState(m_depthStencilState.Get(), 1);

    // Setup Rasterizer State.
    D3D11_RASTERIZER_DESC rasterizerDesc = {};
    rasterizerDesc.AntialiasedLineEnable = false;
    rasterizerDesc.CullMode = D3D11_CULL_BACK;
    rasterizerDesc.DepthBias = 0;
    rasterizerDesc.DepthBiasClamp = 0.0f;
    rasterizerDesc.DepthClipEnable = true;
    rasterizerDesc.FillMode = D3D11_FILL_SOLID;
    rasterizerDesc.FrontCounterClockwise = false;
    rasterizerDesc.MultisampleEnable = false;
    rasterizerDesc.ScissorEnable = false;
    rasterizerDesc.SlopeScaledDepthBias = 0.0f;

    result = m_device->CreateRasterizerState(&rasterizerDesc, &m_rasterizerState);
    if (FAILED(result))
        LogHRESULTError(result, "Failed to create Rasterize State: ");

    // Now bind the Rasterizer State.
    m_deviceContext->RSSetState(m_rasterizerState.Get());

    // Bind Render Target and Depth-Stencil Views
    if (!m_renderTargetView || !m_depthStencilView) {
        CONSOLE_LOG_ERROR("Render target or depth stencil view is uninitialized.");
        return;
    }
    m_deviceContext->OMSetRenderTargets(1, m_renderTargetView.GetAddressOf(), m_depthStencilView.Get());
}
// Configure the viewport
void RenderDeviceD3D11::SetupViewport() {
    m_viewport.TopLeftX = 0.0f;
    m_viewport.TopLeftY = 0.0f;
    m_viewport.Width = static_cast<FLOAT>(m_windowWidth);
    m_viewport.Height = static_cast<FLOAT>(m_windowHeight);
    m_viewport.MinDepth = 0.0f;
    m_viewport.MaxDepth = 1.0f;

    m_deviceContext->RSSetViewports(1, &m_viewport);
}
void RenderDeviceD3D11::InitializeGPUQuery() {
    D3D11_QUERY_DESC queryDesc = {};
    queryDesc.Query = D3D11_QUERY_TIMESTAMP_DISJOINT;
    m_device->CreateQuery(&queryDesc, &m_disjointQuery);

    queryDesc.Query = D3D11_QUERY_TIMESTAMP;
    m_device->CreateQuery(&queryDesc, &m_startQuery);
    m_device->CreateQuery(&queryDesc, &m_endQuery);
}

void RenderDeviceD3D11::LogHRESULTError(HRESULT hr, const char* message) {
	char buffer[512];
	FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		nullptr, hr, 0, buffer, sizeof(buffer), nullptr);
    CONSOLE_LOG_CRITICAL_ERROR(message, buffer);

    exit(EXIT_FAILURE);
}

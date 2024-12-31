#include "RenderDeviceD3D11.h"

#include "../Utils/ConsoleLogger.h"


// This line indicates to hybrid graphics system to prefer the discrete part by default
extern "C" {
    _declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
    _declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}


RenderDeviceD3D11::RenderDeviceD3D11(int t_window_width, int t_window_height, HWND t_hwnd) {
    m_windowWidth = t_window_width;
    m_windowHeight = t_window_height;

    InitD3D11(t_hwnd);
}

RenderDeviceD3D11::~RenderDeviceD3D11() {
    if (m_deviceContext) {
        m_deviceContext->ClearState(); // Break bindings to avoid reference cycles
    }

    Microsoft::WRL::ComPtr<ID3D11Debug> debugDevice;
    if (SUCCEEDED(m_device->QueryInterface(IID_PPV_ARGS(&debugDevice)))) {
        debugDevice->ReportLiveDeviceObjects(D3D11_RLDO_DETAIL);
    }
}

ID3D11Device* RenderDeviceD3D11::GetDevice() {
    return m_device.Get();
}

ID3D11DeviceContext* RenderDeviceD3D11::GetDeviceContext() {
    return m_deviceContext.Get();
}

// This function calls all the next steps declarated in this header
void RenderDeviceD3D11::InitD3D11(HWND t_hwnd) {
    CreateFactory();
    SetupHardwareAdapter();
    InitializeDeviceAndContext();
    CreateSwapChain(t_hwnd);
    CreateRenderTargetView();
    CreateRenderPipeline();
    SetupViewport();

    ConsoleLogger::Print(ConsoleLogger::LogType::C_INFO, "DirectX 11 initialization complete.");
}

// Creates the DXGI Factory
void RenderDeviceD3D11::CreateFactory() {
    HRESULT result = CreateDXGIFactory(IID_PPV_ARGS(&m_factory));
    if (FAILED(result))
        LogHRESULTError(result, "Failed to create DXGI Factory: ");
}
// Enumerate the hardware adapter
void RenderDeviceD3D11::SetupHardwareAdapter() {
    HRESULT result = m_factory->EnumAdapters(0, &m_adapter);
    if (FAILED(result))
        LogHRESULTError(result, "Failed to enumerate adapters: ");
    // Enumerate the primary adapter output (monitor).
    result = m_adapter->EnumOutputs(0, &m_adapterOutput);

    // Get the number of modes that fit the DXGI_FORMAT_R8G8B8A8_UNORM display
    // format for the adapter output (monitor).
    unsigned int numModes = 0;
    DXGI_MODE_DESC* displayModeList;
    result = m_adapterOutput->GetDisplayModeList(
        DXGI_FORMAT_R16G16B16A16_FLOAT,
        DXGI_ENUM_MODES_INTERLACED,
        &numModes,
        nullptr
    );
    if (FAILED(result))
        LogHRESULTError(result, "Failed to setup the adapter output: ");

    // Create a list to hold all the possible display modes for this monitor/video
    // card combination.
    displayModeList = new DXGI_MODE_DESC[numModes];
    result = m_adapterOutput->GetDisplayModeList(
        DXGI_FORMAT_R8G8B8A8_UNORM,
        DXGI_ENUM_MODES_INTERLACED,
        &numModes,
        displayModeList
    );
    if (FAILED(result))
        LogHRESULTError(result, "Failed to create a list to hold all the possible display modes: ");

    // Now go through all the display modes and find the one that matches the screen width and height.
    // When a match is found store the numerator and denominator of the refresh rate for that monitor.
    for (unsigned int i = 0; i < numModes; i++) {
        if (displayModeList[i].Width == static_cast<unsigned int>(m_windowWidth)) {
            if (displayModeList[i].Height == static_cast<unsigned int>(m_windowHeight)) {
                m_numerator = displayModeList[i].RefreshRate.Numerator;
                m_denominator = displayModeList[i].RefreshRate.Denominator;
            }
        }
    }

    // Get the adapter (video card) description.
    DXGI_ADAPTER_DESC adapterDesc;
    result = m_adapter->GetDesc(&adapterDesc);
    if (FAILED(result))
    {
        LogHRESULTError(result, "Failed to get the adapter description: ");
    }

    // Store the dedicated video card memory in megabytes,
    // This is mostly for debug information.
    videoCardDedicatedMemory = int(adapterDesc.DedicatedVideoMemory / 1024 / 1024);
    // Store the shared system video card memory in megabytes,
    // This is mostly for debug information.
    videoCardSharedSystemMemory = int(adapterDesc.SharedSystemMemory / 1024 / 1024);
    size_t stringLength;
    // Convert the name of the video card to a character array and store it.
    const int error = wcstombs_s(&stringLength, videoCardDescription, 128, adapterDesc.Description, 128);
    if (error != 0)
    {
        ConsoleLogger::Print(ConsoleLogger::LogType::C_ERROR, "Failed to convert video card description");
    }

#if defined(_DEBUG)
    ConsoleLogger::Print(ConsoleLogger::LogType::C_INFO, "Video Card dedicated memory - VRAM: ", videoCardDedicatedMemory, "MB");
    ConsoleLogger::Print(ConsoleLogger::LogType::C_INFO, "Video Card shared system memory - RAM: ", videoCardSharedSystemMemory, "MB");
    ConsoleLogger::Print(ConsoleLogger::LogType::C_INFO, "Video Card name - Vendor: ", videoCardDescription);
#endif

    // Release the display mode list.
    delete[] displayModeList;
    displayModeList = nullptr;

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
void RenderDeviceD3D11::CreateSwapChain(HWND t_hwnd) {
    DXGI_SWAP_CHAIN_DESC swapChainDesc = {};

    // Double buffering
    swapChainDesc.BufferCount = 2;

    swapChainDesc.BufferDesc.Width = m_windowWidth;
    swapChainDesc.BufferDesc.Height = m_windowHeight;
    // RGBA 32-bit
    swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; 

    if (is_vsync_enabled)
    {
        swapChainDesc.BufferDesc.RefreshRate.Numerator = m_numerator;
        swapChainDesc.BufferDesc.RefreshRate.Denominator = m_denominator;
    }
    else
    {
        swapChainDesc.BufferDesc.RefreshRate.Numerator = 0;
        swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
    }

    // Set the usage of the back buffer.
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    // Set the scan line ordering and scaling to unspecified.
    swapChainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
    swapChainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;

    swapChainDesc.OutputWindow = t_hwnd;

    swapChainDesc.SampleDesc.Count = 1; // No MSAA
    swapChainDesc.SampleDesc.Quality = 0;

    swapChainDesc.Windowed = TRUE;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

    swapChainDesc.Flags = 0;

    // Use the factory to create the swap chain
    HRESULT result = m_factory->CreateSwapChain(
        m_device.Get(),
        &swapChainDesc,
        m_swapChain.GetAddressOf()
    );
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
        ConsoleLogger::Print(ConsoleLogger::LogType::C_ERROR, "Render target or depth stencil view is uninitialized.");
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

// Frame lifecycle
void RenderDeviceD3D11::StartFrame(const std::array<float, 4>& clearColor) {
    // Bind the render target view and depth/stencil view
    m_deviceContext->OMSetRenderTargets(1, m_renderTargetView.GetAddressOf(), m_depthStencilView.Get());

    // Clear the Render Target View
	m_deviceContext->ClearRenderTargetView(m_renderTargetView.Get(), clearColor.data());
    // Clear the Depth-Stencil View
    m_deviceContext->ClearDepthStencilView(m_depthStencilView.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);
}
void RenderDeviceD3D11::PresentFrame() {
        m_swapChain->Present(is_vsync_enabled ? 1 : 0, 0);
}


void RenderDeviceD3D11::LogHRESULTError(HRESULT hr, const char* message) {
	char buffer[512];
	FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		nullptr, hr, 0, buffer, sizeof(buffer), nullptr);
	ConsoleLogger::Print(ConsoleLogger::LogType::C_ERROR, message, buffer);

    exit(EXIT_FAILURE);
}

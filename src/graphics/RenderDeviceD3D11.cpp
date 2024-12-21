#include "RenderDeviceD3D11.hpp"

#include "../utils/ConsoleLogger.hpp"



RenderDeviceD3D11::RenderDeviceD3D11(int t_window_width, int t_window_height) {
	glfwInit();

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	window = glfwCreateWindow(t_window_width, t_window_height, "Penumbra-D3D11 Window :D", nullptr, nullptr);
	if (window == nullptr) {
		ConsoleLogger::consolePrint(ConsoleLogger::LogType::C_ERROR, "Unable to create GLFW window");
		glfwTerminate();
		exit(EXIT_FAILURE);
	}

    InitD3D11();
}

RenderDeviceD3D11::~RenderDeviceD3D11() {
	if (window) {
		glfwDestroyWindow(window);
		window = nullptr;
	}
	glfwTerminate();
}


void RenderDeviceD3D11::InitD3D11() {
    CreateDXGIFactoryInstance();
    InitializeHardwareAdapter();
    CreateDevice();
    CreateSwapChain();
    CreateRenderTargetView();
    CreateStencilState();
    ConfigureViewport();

    ConsoleLogger::consolePrint(ConsoleLogger::LogType::C_INFO, "DirectX 11 initialization complete.");
}

ID3D11Device* RenderDeviceD3D11::GetDevice() {
    return m_device.Get();
}

// Creates the DXGI Factory
void RenderDeviceD3D11::CreateDXGIFactoryInstance() {
    HRESULT result = CreateDXGIFactory(IID_PPV_ARGS(&m_factory));
    if (FAILED(result))
        LogHRESULTError(result, "Failed to create DXGI Factory");
}
// Enumerate the hardware adapter
void RenderDeviceD3D11::InitializeHardwareAdapter() {
    HRESULT result = m_factory->EnumAdapters(0, &m_adapter);
    if (FAILED(result))
        LogHRESULTError(result, "Failed to enumerate adapters");
    // Enumerate the primary adapter output (monitor).
    result = m_adapter->EnumOutputs(0, &m_adapterOutput);

    // Get the number of modes that fit the DXGI_FORMAT_R8G8B8A8_UNORM display
    // format for the adapter output (monitor).
    unsigned int numModes;
    DXGI_MODE_DESC* displayModeList;
    result = m_adapterOutput->GetDisplayModeList(
        DXGI_FORMAT_R16G16B16A16_FLOAT,
        DXGI_ENUM_MODES_INTERLACED,
        &numModes,
        nullptr
    );
    if (FAILED(result))
        LogHRESULTError(result, "Failed to setup the adapter output");

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
        LogHRESULTError(result, "Failed to create a list to hold all the possible display modes");

    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    // Now go through all the display modes and find the one that matches the screen width and height.
    // When a match is found store the numerator and denominator of the refresh rate for that monitor.
    for (unsigned int i = 0; i < numModes; i++)
    {
        if (displayModeList[i].Width == static_cast<unsigned int>(width))
        {
            if (displayModeList[i].Height == static_cast<unsigned int>(height))
            {
                m_numerator = displayModeList[i].RefreshRate.Numerator;
                m_denominator = displayModeList[i].RefreshRate.Denominator;
            }
        }
    }

    DXGI_ADAPTER_DESC adapterDesc;
    size_t stringLength;

    // Get the adapter (video card) description.
    result = m_adapter->GetDesc(&adapterDesc);
    if (FAILED(result))
    {
        LogHRESULTError(result, "Failed to get the adapter description");
    }

    // Store the dedicated video card memory in megabytes.
    m_videoCardMemory = int(adapterDesc.DedicatedVideoMemory / 1024 / 1024);

    // Convert the name of the video card to a character array and store it.
    const int error = wcstombs_s(&stringLength, m_videoCardDescription, 128, adapterDesc.Description, 128);
    if (error != 0)
    {
        ConsoleLogger::consolePrint(ConsoleLogger::LogType::C_ERROR, "Video Card has no name lol: ", error);
    }

#if defined(_DEBUG)
    ConsoleLogger::consolePrint(ConsoleLogger::LogType::C_INFO, "Video Card memory - VRAM: ", m_videoCardMemory, "MB");
    ConsoleLogger::consolePrint(ConsoleLogger::LogType::C_INFO, "Video Card name - Vendor: ", m_videoCardDescription);
#endif

    // Release the display mode list.
    delete[] displayModeList;
    displayModeList = nullptr;
}
// Create device and device context
void RenderDeviceD3D11::CreateDevice() {
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
        LogHRESULTError(result, "Failed to create Direct3D device");
}
// Create the swapchain desc and setup the swapchain
void RenderDeviceD3D11::CreateSwapChain() {
    DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);

    ZeroMemory(&swapChainDesc, sizeof(DXGI_SWAP_CHAIN_DESC));
    // Double buffering
    swapChainDesc.BufferCount = 2;

    swapChainDesc.BufferDesc.Width = width;
    swapChainDesc.BufferDesc.Height = height;
    // RGBA 32-bit
    swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; 

    if (vsync)
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

    swapChainDesc.OutputWindow = glfwGetWin32Window(window);

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
        LogHRESULTError(result, "Failed to get the back buffer");
    result = m_device->CreateRenderTargetView(m_backBuffer.Get(), nullptr, &m_renderTargetView);
    if (FAILED(result))
        LogHRESULTError(result, "Failed to create render target view");
}
// Create the stencil desc and setup the stencil state
void RenderDeviceD3D11::CreateStencilState() {
    D3D11_DEPTH_STENCIL_DESC depthStencilDesc;
    // Initialize the description of the stencil state.
    ZeroMemory(&depthStencilDesc, sizeof depthStencilDesc);

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

    // Create the depth stencil state.
    HRESULT result = m_device->CreateDepthStencilState(&depthStencilDesc, &m_depthStencilState);
    if (FAILED(result))
    {
        LogHRESULTError(result, "Failed to create depth stencil state");
    }

    // Set the depth stencil state.
    m_deviceContext->OMSetDepthStencilState(m_depthStencilState.Get(), 1);

    // Set the render target
    // Bind the render target view and depth stencil buffer to the output render pipeline
    m_deviceContext->OMSetRenderTargets(1, m_renderTargetView.GetAddressOf(), m_depthStencilView.Get());
}
// Configure the viewport
void RenderDeviceD3D11::ConfigureViewport() {
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);

    m_viewport.TopLeftX = 0.0f;
    m_viewport.TopLeftY = 0.0f;
    m_viewport.Width = static_cast<FLOAT>(width);
    m_viewport.Height = static_cast<FLOAT>(height);
    m_viewport.MinDepth = 0.0f;
    m_viewport.MaxDepth = 1.0f;

    m_deviceContext->RSSetViewports(1, &m_viewport);
}

// Frame lifecycle
void RenderDeviceD3D11::beginFrame(const std::array<float, 4>& clearColor) {
	m_deviceContext->ClearRenderTargetView(m_renderTargetView.Get(), clearColor.data());
}
void RenderDeviceD3D11::endFrame() {
	m_swapChain->Present(1, 0);
}


void RenderDeviceD3D11::LogHRESULTError(HRESULT hr, const char* message) {
	char buffer[512];
	FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		nullptr, hr, 0, buffer, sizeof(buffer), nullptr);
	ConsoleLogger::consolePrint(ConsoleLogger::LogType::C_ERROR, message, buffer);
}

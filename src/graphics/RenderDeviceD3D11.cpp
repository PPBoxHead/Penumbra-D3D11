#include "RenderDeviceD3D11.hpp"

#include "../utils/ConsoleLogger.hpp"



RenderDeviceD3D11::RenderDeviceD3D11(int t_window_width, int t_window_height) {
	glfwInit();

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	m_window = glfwCreateWindow(t_window_width, t_window_height, "Penumbra-D3D11 Window :D", nullptr, nullptr);
	if (m_window == nullptr) {
		ConsoleLogger::consolePrint(ConsoleLogger::LogType::C_ERROR, "Unable to create GLFW window");
		glfwTerminate();
		exit(EXIT_FAILURE);
	}

	InitD3D11();
}

RenderDeviceD3D11::~RenderDeviceD3D11() {
	if (m_window) {
		glfwDestroyWindow(m_window);
		m_window = nullptr;
	}
	glfwTerminate();
}


void RenderDeviceD3D11::InitD3D11() {
    // 1. Create the DXGI Factory
    if (FAILED(CreateDXGIFactory(IID_PPV_ARGS(&m_factory))))
        throw std::runtime_error("Failed to create DXGI Factory");

    // 2. Enumerate the hardware adapter
    if (FAILED(m_factory->EnumAdapters(0, &m_adapter)))
        throw std::runtime_error("Failed to enumerate adapters");

    // 3. Define device creation flags (enable debug layer in debug builds)
    UINT deviceFlags = 0;
#if defined(_DEBUG)
    deviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    // 4. Define swap chain description
    DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
    int width, height;
    glfwGetFramebufferSize(m_window, &width, &height);

    swapChainDesc.BufferCount = 1; // Double buffering
    swapChainDesc.BufferDesc.Width = width;
    swapChainDesc.BufferDesc.Height = height;
    swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; // RGBA 32-bit
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;  // Render target
    swapChainDesc.OutputWindow = glfwGetWin32Window(m_window);
    swapChainDesc.SampleDesc.Count = 1; // No MSAA
    swapChainDesc.SampleDesc.Quality = 0;
    swapChainDesc.Windowed = TRUE;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    // These are the feature levels that we will accept.
    D3D_FEATURE_LEVEL featureLevels[] =
    {
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0,
    };

    // 5. Create device, device context, and swap chain
    D3D_FEATURE_LEVEL featureLevel;
    if (FAILED(D3D11CreateDeviceAndSwapChain(
        m_adapter.Get(),                   // Specific adapter
        D3D_DRIVER_TYPE_UNKNOWN,           // Hardware driver type (WRONG)
        nullptr,                           // Software rasterizer (not used)
        deviceFlags,                       // Flags
        featureLevels,                     // Feature levels
        _countof(featureLevels),           // Number of feature levels
        D3D11_SDK_VERSION,                 // SDK version
        &swapChainDesc,                    // Swap chain description
        m_swapChain.GetAddressOf(),        // Swap chain
        m_device.GetAddressOf(),           // Device
        nullptr,                           // Feature level (optional)
        m_deviceContext.GetAddressOf())))  // Device context
    {
        throw std::runtime_error("Failed to create Direct3D device and swap chain");
    }

    // 6. Get the back buffer and create the render target view
    if (FAILED(m_swapChain->GetBuffer(0, IID_PPV_ARGS(&m_backBuffer))))
        throw std::runtime_error("Failed to get the back buffer");

    if (FAILED(m_device->CreateRenderTargetView(m_backBuffer.Get(), nullptr, &m_renderTargetView)))
        throw std::runtime_error("Failed to create render target view");

    // 7. Set the render target
    m_deviceContext->OMSetRenderTargets(1, m_renderTargetView.GetAddressOf(), nullptr);

    // 8. Configure the viewport
    m_viewport.TopLeftX = 0.0f;
    m_viewport.TopLeftY = 0.0f;
    m_viewport.Width = static_cast<FLOAT>(width);
    m_viewport.Height = static_cast<FLOAT>(height);
    m_viewport.MinDepth = 0.0f;
    m_viewport.MaxDepth = 1.0f;

    m_deviceContext->RSSetViewports(1, &m_viewport);

    std::cout << "DirectX 11 initialization complete." << std::endl;
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

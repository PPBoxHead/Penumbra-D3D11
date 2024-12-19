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
    CreateDXGIFactoryInstance();
    EnumerateHardwareAdapter();
    CreateDevice();
    CreateSwapChain();
    CreateRenderTargetView();
    ConfigureViewport();

    ConsoleLogger::consolePrint(ConsoleLogger::LogType::C_INFO, "DirectX 11 initialization complete.");
}

// Creates the DXGI Factory
void RenderDeviceD3D11::CreateDXGIFactoryInstance() {
    HRESULT result = CreateDXGIFactory(IID_PPV_ARGS(&m_factory));
    if (FAILED(result))
        LogHRESULTError(result, "Failed to create DXGI Factory");
}
// Enumerate the hardware adapter
void RenderDeviceD3D11::EnumerateHardwareAdapter() {
    HRESULT result = m_factory->EnumAdapters(0, &m_adapter);
    if (FAILED(result))
        LogHRESULTError(result, "Failed to enumerate adapters");
}
// Create device and device context
void RenderDeviceD3D11::CreateDevice() {
    UINT deviceFlags = 0;
#if defined(_DEBUG)
    deviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    D3D_FEATURE_LEVEL featureLevels[] = {
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0,
    };
    D3D_FEATURE_LEVEL t_featureLevel;

    HRESULT result = D3D11CreateDevice(
        m_adapter.Get(),              // Use the specific adapter
        D3D_DRIVER_TYPE_UNKNOWN,      // Must be UNKNOWN when specifying an adapter
        nullptr,                      // No software rasterizer
        deviceFlags,                  // Creation flags
        featureLevels,                // Array of feature levels
        _countof(featureLevels),      // Number of feature levels
        D3D11_SDK_VERSION,            // SDK version
        m_device.GetAddressOf(),      // Output device
        &t_featureLevel,              // Output feature level
        m_deviceContext.GetAddressOf()// Output device context
    );
    if (FAILED(result))
        LogHRESULTError(result, "Failed to create Direct3D device");
}
// Create the swapchain desc and setup the swapchain
void RenderDeviceD3D11::CreateSwapChain() {
    DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
    int width, height;
    glfwGetFramebufferSize(m_window, &width, &height);

    swapChainDesc.BufferCount = 2; // Double buffering
    swapChainDesc.BufferDesc.Width = width;
    swapChainDesc.BufferDesc.Height = height;
    swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; // RGBA 32-bit
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;  // Render target
    swapChainDesc.OutputWindow = glfwGetWin32Window(m_window);
    swapChainDesc.SampleDesc.Count = 1; // No MSAA
    swapChainDesc.SampleDesc.Quality = 0;
    swapChainDesc.Windowed = TRUE;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

    // Use the factory to create the swap chain
    HRESULT result = m_factory->CreateSwapChain(
        m_device.Get(),              // The device
        &swapChainDesc,              // Swap chain description
        m_swapChain.GetAddressOf()   // Output swap chain
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

    // Set the render target
    m_deviceContext->OMSetRenderTargets(1, m_renderTargetView.GetAddressOf(), nullptr);
}
// Configure the viewport
void RenderDeviceD3D11::ConfigureViewport() {
    int width, height;
    glfwGetFramebufferSize(m_window, &width, &height);

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

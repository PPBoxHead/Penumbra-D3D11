#include <iostream>

#include <d3d11.h>
#include <wrl/client.h> // For ComPtr

#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

using Microsoft::WRL::ComPtr;

ComPtr<ID3D11Device> g_device;
ComPtr<ID3D11DeviceContext> g_deviceContext;
ComPtr<IDXGISwapChain> g_swapChain;
ComPtr<ID3D11RenderTargetView> g_renderTargetView;

const int W_WIDTH = 800;
const int W_HEIGHT = 600;

void InitDirectX11(GLFWwindow* window) {
	// Swap-chain description
	DXGI_SWAP_CHAIN_DESC scDesc = {};
	scDesc.BufferDesc.Width = W_WIDTH;
	scDesc.BufferDesc.Height = W_HEIGHT;
	scDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	scDesc.BufferDesc.RefreshRate.Numerator = 60;
	scDesc.BufferDesc.RefreshRate.Denominator = 1;
	scDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	scDesc.BufferCount = 1;
	scDesc.OutputWindow = glfwGetWin32Window(window);
	scDesc.Windowed = TRUE;
	scDesc.SampleDesc.Count = 1;
	scDesc.SampleDesc.Quality = 0;
	scDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

	// now create device and swapchain
	UINT createDeviceFlags = 0;
#if defined(_DEBUG)
	createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
	D3D_FEATURE_LEVEL featureLevel;
	HRESULT hr = D3D11CreateDeviceAndSwapChain(
		nullptr,
		D3D_DRIVER_TYPE_HARDWARE,
		nullptr,
		createDeviceFlags,
		nullptr,
		0,
		D3D11_SDK_VERSION,
		&scDesc,
		g_swapChain.GetAddressOf(),
		g_device.GetAddressOf(),
		&featureLevel,
		g_deviceContext.GetAddressOf()
	);
	if (FAILED(hr)) {
		std::cerr << "Failed to create DirectX 11 device and swap chain." << std::endl;
		exit(-1);
	}

	// create render target
	ComPtr<ID3D11Texture2D> backBuffer;
	hr = g_swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(backBuffer.GetAddressOf()));
	if (FAILED(hr)) {
		std::cerr << "Failed to get back buffer." << std::endl;
		exit(-1);
	}

	hr = g_device->CreateRenderTargetView(backBuffer.Get(), nullptr, g_renderTargetView.GetAddressOf());
	if (FAILED(hr)) {
		std::cerr << "Failed to create render target view." << std::endl;
		exit(-1);
	}

	g_deviceContext->OMSetRenderTargets(1, g_renderTargetView.GetAddressOf(), nullptr);

	// setup viewport
	D3D11_VIEWPORT viewport = {};
	viewport.Width = static_cast<float>(W_WIDTH);
	viewport.Height = static_cast<float>(W_HEIGHT);
	viewport.TopLeftX = 0.0f;
	viewport.TopLeftY = 0.0f;
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;
	g_deviceContext->RSSetViewports(1, &viewport);
}


int main() {
	glfwInit();

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	GLFWwindow* window = glfwCreateWindow(W_WIDTH, W_HEIGHT, "Penumbra-D3D11 Window :D", nullptr, nullptr);
	if (window == nullptr) {
		std::cout << "GLFW: Unable to create window\n" << std::endl;
		glfwTerminate();
		return -1;
	}
	InitDirectX11(window);

	// Main Loop
	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();

		const float clearColor[4] = { 0.1f, 0.2f, 0.3f, 1.0f };
		g_deviceContext->ClearRenderTargetView(g_renderTargetView.Get(), clearColor);

		// Present the back buffer to the screen
		g_swapChain->Present(1, 0);
	}

	glfwTerminate();
	return 0;
}

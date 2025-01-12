#ifndef RENDER_DEVICE_D3D11_H
#define RENDER_DEVICE_D3D11_H

#include <d3d11.h>
#include <DirectXMath.h>
//#include <dxgi.h>
#include <dxgi1_4.h>
#include <wrl/client.h> // For ComPtr
#include <array>


class RenderDeviceD3D11 {
// Public Methods
public:
	RenderDeviceD3D11(int t_window_width, int t_window_height, HWND t_window);
	~RenderDeviceD3D11();

	// Frame lifecycle
	void StartFrame(const std::array<float, 4>& clearColor);
	void PresentFrame();

	void Resize(int newWidth, int newHeight);

	void QueryVRAMInfo();

	// Getters
	ID3D11Device* GetDevice();
	ID3D11DeviceContext* GetDeviceContext();

	IDXGISwapChain* GetSwapchain();

	ID3D11RenderTargetView* GetRenderTargetView();
	ID3D11DepthStencilView* GetDepthStencilView();

	DXGI_QUERY_VIDEO_MEMORY_INFO GetVideoMemoryInfo();

	char* GetVideoCardDescription();

	int GetWindowWidth();
	int GetWindowHeight();

// Private Member Variables
public:
	int videoCardDedicatedMemory;
	int videoCardSharedSystemMemory;

	float gpuFrameTime = 0.0f;

// Private Methods
private:
	// D3D11 Initialization pipeline
	void InitializeD3D11(HWND t_hwnd);
	// Creates the DXGI Factory
	void CreateFactory();
	// Enumerate the hardware adapter
	void SetupHardwareAdapter();
	// Create device and device context
	void InitializeDeviceAndContext();
	// Create the swapchain desc and setup the swapchain
	void CreateSwapChain(HWND t_hwnd);
	// Get the back buffer and create the render target view
	void CreateRenderTargetView();
	// Creates the stencil desc and setup the Stencil State, but also render desc and the Rasterizer State
	void CreateRenderPipeline();
	// Configure the viewport
	void SetupViewport();

	void InitializeGPUQuery();

	void LogHRESULTError(HRESULT hr, const char* message);

// Private Member Variables
private:
	bool m_vsyncEnabled = false;
	
	int m_windowWidth;
	int m_windowHeight;

	int m_renderWidth;
	int m_renderHeight;

	Microsoft::WRL::ComPtr<ID3D11Query> m_disjointQuery;
	Microsoft::WRL::ComPtr<ID3D11Query> m_startQuery;
	Microsoft::WRL::ComPtr<ID3D11Query> m_endQuery;

	Microsoft::WRL::ComPtr<IDXGIFactory4> m_factory;

	Microsoft::WRL::ComPtr<IDXGIAdapter3> m_adapter;
	Microsoft::WRL::ComPtr<IDXGIOutput> m_adapterOutput;
	unsigned m_numerator = 0;
	unsigned m_denominator = 1;
	char m_videoCardDescription[128];
	DXGI_QUERY_VIDEO_MEMORY_INFO m_videoMemoryInfo = {};

	Microsoft::WRL::ComPtr<ID3D11Device> m_device;
	Microsoft::WRL::ComPtr<ID3D11DeviceContext> m_deviceContext;
	Microsoft::WRL::ComPtr<IDXGISwapChain> m_swapChain;

	Microsoft::WRL::ComPtr<ID3D11Texture2D> m_depthStencilBuffer;
	Microsoft::WRL::ComPtr<ID3D11DepthStencilState> m_depthStencilState;
	Microsoft::WRL::ComPtr<ID3D11DepthStencilView> m_depthStencilView;
	Microsoft::WRL::ComPtr<ID3D11RasterizerState> m_rasterizerState;

	Microsoft::WRL::ComPtr<ID3D11Texture2D> m_backBuffer;
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> m_renderTargetView;

	D3D11_VIEWPORT m_viewport;
};


#endif // !RENDER_DEVICE_D3D11_H


#ifndef RENDER_DEVICE_D3D11_H
#define RENDER_DEVICE_D3D11_H

#include <d3d11.h>
#include <DirectXMath.h>
#include <dxgi.h>
#include <wrl/client.h> // For ComPtr
#include <array>

#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>


// Vertex structure
struct VertexData {
	DirectX::XMFLOAT3 vPosition;
	DirectX::XMFLOAT3 vNormal;
	DirectX::XMFLOAT4 vColor;
	DirectX::XMFLOAT2 vUv;
};

// Vertex structure
struct SimpleVertexData {
	DirectX::XMFLOAT3 vPosition;
	DirectX::XMFLOAT4 vColor;
};


class RenderDeviceD3D11 {
	public:
		RenderDeviceD3D11(int t_window_width, int t_window_height);
		~RenderDeviceD3D11();
		void InitD3D11();

		ID3D11Device* GetDevice();

		// Frame lifecycle
		void beginFrame(const std::array<float, 4>& clearColor);
		void endFrame();

		bool vsync = false;
		/// TODO -> Take this out and add a pointer reference as argument. Basically quit the window creation from this class
		GLFWwindow* window;
	private:
		// Creates the DXGI Factory
		void CreateDXGIFactoryInstance();
		// Enumerate the hardware adapter
		void InitializeHardwareAdapter();
		// Create device and device context
		void CreateDevice();
		// Create the swapchain desc and setup the swapchain
		void CreateSwapChain();
		// Get the back buffer and create the render target view
		void CreateRenderTargetView();
		/// TODO -> Change the name of this to CreateRenderPipeline() and add Raster State creation
		// Create the stencil desc and setup the stencil state
		void CreateStencilState();
		// Configure the viewport
		void ConfigureViewport();

		int m_videoCardMemory;
		char m_videoCardDescription[128];

		Microsoft::WRL::ComPtr<IDXGIFactory> m_factory;
		Microsoft::WRL::ComPtr<IDXGIAdapter> m_adapter;
		Microsoft::WRL::ComPtr <IDXGIOutput> m_adapterOutput;
		unsigned m_numerator = 0;
		unsigned m_denominator = 1;

		Microsoft::WRL::ComPtr<ID3D11Device> m_device;
		Microsoft::WRL::ComPtr<ID3D11DeviceContext> m_deviceContext;
		Microsoft::WRL::ComPtr<IDXGISwapChain> m_swapChain;
		Microsoft::WRL::ComPtr<ID3D11DepthStencilState> m_depthStencilState;
		Microsoft::WRL::ComPtr<ID3D11DepthStencilView> m_depthStencilView;
		Microsoft::WRL::ComPtr<ID3D11Texture2D> m_backBuffer;
		Microsoft::WRL::ComPtr<ID3D11RenderTargetView> m_renderTargetView;

		D3D11_VIEWPORT m_viewport;

		void LogHRESULTError(HRESULT hr, const char* message);
};


#endif // !RENDER_DEVICE_D3D11_H

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

using Microsoft::WRL::ComPtr;


// Vertex structure
struct VertexData {
	DirectX::XMFLOAT3 v_position;
	DirectX::XMFLOAT4 v_colors;
};


class RenderDeviceD3D11 {
	public:
		RenderDeviceD3D11(int t_window_width, int t_window_height);
		~RenderDeviceD3D11();
		void InitD3D11();

		// Frame lifecycle
		void beginFrame(const std::array<float, 4>& clearColor);
		void endFrame();

		GLFWwindow* m_window;
	private:

		ComPtr<IDXGIFactory> m_factory;
		ComPtr<IDXGIAdapter> m_adapter;

		ComPtr<ID3D11Device> m_device;
		ComPtr<ID3D11DeviceContext> m_deviceContext;
		ComPtr<IDXGISwapChain> m_swapChain;
		ComPtr<ID3D11Texture2D> m_backBuffer;
		ComPtr<ID3D11RenderTargetView> m_renderTargetView;

		D3D11_VIEWPORT m_viewport;

		void LogHRESULTError(HRESULT hr, const char* message);
};


#endif // !RENDER_DEVICE_D3D11_H


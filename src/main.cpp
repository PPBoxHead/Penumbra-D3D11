#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

#include <d3dcompiler.h>

#include "graphics/RenderDeviceD3D11.hpp"
//#include "utils/ConsoleLogger.hpp"

using namespace Microsoft::WRL;

int main() {
	glfwInit();

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	GLFWwindow* window = glfwCreateWindow(800, 600, "Penumbra-D3D11 Window :D", nullptr, nullptr);
	if (window == nullptr) {
		//ConsoleLogger::consolePrint(ConsoleLogger::LogType::C_ERROR, "Unable to create GLFW window");
		glfwTerminate();
		exit(EXIT_FAILURE);
	}
	RenderDeviceD3D11* renderDevice = new RenderDeviceD3D11(800, 600, glfwGetWin32Window(window));

	/// How to render a colored triangle in D3D11:
	ID3D11Device* device = renderDevice->GetDevice(); // This is to call the creation of buffers with device->CreateBuffer() call
	ID3D11DeviceContext* deviceContext = renderDevice->GetDeviceContext();
		/// First create the Vertex Buffer
	// Declare the vertices and their data, in this case, vertex position and vertex colors
	SimpleVertexData vertices[] = {
		{ DirectX::XMFLOAT3(0.0f, 0.5f, 0.0f), DirectX::XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f) }, // Top (Red)
		{ DirectX::XMFLOAT3(0.5f, -0.5f, 0.0f), DirectX::XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f) }, // Bottom Right (Green)
		{ DirectX::XMFLOAT3(-0.5f, -0.5f, 0.0f), DirectX::XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f) }  // Bottom Left (Blue)
	};
	// Now let's make the buffers for the triangle
	ComPtr<ID3D11Buffer> vertexBuffer;
	D3D11_BUFFER_DESC vertexBufferDesc = {};
	vertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	vertexBufferDesc.ByteWidth = sizeof(vertices);
	vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	// Idk what subresources do yet tbh lol
	D3D11_SUBRESOURCE_DATA vertexData = {};
	vertexData.pSysMem = vertices;
	// Create the vertex buffer
	device->CreateBuffer(&vertexBufferDesc, &vertexData, &vertexBuffer);
		/// Now let's create the index buffer
	// We declare the indices here
	uint32_t indices[] = {0, 1, 2};
	// Now let's make the buffers for the indices
	ComPtr<ID3D11Buffer> indexBuffer;
	D3D11_BUFFER_DESC indexBufferDesc = {};
	indexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	indexBufferDesc.ByteWidth = sizeof(unsigned) * 3;
	indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	D3D11_SUBRESOURCE_DATA indexData;
	indexData.pSysMem = indices;
	// Create the index buffer
	device->CreateBuffer(&indexBufferDesc, &indexData, &indexBuffer);

		/// Now let's create and compile Vertex and Pixel shaders
	const char* hlsl = R"(
		struct VertexIn {
			float3 position : POS;
			float4 color : COL;
		};

		struct VertexOut {
			float4 position : SV_POSITION;
			float4 color : COL;
		};

		VertexOut vs_main(VertexIn input) {
			VertexOut output;
			output.position = float4(input.position, 1.0);
			output.color = input.color;
			return output;
		}

		float4 ps_main(VertexOut input) : SV_TARGET {
			return input.color;
		}
	)";

	UINT compile_flags = D3DCOMPILE_PACK_MATRIX_COLUMN_MAJOR |
		D3DCOMPILE_ENABLE_STRICTNESS |
		D3DCOMPILE_WARNINGS_ARE_ERRORS;
	compile_flags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;


	ComPtr<ID3D11InputLayout> inputLayout;
	// Here we compile and create the vertex shader
	ComPtr<ID3D11VertexShader> vertex_shader;
	{
		ID3DBlob* vertexBlob = nullptr;
		D3DCompile(
			hlsl,
			strlen(hlsl),
			nullptr,
			nullptr,
			D3D_COMPILE_STANDARD_FILE_INCLUDE,
			"vs_main",
			"vs_5_0",
			compile_flags,
			0,
			&vertexBlob,
			nullptr
		);
		device->CreateVertexShader(vertexBlob->GetBufferPointer(), vertexBlob->GetBufferSize(), nullptr, &vertex_shader);
		D3D11_INPUT_ELEMENT_DESC inputElementDesc[] = {
			{"POS", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
			{"COL", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
		};

		device->CreateInputLayout(inputElementDesc, ARRAYSIZE(inputElementDesc), vertexBlob->GetBufferPointer(), vertexBlob->GetBufferSize(), &inputLayout);
	}

	// And here we compile and create the pixel shader
	ComPtr<ID3D11PixelShader> pixel_shader;
	{
		ID3DBlob* pixelBlob = nullptr;
		D3DCompile(
			hlsl,
			strlen(hlsl),
			nullptr,
			nullptr,
			D3D_COMPILE_STANDARD_FILE_INCLUDE,
			"ps_main",
			"ps_5_0",
			compile_flags,
			0,
			&pixelBlob,
			nullptr
		);
		device->CreatePixelShader(pixelBlob->GetBufferPointer(), pixelBlob->GetBufferSize(), nullptr, &pixel_shader);

		pixelBlob->Release();
	}

	std::array<float, 4> clearColor = { 0.1f, 0.2f, 0.3f, 1.0f };
	// Main Loop
	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();

		// Clear the render target and depth/stencil view
		renderDevice->StartFrame(clearColor);

			/// We render a triangle here lol
		// Set the input layout
		deviceContext->IASetInputLayout(inputLayout.Get());

		// Bind the shaders
		deviceContext->VSSetShader(vertex_shader.Get(), nullptr, 0);
		deviceContext->PSSetShader(pixel_shader.Get(), nullptr, 0);

		UINT stride = sizeof(SimpleVertexData);
		UINT offset = 0;
		//Bind the vertex buffer
		deviceContext->IASetVertexBuffers(0, 1, vertexBuffer.GetAddressOf(), &stride, &offset);
		// Bind the index buffer
		deviceContext->IASetIndexBuffer(indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);

		// Define the the type of primitive topology that should be rendered from this vertex buffer, in this case triangles
		deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		// Draw the triangle :D
		deviceContext->DrawIndexed(3, 0, 0);

		// Present the back buffer to the screen
		renderDevice->PresentFrame();
	}

	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;
}

#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

#include <chrono>
#include <d3dcompiler.h>

#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_glfw.h>
#include <imgui/backends/imgui_impl_dx11.h>

#include "graphics/RenderDeviceD3D11.h"
#include "graphics/VertexFormat.h"
#include "graphics/Shader.h"

#include "utils/ConsoleLogger.h"
#include "utils/FileSystem.h"


using namespace Microsoft::WRL;

RenderDeviceD3D11* renderDevice = nullptr;

#include <array>
#include <cstring>

char processorName[128] = {};
void GetProcessorName(char* processorName) {
	int CPUInfo[4] = { -1 };
	unsigned int nExIds, i = 0;
	char brand[0x40] = { 0 };

	// Get the CPU vendor string
	__cpuid(CPUInfo, 0);
	char vendor[13];
	*reinterpret_cast<int*>(vendor) = CPUInfo[1];
	*reinterpret_cast<int*>(vendor + 4) = CPUInfo[3];
	*reinterpret_cast<int*>(vendor + 8) = CPUInfo[2];
	vendor[12] = '\0';

	// Get the processor brand string
	__cpuid(CPUInfo, 0x80000000);
	nExIds = CPUInfo[0];
	for (i = 0x80000000; i <= nExIds; ++i) {
		__cpuid(CPUInfo, i);
		if (i == 0x80000002) {
			std::memcpy(brand, CPUInfo, sizeof(CPUInfo));
		}
		else if (i == 0x80000003) {
			std::memcpy(brand + 16, CPUInfo, sizeof(CPUInfo));
		}
		else if (i == 0x80000004) {
			std::memcpy(brand + 32, CPUInfo, sizeof(CPUInfo));
		}
	}

	// Copy the processor brand string to the output
	strcpy_s(processorName, 128, brand);
}

// Variables for FPS tracking
std::chrono::high_resolution_clock::time_point lastTime = std::chrono::high_resolution_clock::now();
float deltaTime = 0.0f;
float fps = 0.0f;

// Update FPS
void UpdateFPS() {
	auto currentTime = std::chrono::high_resolution_clock::now();
	deltaTime = std::chrono::duration<float>(currentTime - lastTime).count();
	lastTime = currentTime;

	if (deltaTime > 0) {
		fps = 1.0f / deltaTime;
	}
}

// Render ImGui FPS Counter
void RenderImGuiPerformance() {
	renderDevice->GetVRAMInfo();
	// Calculate VRAM usage in MB
	size_t usedVRAM = renderDevice->videoMemoryInfo.CurrentUsage / 1024 / 1024;  // In MB

	ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_Always);
	ImGui::SetNextWindowBgAlpha(0.3f); // Transparent background
	ImGui::Begin("Performance", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
	ImGui::Text("FPS: %.1f", fps); // Display the FPS with one decimal point
	ImGui::Text("Frame Time: %.3f ms", deltaTime * 1000.0f); // Display frame time in milliseconds
	if (ImGui::CollapsingHeader("GPU Data")) {
		ImGui::Text("GPU Vendor: %s", renderDevice->videoCardDescription);
		ImGui::Text("Graphics Adapter Dedicated VRAM: %i MB", renderDevice->videoCardDedicatedMemory);
		ImGui::Text("Graphics Adapter Shared RAM: %i MB", renderDevice->videoCardSharedSystemMemory);
		ImGui::Text("Used VRAM: %zu MB", usedVRAM);
		ImGui::Separator();
	}
	if (ImGui::CollapsingHeader("CPU Data")) {
		ImGui::Text("CPU Vendor: %s", processorName);
		// Memory status structure
		MEMORYSTATUSEX statex;
		statex.dwLength = sizeof(statex);

		// Get memory status
		if (GlobalMemoryStatusEx(&statex)) {
			// Total physical memory in MB
			size_t totalRAM = statex.ullTotalPhys / 1024 / 1024;

			// Available physical memory in MB
			size_t availableRAM = statex.ullAvailPhys / 1024 / 1024;

			// Used RAM in MB
			size_t usedRAM = totalRAM - availableRAM;

			// Render RAM usage with ImGui
			ImGui::Text("Total RAM: %zu MB", totalRAM);
			ImGui::Text("Available RAM: %zu MB", availableRAM);
			ImGui::Text("Used RAM: %zu MB", usedRAM);
		}
		else {
			ImGui::Text("Failed to retrieve memory status.");
		}
	}
	ImGui::End();
}


int main() {
	GetProcessorName(processorName);

	FileSystem::setWorkingDirectory("resources");

	glfwInit();

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	GLFWwindow* window = glfwCreateWindow(1280, 720, "Penumbra-D3D11 Window :D", nullptr, nullptr);
	if (window == nullptr) {
		ConsoleLogger::Print(ConsoleLogger::LogType::C_ERROR, "Unable to create GLFW window");
		glfwTerminate();
		exit(EXIT_FAILURE);
	}
	renderDevice = new RenderDeviceD3D11(1280, 720, glfwGetWin32Window(window));

	/// How to render a colored triangle in D3D11:
	ID3D11Device* device = renderDevice->GetDevice(); // This is to call the creation of buffers with device->CreateBuffer() call
	ID3D11DeviceContext* deviceContext = renderDevice->GetDeviceContext();
		/// First create the Vertex Buffer
	// Declare the vertices and their data, in this case, vertex position and vertex colors
	ColoredVertexData vertices[] = {
		{ DirectX::XMFLOAT3(-0.5f,  0.5f, 0.0f), DirectX::XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f) },  // Top-left
		{ DirectX::XMFLOAT3(0.5f,  0.5f, 0.0f), DirectX::XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f) },  // Top-right
		{ DirectX::XMFLOAT3(-0.5f, -0.5f, 0.0f), DirectX::XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f) },  // Bottom-left
		{ DirectX::XMFLOAT3(0.5f, -0.5f, 0.0f), DirectX::XMFLOAT4(1.0f, 1.0f, 0.0f, 1.0f) }   // Bottom-right
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
	uint32_t indices[] = {
		0, 1, 2,
		1, 3, 2 
	};
	// Now let's make the buffers for the indices
	ComPtr<ID3D11Buffer> indexBuffer;
	D3D11_BUFFER_DESC indexBufferDesc = {};
	indexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	indexBufferDesc.ByteWidth = sizeof(indices);
	indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	D3D11_SUBRESOURCE_DATA indexData;
	indexData.pSysMem = indices;
	// Create the index buffer
	device->CreateBuffer(&indexBufferDesc, &indexData, &indexBuffer);

		/// Let's try create our new shader class thing
	struct ConstantBufferData {
		DirectX::XMMATRIX worldMatrix;
	};

	// Example input layout (this should match your vertex structure)
	D3D11_INPUT_ELEMENT_DESC layout[] = {
		{"POS", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"COL", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0}
	};
	UINT numElements = ARRAYSIZE(layout);


	SHADER_DESC shaderDesc = {};
	shaderDesc.vertexShaderPath = L"shaders/TestShader_vs.hlsl";
	shaderDesc.pixelShaderPath = L"shaders/TestShader_ps.hlsl";
	
	Shader shaderTest;
	shaderTest.Initialize(device, shaderDesc, layout, numElements);

	DirectX::XMMATRIX transformMatrix = DirectX::XMMatrixIdentity();

		/// Let's try initialize ImGui
	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // IF using Docking Branch

	ImGui::StyleColorsDark();

	// Setup Platform/Renderer backends
	ImGui_ImplGlfw_InitForOther(window, true);
	ImGui_ImplDX11_Init(device, deviceContext);

	std::array<float, 4> clearColor = { 0.1f, 0.2f, 0.3f, 1.0f };
	// Main Loop
	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();

		UpdateFPS();

		ImGui_ImplDX11_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();
		//ImGui::ShowDemoWindow();

		RenderImGuiPerformance();

		// Clear the render target and depth/stencil view
		renderDevice->StartFrame(clearColor);


			/// We render a triangle here lol
		shaderTest.SetShaders(deviceContext);

		UINT stride = sizeof(ColoredVertexData);
		UINT offset = 0;
		//Bind the vertex buffer
		deviceContext->IASetVertexBuffers(0, 1, vertexBuffer.GetAddressOf(), &stride, &offset);
		// Bind the index buffer
		deviceContext->IASetIndexBuffer(indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);

		// Define the the type of primitive topology that should be rendered from this vertex buffer, in this case triangles
		deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		
		// Draw the triangle :D
		deviceContext->DrawIndexed(6, 0, 0);


		ImGui::Render();
		ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

		// Present the back buffer to the screen
		renderDevice->PresentFrame();
	}

	ImGui_ImplDX11_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;
}

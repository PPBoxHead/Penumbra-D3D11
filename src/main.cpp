#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

#include <numeric>
#include <chrono>
#include <d3dcompiler.h>

#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_glfw.h>
#include <imgui/backends/imgui_impl_dx11.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

#include "graphics/RenderDeviceD3D11.h"
#include "graphics/VertexFormat.h"
#include "graphics/Shader.h"

#include "utils/ConsoleLogger.h"
#include "utils/FileSystem.h"


using namespace Microsoft::WRL;

std::unique_ptr<RenderDeviceD3D11> renderDevice;

#include <array>
#include <cstring>
#include <psapi.h>

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

std::chrono::high_resolution_clock::time_point m_LastFrameTime = std::chrono::high_resolution_clock::now();
double m_TimeAccumulator = 0.0;
double m_Delta = 0.0;
double m_AvgFPS = 0.0;
std::vector<double> m_FrameTimes;


void UpdateFPS() {
	auto currentFrameTime = std::chrono::high_resolution_clock::now();
	m_Delta = std::chrono::duration<double>(currentFrameTime - m_LastFrameTime).count();
	m_LastFrameTime = currentFrameTime;
	m_TimeAccumulator += m_Delta;
	m_FrameTimes.push_back(1.0 / m_Delta);

	if (m_TimeAccumulator >= 1.0) {
		m_AvgFPS = std::accumulate(m_FrameTimes.begin(), m_FrameTimes.end(), 0.0) / m_FrameTimes.size();
		m_FrameTimes.clear();
		m_TimeAccumulator -= 1.0;
	}
}

std::chrono::high_resolution_clock::time_point cpuStartTime;
float cpuFrameTime = 0.0f;

// Render ImGui FPS Counter
void RenderImGuiPerformance() {
	renderDevice->GetVRAMInfo();
	// Calculate VRAM usage in MB
	size_t usedVRAM = renderDevice->videoMemoryInfo.CurrentUsage / 1024 / 1024;  // In MB
	float totalFrameTime = cpuFrameTime + renderDevice->gpuFrameTime;

	ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_Always);
	ImGui::SetNextWindowBgAlpha(1.0f); // Dark background
	ImGui::Begin("Performance", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiTreeNodeFlags_DefaultOpen);
	ImGui::Text("Average FPS: %.2f", m_AvgFPS); // Display the FPS with one decimal point
	ImGui::Text("Total Frame Time: %.4f ms", totalFrameTime);
	ImGui::Text("Last Delta: %.4f ms", m_Delta * 1000.0f); // Display frame time in milliseconds
	ImGui::Text("Window Size: %ix%ipx", renderDevice->m_windowWidth, renderDevice->m_windowHeight); // Display frame time in milliseconds
	if (ImGui::CollapsingHeader("GPU Data", ImGuiTreeNodeFlags_DefaultOpen)) {
		ImGui::Text("GPU Vendor: %s", renderDevice->videoCardDescription);
		ImGui::Text("GPU frame time: %.4f ms", renderDevice->gpuFrameTime);
		ImGui::Text("Graphics Adapter Dedicated VRAM: %i MB", renderDevice->videoCardDedicatedMemory);
		ImGui::Text("Graphics Adapter Shared RAM: %i MB", renderDevice->videoCardSharedSystemMemory);
		ImGui::Text("Used VRAM: %zu MB", usedVRAM);
		ImGui::Separator();
	}
	if (ImGui::CollapsingHeader("CPU Data", ImGuiTreeNodeFlags_DefaultOpen)) {
		ImGui::Text("CPU Vendor: %s", processorName);
		ImGui::Text("CPU frame time: %.4f ms", cpuFrameTime);
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
			// Render global RAM usage with ImGui
			ImGui::Text("Total RAM: %zu MB", totalRAM);
			ImGui::Text("Available RAM: %zu MB", availableRAM);
			ImGui::Text("Used RAM (System-wide): %zu MB", usedRAM);
		}
		else {
			ImGui::Text("Failed to retrieve global memory status.");
		}

		// Memory usage of the current process
		HANDLE hProcess = GetCurrentProcess();
		PROCESS_MEMORY_COUNTERS_EX pmc;

		if (GetProcessMemoryInfo(hProcess, (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc))) {
			// Working set size (RAM currently in use by the process) in MB
			size_t workingSetSize = pmc.WorkingSetSize / 1024 / 1024;

			// Private bytes (committed memory) in MB
			size_t privateBytes = pmc.PrivateUsage / 1024 / 1024;

			// Render process memory usage with ImGui
			ImGui::Text("Program RAM Usage (Working Set): %zu MB", workingSetSize);
			ImGui::Text("Program RAM Usage (Private Bytes): %zu MB", privateBytes);
		}
		else {
			ImGui::Text("Failed to retrieve program memory status.");
		}

		// Close the process handle (not strictly necessary for GetCurrentProcess)
		CloseHandle(hProcess);
	}
	ImGui::End();
}

void FramebufferSizeCallback(GLFWwindow* window, int width, int height)
{
	// Store the new width and height, and mark the need to resize DirectX resources.
	if (width > 0 && height > 0 && renderDevice)
	{
		renderDevice->Resize(width, height);
	}
}

void UpdateRotation(Shader& shader, ID3D11DeviceContext* context, float angle)
{
	DirectX::XMMATRIX worldMatrix = DirectX::XMMatrixRotationAxis(DirectX::FXMVECTOR{0.0f, 0.0f, 1.0f}, angle);
	worldMatrix = DirectX::XMMatrixTranspose(worldMatrix); // Transpose for HLSL compatibility

	shader.UpdateConstantBuffer(context, "WorldMatrixBuffer", &worldMatrix, sizeof(worldMatrix));
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
	// Set the framebuffer size callback.
	glfwSetFramebufferSizeCallback(window, FramebufferSizeCallback);

	// Disable V-Sync in GLFW
	glfwSwapInterval(0); // This sets the swap interval to 0, disabling V-Sync
	

	renderDevice = std::make_unique<RenderDeviceD3D11>(1280, 720, glfwGetWin32Window(window));

	/// How to render a colored quad in D3D11:
	ID3D11Device* device = renderDevice->GetDevice(); // This is to call the creation of buffers with device->CreateBuffer() call
	ID3D11DeviceContext* deviceContext = renderDevice->GetDeviceContext();
		/// First create the Vertex Buffer
	// Declare the vertices and their data, in this case, vertex position and vertex colors
	ColoredTexturedVertexData vertices[] = {
		{ DirectX::XMFLOAT3(-0.5f,  0.5f, 0.0f), DirectX::XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f), DirectX::XMFLOAT2(0.0f, 0.0f) },  // Top-left
		{ DirectX::XMFLOAT3(0.5f,  0.5f, 0.0f), DirectX::XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f), DirectX::XMFLOAT2(1.0f, 0.0f) },  // Top-right
		{ DirectX::XMFLOAT3(-0.5f, -0.5f, 0.0f), DirectX::XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f), DirectX::XMFLOAT2(0.0f, 1.0f) },  // Bottom-left
		{ DirectX::XMFLOAT3(0.5f, -0.5f, 0.0f), DirectX::XMFLOAT4(1.0f, 1.0f, 0.0f, 1.0f), DirectX::XMFLOAT2(1.0f, 1.0f) }   // Bottom-right
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

		/// Here we create a shader and initialize it!
	// Example input layout (this should match your vertex structure)
	D3D11_INPUT_ELEMENT_DESC layout[] = {
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0}  // Texture Coordinates
	};

	SHADER_DESC shaderDesc = {};
	shaderDesc.vertexShaderPath = L"shaders/ColoredTexturedVertex_vs.hlsl";
	shaderDesc.pixelShaderPath = L"shaders/ColoredTexturedVertex_ps.hlsl";
	
	Shader shaderTest;
	shaderTest.Initialize(device, shaderDesc, layout);

	CONSTANT_BUFFER_DESC cBufferDesc = {};
	cBufferDesc.bufferSize = sizeof(DirectX::XMMATRIX);

	shaderTest.CreateConstantBuffer(device, "WorldMatrixBuffer", cBufferDesc);
	float angle = 1.0f;

		/// Now let's create a texturE
	// craete texture
	ID3D11Texture2D* texture = nullptr;
	int ImageWidth;
	int ImageHeight;
	int ImageChannels;
	int ImageDesiredChannels = 4;

	unsigned char* ImageData = stbi_load("textures/tile_64x64.png",
		&ImageWidth,
		&ImageHeight,
		&ImageChannels, ImageDesiredChannels);
	assert(ImageData);

	int ImagePitch = ImageWidth * 4;

	// Texture

	D3D11_TEXTURE2D_DESC ImageTextureDesc = {};

	ImageTextureDesc.Width = ImageWidth;
	ImageTextureDesc.Height = ImageHeight;
	ImageTextureDesc.MipLevels = 1;
	ImageTextureDesc.ArraySize = 1;
	ImageTextureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	ImageTextureDesc.SampleDesc.Count = 1;
	ImageTextureDesc.SampleDesc.Quality = 0;
	ImageTextureDesc.Usage = D3D11_USAGE_IMMUTABLE;
	ImageTextureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

	D3D11_SUBRESOURCE_DATA ImageSubresourceData = {};

	ImageSubresourceData.pSysMem = ImageData;
	ImageSubresourceData.SysMemPitch = ImagePitch;

	device->CreateTexture2D(&ImageTextureDesc,
		&ImageSubresourceData,
		&texture
	);


	free(ImageData);

	// Shader resource view
	ID3D11ShaderResourceView* ImageShaderResourceView;
	device->CreateShaderResourceView(texture,
		nullptr,
		&ImageShaderResourceView
	);

	// Sampler
	D3D11_SAMPLER_DESC ImageSamplerDesc = {};
	ImageSamplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	ImageSamplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	ImageSamplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	ImageSamplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	ImageSamplerDesc.MipLODBias = 0.0f;
	ImageSamplerDesc.MaxAnisotropy = 1;
	ImageSamplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	ImageSamplerDesc.BorderColor[0] = 1.0f;
	ImageSamplerDesc.BorderColor[1] = 1.0f;
	ImageSamplerDesc.BorderColor[2] = 1.0f;
	ImageSamplerDesc.BorderColor[3] = 1.0f;
	ImageSamplerDesc.MinLOD = -FLT_MAX;
	ImageSamplerDesc.MaxLOD = FLT_MAX;

	ID3D11SamplerState* ImageSamplerState;

	device->CreateSamplerState(&ImageSamplerDesc,
		&ImageSamplerState);
	
	// Assuming a Shader instance `shader` and valid SRV and sampler are created:
	shaderTest.CreateShaderResourceView("diffuseTexture", ImageShaderResourceView);
	shaderTest.CreateSamplerState("samplerState", ImageSamplerState);

	// Binding SRV and Sampler to Pixel Shader slot 0
	shaderTest.BindShaderResourceView(deviceContext, "diffuseTexture", 0, ShaderStage::PixelShader);
	shaderTest.BindSamplerState(deviceContext, "samplerState", 0, ShaderStage::PixelShader);

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
		cpuStartTime = std::chrono::high_resolution_clock::now();

		ImGui_ImplDX11_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();
		//ImGui::ShowDemoWindow();

		RenderImGuiPerformance();

		// Clear the render target and depth/stencil view
		renderDevice->StartFrame(clearColor);

		// Update the rotation angle
		angle += 0.01f;

		// Update constant buffer
		UpdateRotation(shaderTest, deviceContext, angle);

		shaderTest.SetShaders(deviceContext);

		shaderTest.BindConstantBuffer(deviceContext, "WorldMatrixBuffer", 0, ShaderStage::VertexShader);

		UINT stride = sizeof(ColoredTexturedVertexData);
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

		auto cpuEndTime = std::chrono::high_resolution_clock::now();
		cpuFrameTime = std::chrono::duration<float, std::milli>(cpuEndTime - cpuStartTime).count();
		
	}

	ImGui_ImplDX11_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;
}

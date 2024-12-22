#include <iostream>

#include "graphics/RenderDeviceD3D11.hpp"

int main() {

	RenderDeviceD3D11* renderDevice = new RenderDeviceD3D11(800, 600);

	std::array<float, 4> clearColor = { 0.1f, 0.2f, 0.3f, 1.0f };
	// Main Loop
	while (!glfwWindowShouldClose(renderDevice->window)) {
		glfwPollEvents();

		renderDevice->StartFrame(clearColor);

		// Present the back buffer to the screen
		renderDevice->PresentFrame();
	}

	glfwTerminate();
	return 0;
}

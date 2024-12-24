#include <iostream>

#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

#include "graphics/RenderDeviceD3D11.hpp"
//#include "utils/ConsoleLogger.hpp"


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

	std::array<float, 4> clearColor = { 0.1f, 0.2f, 0.3f, 1.0f };
	// Main Loop
	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();

		renderDevice->StartFrame(clearColor);


		// Render a triangle here lol


		// Present the back buffer to the screen
		renderDevice->PresentFrame();
	}

	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;
}

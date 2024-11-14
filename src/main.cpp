#include <iostream>

#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>


int main() {
	std::cout << "Lol\n" << std::endl;

	glfwInit();

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	GLFWwindow* window = glfwCreateWindow(800, 600, "Penumbra-D3D11 Window :D", nullptr, nullptr);
	if (window == nullptr) {
		std::cout << "GLFW: Unable to create window\n" << std::endl;
		glfwTerminate();
		return -1;
	}

	// Main Loop
	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();
		// future update code
		// future render code
	}

	glfwTerminate();
	return 0;
}

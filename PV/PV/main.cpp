#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <stdexcept>
#include <functional>
#include <cstdlib>

class PVWindow {
public:
	void run() {
		initWindow();
		initRenderer();
		runLoop();
		cleanup();
	}

private:

#pragma region Constants

	const int windowWidth = 800;
	const int windowHeight = 600;

#pragma endregion
	// Data
#pragma region Data
	GLFWwindow *window;
	bool closingWindow;

#pragma endregion
	// Methods
	void initWindow() {
		
		window = nullptr;
		closingWindow = false;

		// initialize GLFW
		glfwInit();
		// create without OpenGL context
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		// disable window resizing
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
		//create the GLFW window
		window = glfwCreateWindow(windowWidth, windowHeight, "GAM 400 Independent Study - Vulkan Renderer - Micah Rust", nullptr, nullptr);


	}
	void initRenderer() {

	}

	void runLoop() 
	{
		while (glfwWindowShouldClose(window) == false)
		{
			glfwPollEvents();
		}
	}

	void cleanup() {

		// destroy GLFW window
		glfwDestroyWindow(window);
		window = nullptr;
		// close down glfw
		glfwTerminate();

	}
};

int main() {
	PVWindow app;

	try {
		app.run();
	}
	catch (const std::runtime_error& e) {
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
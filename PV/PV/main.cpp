#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <stdexcept>
#include <functional>
#include <cstdlib>
#include <string>
#include <vector>

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
	VkDebugReportFlagsEXT flags,
	VkDebugReportObjectTypeEXT objType,
	uint64_t obj,
	size_t location,
	int32_t code,
	const char* layerPrefix,
	const char* msg,
	void* userData);

#pragma region Macros
#define allocnullptr nullptr
#define PVVK_RUN(VK_OPERATION)																													\
do { VkResult res = VK_OPERATION;																												\
	if (res != VK_SUCCESS)																														\
	{																																			\
		std::string msg;																														\
		msg.append("ERROR, VULKAN Operation: " #VK_OPERATION).append(" in file ").append(__FILE__).append(" at line ").append(std::to_string(__LINE__)).append(" failed with code: ").append(std::to_string(res));	\
		throw std::runtime_error(msg);																											\
	}																																			\
} while (0);
#pragma endregion


class PVWindow {
public:
	
	// code interface
	void run() {
		initWindow();
		initRenderer();
		runLoop();
		cleanup();
	}

private:

#pragma region structures

	struct QueueFamilyIndices
	{
		int graphicsFamily = -1;
		bool isComplete()
		{
			return graphicsFamily >= 0;
		}
	};

#pragma endregion


#pragma region Constants

	const int PV_WINDOW_WIDTH = 800;
	const int PV_WINDOW_HEIGHT = 600;
	const char* WINDOW_NAME = "GAM 400 Independent Study - Vulkan Renderer - Micah Rust";
	const char* APPLICATION_NAME = WINDOW_NAME;
	const char* ENGINE_NAME = APPLICATION_NAME;

	const bool PRINT_AVAILABLE_VULKAN_EXTENSIONS = true;
	const bool PRINT_DEBUG_LOGS = true;

	

#ifdef NDEBUG
	const bool enableValidationLayers = false;
#else
	const bool enableValidationLayers = true;
#endif

#pragma endregion

	// Data
#pragma region Data
	GLFWwindow *pvwindow;

	VkInstance pvinstance;
	VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
	VkDevice device = VK_NULL_HANDLE;
	VkQueue graphicsQueue;
	VkSurfaceKHR surface;
	VkDebugReportCallbackEXT debugCallbackExt;

	QueueFamilyIndices selectedQueueFamily;
	float queuePriority = 1.0f;

	std::vector<const char*> validationLayers =
	{
		"VK_LAYER_LUNARG_standard_validation",
	};
	std::vector<const char*> extensions =
	{
	};
#pragma endregion

	// Methods
#pragma region Init
	// Init GLFW window
	void initWindow() {
		
		pvwindow = nullptr;
		// initialize GLFW
		glfwInit();
		// create without OpenGL context
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		// disable window resizing
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
		//create the GLFW window
		pvwindow = glfwCreateWindow(PV_WINDOW_WIDTH, PV_WINDOW_HEIGHT, WINDOW_NAME, nullptr, nullptr);
		
	}
	// init Vulkan
	void initRenderer() 
	{
		createInstance();
		setupDebugCallback();
		createSurface();
		pickPhysicalDevice();
		createLogicalDevice();
	}

	void createInstance()
	{
		// print out all available extensions (DEBUG)
		if (PRINT_AVAILABLE_VULKAN_EXTENSIONS)
		{
			uint32_t extensionCount = 0;
			vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
			std::vector<VkExtensionProperties> availableExtensionProperties(extensionCount);
			vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, availableExtensionProperties.data());

			std::cout << "Available Vuklan Extensions: \n";
			for (const auto& ext : availableExtensionProperties)
			{
				std::cout << "\t" << ext.extensionName << "Spec Version: " << ext.specVersion << "\n";
			}
			std::cout << std::endl;
		}

		// check for critical extensions here (VERIFICATION)
		if (checkExtensionSupport() == false)
		{
			throw std::runtime_error("Vuklan Extension Support Check failed. Not all listed exteions are avaiable!");
		}

		// validate Layers (DEBUG)
		if (enableValidationLayers && checkValidationLayerSupport() == false)
		{
			throw std::runtime_error("Validation Layer Support Check failed. Not all listed layers are available!");
		}

		// Create Vulkan Instance (Create)
		{
			// setup Application Info
			VkApplicationInfo appInfo = {};
			appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
			appInfo.pApplicationName = APPLICATION_NAME;
			appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
			appInfo.pEngineName = ENGINE_NAME;
			appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
			appInfo.apiVersion = VK_API_VERSION_1_0; // @TODO @BUG? May want to change this to 1.0?

			// setup instance CreationInfo 
			VkInstanceCreateInfo createInfo = {};
			createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
			createInfo.pApplicationInfo = &appInfo;

			// Adding Etensions
			createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
			createInfo.ppEnabledExtensionNames = extensions.data();

			// Added Validation Layers
			if (enableValidationLayers) 
			{
				createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
				createInfo.ppEnabledLayerNames = validationLayers.data();
			}
			else
			{
				createInfo.enabledLayerCount = 0;
			}

			// Creating the vulkan instance
			PVVK_RUN(vkCreateInstance(&createInfo, allocnullptr, &pvinstance));
		}
	}
	void setupDebugCallback()
	{
		if (!enableValidationLayers) return;

		VkDebugReportCallbackCreateInfoEXT createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
		// Lets us filter the types of messages we would like to recieve
		createInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
		// callback function
		createInfo.pfnCallback = debugCallback; 
		// @TODO passed to the callback function. Add if we want to get a ptr to something from the static function.
		//createInfo.pUserData = //SOMETHING

		// create Debug Callback Ext
		{
			auto func = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(pvinstance, "vkCreateDebugReportCallbackEXT");
			if (func == nullptr)
			{
				throw std::runtime_error("failed to load vkCreateDebugReportCallbackEXT extension function");
			}
			
			PVVK_RUN(func(pvinstance, &createInfo, allocnullptr, &debugCallbackExt));
		}

	}
	
	void createSurface()
	{
		// we will let glfw handle our surface creation, but we can do this ourself 
		// more directly by loading in the functions from the instance which has the extensions
		// to create the surface. These extensions are incuded in the "required extesions for glfw"
		// we added a while back. See (https://vulkan-tutorial.com/Drawing_a_triangle/Presentation/Window_surface)
		// for more details

		// let glfw handle surface creation! yeah!
		PVVK_RUN(glfwCreateWindowSurface(pvinstance, pvwindow, allocnullptr, &surface));
	}
	void pickPhysicalDevice()
	{
		uint32_t deviceCount = 0;
		vkEnumeratePhysicalDevices(pvinstance, &deviceCount, nullptr);

		if (deviceCount == 0) 
		{
			throw std::runtime_error("Error: failed to find GPUs with Vulkan Support");
		}

		std::vector<VkPhysicalDevice> devices(deviceCount);
		vkEnumeratePhysicalDevices(pvinstance, &deviceCount, devices.data());

		struct ScoredDev { int score; VkPhysicalDevice device; std::string name; };
		std::vector<ScoredDev> candidates;
		for (const auto& dev : devices)
		{
			if (isDeviceSuitable(dev))
			{
				ScoredDev sd;
				sd.device = dev;
				sd.score = scoreDevice(dev);
				sd.name = GetDeviceName(dev);
				candidates.push_back(sd);
			}
		}


		if (candidates.size() == 0)
		{
			throw std::runtime_error("failed to find a suitable GPU!");
		}

		
		{// Get Highest Scoring Device, and set physical Device to it
			int indexOfHighest = -1;
			int highestScore = -1;
			for (int i = 0; i < candidates.size(); ++i)
			{
				if (candidates[i].score > highestScore)
				{
					highestScore = candidates[i].score;
					indexOfHighest = i;
				}
			}

			ScoredDev selectedDevice = candidates[indexOfHighest];
			physicalDevice = selectedDevice.device;
			
			if (PRINT_DEBUG_LOGS)
			{
				std::cout << "Selected Physical Device: " << selectedDevice.name << "\n";
				std::cout << "Available Physical Devices:\n";

				for (const auto& sd : candidates)
				{
					std::cout << "\t" << sd.name << "\n";
				}
				std::cout << std::endl;
			}
		}

		
		

	}


	void createLogicalDevice()
	{
		VkDeviceQueueCreateInfo queueCreateInfo = {};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = selectedQueueFamily.graphicsFamily;
		queueCreateInfo.queueCount = 1;
		queueCreateInfo.pQueuePriorities = &queuePriority;

		// default features are fine for now. If we do anything fancy we probably want to change this...
		VkPhysicalDeviceFeatures deviceFeatures = {}; 


		VkDeviceCreateInfo deviceCreateInfo = {};
		deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		deviceCreateInfo.pQueueCreateInfos = &queueCreateInfo;
		deviceCreateInfo.queueCreateInfoCount = 1;
		// Device Features Enable
		deviceCreateInfo.pEnabledFeatures = &deviceFeatures;
		// Device Extensions here, none for now
		deviceCreateInfo.enabledExtensionCount = 0;

		// Validation Layers
		if (enableValidationLayers)
		{
			deviceCreateInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
			deviceCreateInfo.ppEnabledLayerNames = validationLayers.data();
		}
		else
		{
			deviceCreateInfo.enabledLayerCount = 0;
		}

		// create the logical device
		PVVK_RUN(vkCreateDevice(physicalDevice, &deviceCreateInfo, allocnullptr, &device));

		// Get the graphics queu from the logical device. graphic queue is implicitly created.
		// queue index is 0 b/c we only have 1 graphicsQueue @TODO make this use multiple queues
		vkGetDeviceQueue(device, selectedQueueFamily.graphicsFamily, 0, &graphicsQueue);
	}
	bool isDeviceSuitable(VkPhysicalDevice device)
	{
		bool notSuitable = false;
		// Base Sevice Suitability checks
		VkPhysicalDeviceProperties deviceProperties;
		vkGetPhysicalDeviceProperties(device, &deviceProperties);

		// Device Features
		VkPhysicalDeviceFeatures deviceFeatures;
		vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

		// has a good Queue Family, select it
		selectedQueueFamily = findQueueFamilies(device, VK_QUEUE_GRAPHICS_BIT);
		notSuitable |= (selectedQueueFamily.isComplete() == false);

		return notSuitable == false;
	}
	std::string GetDeviceName(VkPhysicalDevice device)
	{
		// Base Sevice Suitability checks
		VkPhysicalDeviceProperties deviceProperties;
		vkGetPhysicalDeviceProperties(device, &deviceProperties);
		std::string name;
		name.append(deviceProperties.deviceName);
		return name;
	}
	int scoreDevice(VkPhysicalDevice device)
	{
		int scoreAccum = 0;

		// Base Sevice Suitability checks
		VkPhysicalDeviceProperties deviceProperties;
		vkGetPhysicalDeviceProperties(device, &deviceProperties);

		// Device Features
		VkPhysicalDeviceFeatures deviceFeatures;
		vkGetPhysicalDeviceFeatures(device, &deviceFeatures);
		
		// Discrete GPUs have a significant performance advantage
		if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) 
			scoreAccum += 10000;
		
		// Maximum possible size of textures affects graphics quality
		scoreAccum += deviceProperties.limits.maxPerStageResources;
		scoreAccum += deviceProperties.limits.maxVertexInputBindings;
		scoreAccum += deviceProperties.limits.maxImageDimension2D;

		return scoreAccum;
	}


	// Helper functions
	bool checkValidationLayerSupport()
	{
		// Get list of available Layers
		uint32_t layerCount = 0;
		vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
		std::vector<VkLayerProperties> availableLayers(layerCount);
		vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

		// Check to make sure all of our layers we are going to use are supported
		for (const char* layerName : validationLayers)
		{
			bool layerFound = false;
			for (const auto& layerProp : availableLayers)
			{
				if (strcmp(layerName, layerProp.layerName) == 0)
				{
					layerFound = true;
					break;
				}
			}

			if (!layerFound)
				return false;
		}

		return true;
	}
	bool checkExtensionSupport()
	{
		uint32_t extensionCount = 0;
		vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
		std::vector<VkExtensionProperties> availableExtensionProperties(extensionCount);
		vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, availableExtensionProperties.data());

		// Add GLFW required extensions to EXTENSIONS
		{
			std::vector<const char*> glfwRequiredExtensions = getExtensionsRequiredByGLFW();
			// Add any extensions not already present in EXTENSIONS
			for (const auto& reqExt : glfwRequiredExtensions)
			{
				bool requiredExtensionAlreadyListed = false;

				for (const auto& ext : extensions)
				{
					if (strcmp(ext, reqExt) == 0)
					{
						requiredExtensionAlreadyListed = true;
						break;
					}
				}

				if (requiredExtensionAlreadyListed == false)
				{
					extensions.push_back(reqExt);
				}
			}
		}

		for (const char* extName : extensions)
		{
			bool layerFound = false;
			for (const auto& extensionProp : availableExtensionProperties)
			{
				if (strcmp(extName, extensionProp.extensionName) == 0)
				{
					layerFound = true;
					break;
				}
			}
			if (!layerFound)
				return false;
		}
		return true;
	}
	std::vector<const char*> getExtensionsRequiredByGLFW()
	{
		uint32_t glfwExtensionCount = 0;
		const char** glfwExtensions;
		glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

		//									(begin, end)
		std::vector <const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

		if (enableValidationLayers)
		{
			extensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
		}

		return extensions;
	}
	QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device, VkQueueFlags qualifierFlags)
	{
		QueueFamilyIndices indicies = {};

		uint32_t queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
		std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

		// find first queueFamily which satisfies our needs
		{
			for (int i = 0; i < queueFamilies.size(); ++i)
			{
				// family has a some number of that queue
				if (queueFamilies[i].queueCount > 0 &&
				   (queueFamilies[i].queueFlags & qualifierFlags) == qualifierFlags)
				{
					indicies.graphicsFamily = i;
				}
				if (indicies.isComplete())
					break;
			}
		}

		return indicies;
	}

#pragma endregion

	void runLoop() 
	{
		while (glfwWindowShouldClose(pvwindow) == false)
		{
			glfwPollEvents();
		}
	}


#pragma region Cleanup
	void cleanup() {

		// destroy logical device
		vkDestroyDevice(device, allocnullptr);

		if(enableValidationLayers)
		{
			DestroyDebugReportCallbackEXT();
		}


		// vulkan cleanup
		{
			vkDestroySurfaceKHR(pvinstance, surface, allocnullptr);
			vkDestroyInstance(pvinstance, allocnullptr);
		}

		{
			// destroy GLFW window
			glfwDestroyWindow(pvwindow);
			pvwindow = nullptr;
			// close down glfw
			glfwTerminate();
		}
	}

	void DestroyDebugReportCallbackEXT()
	{
		{
			auto func = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(pvinstance, "vkDestroyDebugReportCallbackEXT");
			if (func == nullptr)
			{
				throw std::runtime_error("failed to load vkDestroyDebugReportCallbackEXT extension function in cleanup");
			}
			func(pvinstance, debugCallbackExt, allocnullptr);
		}
	}

#pragma endregion
};

#pragma region ErrorHandling

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
	VkDebugReportFlagsEXT flags, // indicate the type of message, VK_DEBUG_REPORT_...
	VkDebugReportObjectTypeEXT objType,
	uint64_t obj,
	size_t location,
	int32_t code,
	const char* layerPrefix,
	const char* msg,
	void* userData)
{

	std::cerr << "validation layer: " << msg << std::endl;

	return VK_FALSE;
}

#pragma endregion

int main() {
	PVWindow app;

	try {
		app.run();
	}
	catch (const std::runtime_error& e) {
		std::cerr << e.what() << std::endl;

		int input;
		std::cin >> input;
		return EXIT_FAILURE;
	}

	//int input;
	//std::cin >> input;
	return EXIT_SUCCESS;
}
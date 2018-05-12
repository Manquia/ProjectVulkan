#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <stdexcept>
#include <functional>
#include <cstdlib>
#include <string>
#include <vector>
#include <set>
#include <algorithm> // std::min,max

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
#define PV_VK_RUN(VK_OPERATION)																													\
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
		int presentFamily = -1;

		bool isComplete()
		{
			// We can do graphics && we can present
			return graphicsFamily >= 0 && presentFamily >= 0;
		}
	};

	struct SwapChainSupportDetails
	{
		VkSurfaceCapabilitiesKHR capabilities;
		std::vector<VkSurfaceFormatKHR> formats;
		std::vector<VkPresentModeKHR> presentModes;
	};

#pragma endregion


#pragma region Constants

	const uint32_t PV_WINDOW_WIDTH = 800;
	const uint32_t PV_WINDOW_HEIGHT = 600;
	const char* WINDOW_NAME = "GAM 400 Independent Study - Vulkan Renderer - Micah Rust";
	const char* APPLICATION_NAME = WINDOW_NAME;
	const char* ENGINE_NAME = APPLICATION_NAME;

	const bool PRINT_AVAILABLE_VULKAN_EXTENSIONS = true;
	const bool PRINT_DEBUG_LOGS = true;

	const VkQueueFlagBits PV_VK_QUEUE_FLAGS = VK_QUEUE_GRAPHICS_BIT;

	

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
	VkQueue presentQueue;
	VkSurfaceKHR surface;
	VkDebugReportCallbackEXT debugCallbackExt;

	VkSwapchainKHR swapChain;
	std::vector<VkImage> swapChainImages;
	VkFormat swapChainImageFormat;
	VkExtent2D swapChainExtent;
	std::vector<VkImageView> swapChainImageViews;

	QueueFamilyIndices selectedQueueFamily;
	float queuePriority = 1.0f;

	std::vector<const char*> validationLayers =
	{
		"VK_LAYER_LUNARG_standard_validation",
	};
	// GLFW required extensions added elsewhere
	std::vector<const char*> instanceExtensions =
	{
	};
	std::vector<const char*> deviceExtensions =
	{
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
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
		createSwapChain();
		createSwapChainImageViews();	
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
		if (checkInstanceExtensionSupport() == false)
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
			createInfo.enabledExtensionCount = static_cast<uint32_t>(instanceExtensions.size());
			createInfo.ppEnabledExtensionNames = instanceExtensions.data();

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
			PV_VK_RUN(vkCreateInstance(&createInfo, allocnullptr, &pvinstance));
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
			
			PV_VK_RUN(func(pvinstance, &createInfo, allocnullptr, &debugCallbackExt));
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
		PV_VK_RUN(glfwCreateWindowSurface(pvinstance, pvwindow, allocnullptr, &surface));
	}
	void pickPhysicalDevice()
	{
		uint32_t deviceCount = 0;
		vkEnumeratePhysicalDevices(pvinstance, &deviceCount, nullptr);

		if (deviceCount == 0) 
		{
			throw std::runtime_error("Error: failed to find GPUs with Vulkan Support");
		}

		std::vector<VkPhysicalDevice> physicalDevices(deviceCount);
		vkEnumeratePhysicalDevices(pvinstance, &deviceCount, physicalDevices.data());

		struct ScoredPhysicalDevice { int score; VkPhysicalDevice device; std::string name; };
		std::vector<ScoredPhysicalDevice> candidates;
		for (const auto& physDev : physicalDevices)
		{
			if (isDeviceSuitable(physDev))
			{
				ScoredPhysicalDevice spd;
				spd.device = physDev;
				spd.score = scoreDevice(physDev);
				spd.name = GetDeviceName(physDev);
				candidates.push_back(spd);
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

			ScoredPhysicalDevice selectedDevice = candidates[indexOfHighest];
			physicalDevice = selectedDevice.device;
			selectedQueueFamily = findQueueFamilies(physicalDevice, PV_VK_QUEUE_FLAGS);
			
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
		// Create Present AND Graphics Queue. Push onto std::vector and then create them...

		std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
		std::set<int> uniqueQueueFamiles = { selectedQueueFamily.graphicsFamily, selectedQueueFamily.presentFamily };

		for (int queueFamily : uniqueQueueFamiles)
		{
			VkDeviceQueueCreateInfo queueCreateInfo = {};

			queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueCreateInfo.queueFamilyIndex = queueFamily;
			queueCreateInfo.queueCount = 1;
			queueCreateInfo.pQueuePriorities = &queuePriority;

			queueCreateInfos.push_back(queueCreateInfo);
		}

		
		// default features are fine for now. If we do anything fancy we probably want to change this...
		VkPhysicalDeviceFeatures deviceFeatures = {}; 

		VkDeviceCreateInfo deviceCreateInfo = {};
		deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
		deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
		deviceCreateInfo.pEnabledFeatures = &deviceFeatures;// Device Features Enable

		deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.data();
		deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());

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
		PV_VK_RUN(vkCreateDevice(physicalDevice, &deviceCreateInfo, allocnullptr, &device));

		// Get the graphics queue from the logical device. graphic queue is implicitly created.
		// queue index is 0 b/c we only have 1 graphicsQueue @TODO make this use multiple graphics queues ?
		vkGetDeviceQueue(device, selectedQueueFamily.graphicsFamily, 0, &graphicsQueue);
		// Get the present queue from the logical device. @TODO make multiple present queues?
		vkGetDeviceQueue(device, selectedQueueFamily.presentFamily, 0, &presentQueue);
	}
	void createSwapChain()
	{
		SwapChainSupportDetails swapChainSupport = QuerySwapChainSupport(physicalDevice);

		// Make choices about the capabilities of our swap chain
		VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
		VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
		VkExtent2D resoulationExtent = chooseSwapExtent(swapChainSupport.capabilities);

		// how many images are in the swap chain?
		uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1; // 1 more than the min, for tripple buffering
		// if maxImageCount == 0, then there is not limit to max images in swap chain, so unless there is a limit we will just do +1 min.
		// This is effectivly doing std::min(imageCount, maxImageCount) when maxImageCount > 0
		if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount)
		{
			imageCount = swapChainSupport.capabilities.maxImageCount;
		}

		// swap chain create info
		VkSwapchainCreateInfoKHR createInfo = {};
		{
			createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
			createInfo.surface = surface;
			createInfo.minImageCount = imageCount;
			createInfo.imageFormat = surfaceFormat.format;
			createInfo.imageColorSpace = surfaceFormat.colorSpace;
			createInfo.imageExtent = resoulationExtent;
			createInfo.imageArrayLayers = 1; // used for steroscopic 3d applications, VR?
			createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT; // kinds of operations we intend to do in the swap chain
			// if we were to need to do something like post-processing we would use Vk_IMAGE_TRANSFER_DST_BIT
			// so that we could transfer the image to the swap chain after we have done our post-processing
			// in another image before the transfer.

			QueueFamilyIndices indices = findQueueFamilies(physicalDevice, PV_VK_QUEUE_FLAGS);
			uint32_t queueFamilyIndices[] = { (uint32_t)indices.graphicsFamily, (uint32_t)indices.presentFamily };

			// present and graphics queues are different, use concurrent, b/c reasons I hope to learn soon
			// @TODO @LEARN @MT
			if (indices.graphicsFamily != indices.presentFamily) {
				createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
				createInfo.queueFamilyIndexCount = 2;
				createInfo.pQueueFamilyIndices = queueFamilyIndices;
			}
			else  // present and graphics queue indexes are the same, we will just use exclusive, usually the case
			{
				createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
				createInfo.queueFamilyIndexCount = 0; // Optional
				createInfo.pQueueFamilyIndices = nullptr; // Optional
			}

			// use the current transformation of the swap chain. This is used for rotations to the swap chain like rotation 90,180,270 and mirrror/flip.
			createInfo.preTransform = swapChainSupport.capabilities.currentTransform;

			// @FUN @TODO. Used for blending with the other windows in the window system. Might be able to use 
			// to blend with background...?
			createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

			createInfo.presentMode = presentMode;
			// when clipped is true, we don't care about pixels that are obscured. for example b/c a window
			// is in front we don't need to read the pixels in the back to get predictible results
			// this will improve performance unless we want to do some fancy desktop stuff with Game Desktop HUD
			// while in-game. @FUN
			createInfo.clipped = VK_TRUE; 

			// let vulkan know we are replacing an old swap chain. This would happen
			// if we resize the window or something like that.
			// @TODO @RESOLUTION
			createInfo.oldSwapchain = VK_NULL_HANDLE; // swapChain
		}

		// create swap chain
		PV_VK_RUN(vkCreateSwapchainKHR(device, &createInfo, allocnullptr, &swapChain));

		{
			uint32_t swapChainImageCount; // may be different from imageCount above
			vkGetSwapchainImagesKHR(device, swapChain, &swapChainImageCount, nullptr);
			swapChainImages.resize(swapChainImageCount);
			vkGetSwapchainImagesKHR(device, swapChain, &swapChainImageCount, swapChainImages.data());
		}

		// Store some of the creation data for later
		{
			swapChainImageFormat = surfaceFormat.format;
			swapChainExtent = resoulationExtent;
		}
	}
	void createSwapChainImageViews()
	{
		swapChainImageViews.resize(swapChainImages.size());

		for (size_t i = 0; i < swapChainImages.size(); ++i)
		{
			VkImageViewCreateInfo createInfo = {};
			createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			createInfo.image = swapChainImages[i];

			createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			createInfo.format = swapChainImageFormat;

			createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
			
			createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			createInfo.subresourceRange.baseMipLevel = 0;
			createInfo.subresourceRange.levelCount = 1;
			createInfo.subresourceRange.baseArrayLayer = 0; // for sterioscopic rendering (VR)
			createInfo.subresourceRange.layerCount = 1;

			PV_VK_RUN(vkCreateImageView(device, &createInfo, allocnullptr, &swapChainImageViews[i]));
		}
	}
	void createGraphicsPipeline()
	{

	}
	// Helper functions
	bool isDeviceSuitable(VkPhysicalDevice device)
	{
		bool notSuitable = false;

		// Base Sevice Suitability checks, try and query the device
		{
			VkPhysicalDeviceProperties deviceProperties;
			vkGetPhysicalDeviceProperties(device, &deviceProperties);
		}
		
		// Device Features good for our purposes?
		{
			VkPhysicalDeviceFeatures deviceFeatures;
			vkGetPhysicalDeviceFeatures(device, &deviceFeatures);
		}

		// Device has a good Queue Family we can use for graphics
		{
			QueueFamilyIndices queueFamily = findQueueFamilies(device, PV_VK_QUEUE_FLAGS);
			notSuitable |= (queueFamily.isComplete() == false);
			if (notSuitable) return false;
		}

		// Does device support needed extensions?
		{
			notSuitable |= checkDeviceExtensionSupport(device) == false;
			if (notSuitable) return false;
		}

		// Swap chain support good?
		{
			SwapChainSupportDetails chainSupportDetails = QuerySwapChainSupport(device);
			notSuitable |= chainSupportDetails.formats.empty();
			notSuitable |= chainSupportDetails.presentModes.empty();
			if (notSuitable) return false;
		}

		return notSuitable == false;
	}

	// helpers for swap chain creation
	VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
	{
		// surface has 1 format, which is to say no requirements on the format.
		// we then will use our prefered VK_FORMAT_B8G8R8A8
		if (availableFormats.size() == 1 && availableFormats[0].format == VK_FORMAT_UNDEFINED) {
			return { VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
		}

		// there is a required format, lets see if our prefered is listed
		for (const auto& availableFormat : availableFormats) {
			if (availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
				return availableFormat;
			}
		}

		// prefered not given, lets just settle with the first (probably best) format given to us
		return availableFormats[0];
	}
	VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR> availablePresentModes) 
	{
		VkPresentModeKHR bestMode = VK_PRESENT_MODE_FIFO_KHR;

		for (const auto& availablePresentMode : availablePresentModes) {

			// Prefered: kinda like tripple buffer, except we will draw over the latest frame so we will
			// reduce latency. Take it if we its availble...
			if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
				bestMode = availablePresentMode;
				break;
			}
			// VSYNC
			// Most certainly avaiable on all Vulkan GPUs: VSYNC with fixed frame count, will make program wait
			else if (availablePresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR) {
				bestMode = availablePresentMode;
			}
		}

		return bestMode;
	}
	VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities)
	{
		// special number for width indicates we should pick a resolution that best matches the window within
		// minImageExtent and maxImageExtent
		if (capabilities.currentExtent.width == std::numeric_limits<uint32_t>::max())
		{
			VkExtent2D actualExtent = { PV_WINDOW_WIDTH, PV_WINDOW_HEIGHT };

			actualExtent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actualExtent.width));
			actualExtent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actualExtent.height));

			return actualExtent;
		}
		else // return the currentExtent of the surfaceCapabilities
		{
			return capabilities.currentExtent;
		}
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
	bool checkInstanceExtensionSupport()
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

				for (const auto& ext : instanceExtensions)
				{
					if (strcmp(ext, reqExt) == 0)
					{
						requiredExtensionAlreadyListed = true;
						break;
					}
				}

				if (requiredExtensionAlreadyListed == false)
				{
					instanceExtensions.push_back(reqExt);
				}
			}
		}

		for (const char* extName : instanceExtensions)
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
			for (uint32_t i = 0; i < queueFamilies.size(); ++i)
			{
				// family has present support
				VkBool32 presentSupport = false;
				vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
				if (queueFamilies[i].queueCount > 0 && presentSupport)
				{
					indicies.presentFamily = i;
				}
				
				// family has a some number of that queue
				if (queueFamilies[i].queueCount > 0 &&
				   (queueFamilies[i].queueFlags & qualifierFlags) == qualifierFlags)
				{
					indicies.graphicsFamily = i;
				}

				// if QueueFamily Index is good/complete, use it!
				if (indicies.isComplete())
					break;
			}
		}

		return indicies;
	}

	SwapChainSupportDetails QuerySwapChainSupport(VkPhysicalDevice device)
	{
		SwapChainSupportDetails details;

		// Get Capabilities
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

		// get surface formats
		uint32_t formatCount;
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);
		if (formatCount != 0)
		{
			details.formats.resize(formatCount);
			vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
		}

		// Get Present Modes
		uint32_t presentModeCount = 0;
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);
		if (presentModeCount != 0)
		{
			details.presentModes.resize(presentModeCount);
			vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
		}

		return details;
	}

	bool checkDeviceExtensionSupport(VkPhysicalDevice device)
	{
		uint32_t extensionCount;
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

		std::vector<VkExtensionProperties> availableExtensions(extensionCount);
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

		std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());
		

		for (uint32_t i = 0; i < extensionCount && requiredExtensions.empty() == false; ++i)
		{
			requiredExtensions.erase(availableExtensions[i].extensionName);
		}

		// we have found all of the required device extensions
		return requiredExtensions.empty();
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
	void cleanup() 
	{
		// vulkan cleanup
		{
			// cleanup anyu imageviews connected to the swap chain
			for (auto imageView : swapChainImageViews)
			{
				vkDestroyImageView(device, imageView, allocnullptr);
			}


			// destroy swap chain
			vkDestroySwapchainKHR(device, swapChain, nullptr);
			swapChainImages.clear();

			// destroy logical device
			vkDestroyDevice(device, allocnullptr);

			if (enableValidationLayers)
			{
				DestroyDebugReportCallbackEXT();
			}

			vkDestroySurfaceKHR(pvinstance, surface, allocnullptr);
			vkDestroyInstance(pvinstance, allocnullptr);
		}

		// GLFW Cleanup
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
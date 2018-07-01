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
#include <assert.h>

#include "Misc.hpp"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// glm includes
#define GLM_FORCE_RADIANS 
#include <glm/glm.hpp> // Linear Algebra
#include <glm/gtc/matrix_transform.hpp> // glm::perspective

#include "Vertex.h" // had include dependencies

#include <chrono>

const std::vector<Vertex> vertices = {
    {{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
    {{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
    {{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
    {{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}}
};
// mesh must be < 65535 verticies
// use uint32_t > 65535 verticies
// Will require that our binding get changed
const std::vector<uint16_t> indices = {
	0, 1, 2, 2, 3, 0
};

struct UniformBufferObject
{
	glm::mat4 model;
	glm::mat4 view;
	glm::mat4 proj;
};
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

#ifndef defer
struct defer_dummy {};
template <class F> struct deferrer { F f; ~deferrer() { f(); } };
template <class F> deferrer<F> operator*(defer_dummy, F f) { return { f }; }
#define DEFER_(LINE) zz_defer##LINE
#define DEFER(LINE) DEFER_(LINE)
#define defer auto DEFER(__LINE__) = defer_dummy{} *[&]()
#endif // defer

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
	GLFWwindow *pvWindow;
	int pvWindowWidth = 800;  // starting values
	int pvWindowHeight = 600; // starting values

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
	std::vector<VkFramebuffer> swapChainFramebuffers;

	QueueFamilyIndices selectedQueueFamily;
	float queuePriority = 1.0f;

	VkRenderPass renderPass;
	VkPipelineLayout pipelineLayout;
	VkPipeline graphicsPipeline;


	VkDescriptorPool descriptorPool;
	VkDescriptorSet  descriptorSet;
	VkDescriptorSetLayout descriptorSetLayout;

	// @TODO make this work with lots of models in loading or somethingVkImage textureImage;
	VkImageView textureImageView;
	VkImage textureImage;
	VkDeviceMemory textureImageMemory;

	VkBuffer vertexBuffer;
	VkDeviceMemory vertexBufferMemory;

	VkBuffer indexBuffer;
	VkDeviceMemory indexBufferMemory;

	VkBuffer uniformBuffer;
	VkDeviceMemory uniformBufferMemory;

	VkCommandPool commandPool;
	std::vector<VkCommandBuffer> commandBuffers;

	VkSemaphore imageAvailableSemaphore;
	VkSemaphore renderFinishedSemaphore;

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
		
		pvWindow = nullptr;
		// initialize GLFW
		glfwInit();
		// create without OpenGL context
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		// disable window resizing
		glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
		//create the GLFW window
		pvWindow = glfwCreateWindow(pvWindowWidth, pvWindowHeight, WINDOW_NAME, nullptr, nullptr);
		
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
		createRenderPass();
		createDescriptorSetLayout();
		createGraphicsPipeline();
		createFramebuffers();
		createCommandPool();

		createTextureImage();
		createTextureImageView();

		// make buffers
		createVertexBuffer();
		createIndexBuffer();
		createUniformBuffer();

		createDescriptorPool();
		createDescriptorSet();

		createCommandBuffers();
		createSemaphores();
	}
	// requires that the logical device be initialized
	// call for things like window resize
	void recreateSwapChain()
	{
		// wait till device is idel before changing stuff
		// @Multi-threaded, this may require a mutex or jobs system flag
		// at some point to work with multiple threads
		vkDeviceWaitIdle(device);

		// clean up old swap chain
		cleanupSwapChain();

		createSwapChain();
		createSwapChainImageViews();
		createRenderPass();
		createGraphicsPipeline();
		createFramebuffers();
		createCommandBuffers();
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
		PV_VK_RUN(glfwCreateWindowSurface(pvinstance, pvWindow, allocnullptr, &surface));
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
			swapChainImageViews[i] = createImageView(swapChainImages[i], swapChainImageFormat);
		}
	}
	void createRenderPass()
	{
		VkAttachmentDescription colorAttachment = {};
		colorAttachment.format = swapChainImageFormat;
		colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT; // used for multi-sampling

		// apply to Color and depth data
		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		// apply to stencil data ONLY
		colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		// don't care about initial state, going to clear before drawing
		colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		// final layout should be ready for the swap chain to present
		colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		// subpasses reference Color attachments by index
		// If we wanted to  add another color to our fragment shader
		// we would add another colorAttachementReference to the subpass.
		VkAttachmentReference colorAttachmentRef = {};
		colorAttachmentRef.attachment = 0;
		colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		// a render pass can have 1 or more sub passes. These can be used for
		// many different processes such a post-processing which has mulitple sub-passes.
		VkSubpassDescription subpass = {};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &colorAttachmentRef;

		// Makes the render pass wait for VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BUT
		// to be finished.. We need to wait for the swap chain to finish reading from the image
		// before we can access it.
		VkSubpassDependency dependency = {};
		dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		dependency.dstSubpass = 0;
		dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

		VkRenderPassCreateInfo renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.attachmentCount = 1;
		renderPassInfo.pAttachments = &colorAttachment;
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &subpass;
		renderPassInfo.dependencyCount = 1;
		renderPassInfo.pDependencies = &dependency;


		PV_VK_RUN(vkCreateRenderPass(device, &renderPassInfo, allocnullptr, &renderPass));
	}

	void createDescriptorSetLayout()
	{
		VkDescriptorSetLayoutBinding uboLayoutBinding = {};
		uboLayoutBinding.binding = 0;
		uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		uboLayoutBinding.descriptorCount = 1;
		uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
		uboLayoutBinding.pImmutableSamplers = nullptr; // @TODO image samplers

		VkDescriptorSetLayoutCreateInfo layoutInfo = {};
		layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutInfo.bindingCount = 1;
		layoutInfo.pBindings = &uboLayoutBinding;

		PV_VK_RUN(vkCreateDescriptorSetLayout(device, &layoutInfo, allocnullptr, &descriptorSetLayout));
	}

	void createGraphicsPipeline()
	{
		auto vertShaderCode = readBinaryFile("../SPV/vert.spv"); //@Speed copy
		auto fragShaderCode = readBinaryFile("../SPV/frag.spv"); //@Speed copy

		VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
		VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);

		VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
		vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
		vertShaderStageInfo.module = vertShaderModule;
		vertShaderStageInfo.pName = "main";
		//vertShaderStageInfo.pSpecializationInfo; // used for setting constants

		VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
		fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		fragShaderStageInfo.module = fragShaderModule;
		fragShaderStageInfo.pName = "main";
		//vertShaderStageInfo.pSpecializationInfo; // used for setting constants 

		VkPipelineShaderStageCreateInfo shaderStages[] =
		{
			vertShaderStageInfo, fragShaderStageInfo
		};

		// Get Bindings (shader data layout)
		auto bindingDescription = Vertex::getBindingDescription();
		auto attributeDescription = Vertex::getAttributeDescriptions();
		// Setup vertex Input
		VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
		vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInputInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(bindingDescription.size());
		vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescription.size());
		vertexInputInfo.pVertexBindingDescriptions = bindingDescription.data(); // binding 0 is per tri, binding 1 is per instance
		vertexInputInfo.pVertexAttributeDescriptions = attributeDescription.data();

		// setup Input Assembly (Draw Mode)
		VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
		inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		inputAssembly.primitiveRestartEnable = VK_FALSE;

		VkViewport viewport = {}; // viewport extents
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = static_cast<float>(swapChainExtent.width);
		viewport.height = static_cast<float>(swapChainExtent.height);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		VkRect2D scissor = {}; // what to draw in the viewport
		scissor.offset = { 0,0 };
		scissor.extent = swapChainExtent;

		VkPipelineViewportStateCreateInfo viewportState = {};
		viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportState.viewportCount = 1;
		viewportState.pViewports = &viewport;
		viewportState.scissorCount = 1;
		viewportState.pScissors = &scissor;
			
		VkPipelineRasterizationStateCreateInfo rasterizer = {};
		rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizer.depthClampEnable = VK_FALSE;			// T: clamps fragments to near/far plane. F: Clips fragments beyond plane
		rasterizer.rasterizerDiscardEnable = VK_FALSE;	// T: disables output to framebuffer. F: output goes to framebuffer
		rasterizer.polygonMode = VK_POLYGON_MODE_FILL; // how fragments are generated from geometry
		rasterizer.lineWidth = 1.0f; // thickness of lines in terms of fragment count, >1 requires wideLines GPU Feature
		rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
		rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE; // vertex order

		// can be used for shadow mapping
		rasterizer.depthBiasEnable = VK_FALSE;
		rasterizer.depthBiasConstantFactor = 0.0f;	// optional
		rasterizer.depthBiasClamp = 0.0f;			// optional
		rasterizer.depthBiasSlopeFactor = 0.0f;		// optional

		VkPipelineMultisampleStateCreateInfo multisampling = {};
		multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampling.sampleShadingEnable = VK_FALSE;
		multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
		multisampling.minSampleShading = 1.0f;			// Optional
		multisampling.pSampleMask = nullptr;			// Optional
		multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
		multisampling.alphaToOneEnable = VK_FALSE;		// Optional

		// TODO VkPipelineDepthStencilStateCreateInfo = {};
		// for 3D depth testing...?

		const bool blendEnabled = false;
		VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
		if (blendEnabled)
		{
			colorBlendAttachment.blendEnable = VK_TRUE;
			colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA; // alpha channel determins blending
			colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
			colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
			colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
			colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
			colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
		}
		else
		{
			colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
			colorBlendAttachment.blendEnable = VK_FALSE; // disable colorBlend just makes finalColor = newColor
			colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;		// Optional
			colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;	// Optional
			colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;				// Optional
			colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;		// Optional
			colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;	// Optional
			colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;				// Optional
		}

		VkPipelineColorBlendStateCreateInfo colorBlending = {};
		colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		if (blendEnabled)
		{
			colorBlending.logicOpEnable = VK_TRUE;
			colorBlending.logicOp = VK_LOGIC_OP_COPY; //@BUG? not sure if this should be something else...
		}
		else
		{
			colorBlending.logicOpEnable = VK_FALSE;
			colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
		}
		colorBlending.attachmentCount = 1;
		colorBlending.pAttachments = &colorBlendAttachment;
		colorBlending.blendConstants[0] = 0.0f; // Optional
		colorBlending.blendConstants[1] = 0.0f; // Optional
		colorBlending.blendConstants[2] = 0.0f; // Optional
		colorBlending.blendConstants[3] = 0.0f; // Optional

		// Dynamic state can be changed without recreating the graphic Pipeline
		VkDynamicState dynamicStates[] =
		{
			VK_DYNAMIC_STATE_VIEWPORT,
			VK_DYNAMIC_STATE_LINE_WIDTH
		};

		VkPipelineDynamicStateCreateInfo dynamicState = {};
		dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicState.dynamicStateCount = 2;
		dynamicState.pDynamicStates = dynamicStates;



		// make an empty pipeline layout for now. This will later have things like worldViewMatrix
		// and other constants which are passed on a per graphics pipeline basis
		VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		// Uniform/Sampler bindings
		pipelineLayoutInfo.setLayoutCount = 1;
		pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;
		// Constant values bindings
		pipelineLayoutInfo.pushConstantRangeCount = 0; // Optional
		pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optional

		PV_VK_RUN(vkCreatePipelineLayout(device, &pipelineLayoutInfo, allocnullptr, &pipelineLayout));

		// Create Grpahics Pipeline
		VkGraphicsPipelineCreateInfo pipelineInfo = {};
		pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;

		// vertext and fragment stages
		pipelineInfo.stageCount = 2;
		pipelineInfo.pStages = shaderStages;

		// fixed function state
		pipelineInfo.pVertexInputState = &vertexInputInfo;
		pipelineInfo.pInputAssemblyState = &inputAssembly;
		pipelineInfo.pViewportState = &viewportState;
		pipelineInfo.pRasterizationState = &rasterizer;
		pipelineInfo.pMultisampleState = &multisampling;
		pipelineInfo.pDepthStencilState = nullptr;		// Optional
		pipelineInfo.pColorBlendState = &colorBlending;
		pipelineInfo.pDynamicState = nullptr;			// Optional
		
		// pipeline layout
		pipelineInfo.layout = pipelineLayout;
		// render pass and index of subpass
		pipelineInfo.renderPass = renderPass;
		pipelineInfo.subpass = 0;
		// we can inheret from a simular base pipeline to make the creation faster in some cases.
		// Probably will use this when we get mulitple graphics pipelines and want to modify/resue one...
		// only used if VK_PIPELINE_CREATE_DERIVATIVE_BIT flag is set in flags
		pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
		pipelineInfo.basePipelineIndex = -1; // Optional

		// @SPEED PipelineCache
		vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, allocnullptr, &graphicsPipeline);

		vkDestroyShaderModule(device, vertShaderModule, allocnullptr);
		vkDestroyShaderModule(device, fragShaderModule, allocnullptr);
	}
	void createFramebuffers()
	{
		// we want a frame buffer per swap chain image view
		swapChainFramebuffers.resize(swapChainImageViews.size());

		// make a framebuffer per swapChainImage/swapChainImageView
		for (size_t i = 0; i < swapChainImageViews.size(); i++) {
			VkImageView attachments[] = {
				swapChainImageViews[i]
			};

			VkFramebufferCreateInfo framebufferInfo = {};
			framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebufferInfo.renderPass = renderPass;
			framebufferInfo.attachmentCount = 1;
			// image views into the frame buffer. Only 1 for now
			framebufferInfo.pAttachments = attachments;
			framebufferInfo.width = swapChainExtent.width;
			framebufferInfo.height = swapChainExtent.height;
			framebufferInfo.layers = 1;

			PV_VK_RUN(vkCreateFramebuffer(device, &framebufferInfo, nullptr, &swapChainFramebuffers[i]));
		}

	}
	void createCommandPool()
	{
		QueueFamilyIndices queueFamilyIndicies = findQueueFamilies(physicalDevice, PV_VK_QUEUE_FLAGS);

		VkCommandPoolCreateInfo poolInfo = {};
		poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		poolInfo.queueFamilyIndex = queueFamilyIndicies.graphicsFamily;
		poolInfo.flags = 0; // Optional

		PV_VK_RUN(vkCreateCommandPool(device, &poolInfo, allocnullptr, &commandPool));
	}

	// uses command pool
	void createTextureImage() 
	{
		const char* lunaPath =   "../textures/LunaTooClose.jpg";
		const char* statuePath = "../textures/statue512";

		// Load textures from file
		int texWidth, texHeight, texChannels;
		stbi_uc* pixels = stbi_load(lunaPath, &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
		// RGBA channels are 255,255,255,255 4 bytes each
		VkDeviceSize imageSize = texWidth * texHeight * 4;

		if (!pixels)
		{
			throw std::runtime_error("failed to load texture image!");
		}

		// @SPEED @POOL
		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;

		createBuffer(
			imageSize,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			stagingBuffer,
			stagingBufferMemory);

		// staging buffer now has image
		void* data;
		vkMapMemory(device, stagingBufferMemory, 0, imageSize, 0, &data);
		memcpy(data, pixels, static_cast<size_t>(imageSize));
		vkUnmapMemory(device, stagingBufferMemory);

		// free loaded textures from memory
		stbi_image_free(pixels);

		createImage(texWidth, texHeight, 1, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, textureImage, textureImageMemory);

		// transfer from staging buffer into textureImageMemory
		{
			// Make image able to recieve staging buffer data
			transitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
			// transfer staging buffer data
			copyBufferToImage(stagingBuffer, textureImage, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));
			// mek image able to be read from shaders
			transitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		}
		
		vkDestroyBuffer(device, stagingBuffer, allocnullptr);
		vkFreeMemory(device, stagingBufferMemory, allocnullptr);

	}
	void transitionImageLayout(VkImage image, VkFormat fomat, VkImageLayout oldLayout, VkImageLayout newLayout)
	{
		VkCommandBuffer singleTimeCommandBuffer = beginSingleTimeCommands();

		VkImageMemoryBarrier barrier = {};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.oldLayout = oldLayout;
		barrier.newLayout = newLayout;
		
		// Used to change queue family ownership. Set to Ignored
		// for our purposes of layout transition
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

		// image and its layout
		barrier.image = image;
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		barrier.subresourceRange.baseMipLevel = 0;
		barrier.subresourceRange.levelCount = 1;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = 1;
		
		VkPipelineStageFlags sourceStage = 0;
		VkPipelineStageFlags destinationStage = 0;

		// Undef -> Dst
		if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) 
		{
			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

			sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		}
		// Dst -> Shader
		else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) 
		{
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		}
		else 
		{
			throw std::invalid_argument("unsupported layout transition!");
		}


		vkCmdPipelineBarrier(
			singleTimeCommandBuffer,
			sourceStage, destinationStage,
			0,
			0, nullptr,
			0, nullptr,
			1, &barrier
		);

		endSingleTimeCommands(singleTimeCommandBuffer);
	}

	// Assumes that image in in VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
	void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height)
	{
		VkCommandBuffer singleTimeCommandBuffer = beginSingleTimeCommands();

		VkBufferImageCopy region = {};
		region.bufferOffset = 0;
		region.bufferRowLength = 0;
		region.bufferImageHeight = 0;

		region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		region.imageSubresource.mipLevel = 0; // @MIP
		region.imageSubresource.baseArrayLayer = 0;
		region.imageSubresource.layerCount = 1;

		region.imageOffset = { 0,0,0 };
		region.imageExtent = { width, height, 1 };

		vkCmdCopyBufferToImage(singleTimeCommandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

		endSingleTimeCommands(singleTimeCommandBuffer);
	}
	

	void createTextureImageView()
	{
		textureImageView = createImageView(textureImage, VK_FORMAT_R8G8B8A8_UNORM);
	}

	// Assumes 2D and No mip map details
	VkImageView createImageView(VkImage image, VkFormat format)
	{
		VkImageViewCreateInfo viewInfo = {};
		viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewInfo.image = image; // @PARAM
		viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewInfo.format = format;
		viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		viewInfo.subresourceRange.baseMipLevel = 0; // @MIP
		viewInfo.subresourceRange.levelCount = 1;
		viewInfo.subresourceRange.baseArrayLayer = 0;
		viewInfo.subresourceRange.layerCount = 1;

		viewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		viewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		viewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		viewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

		VkImageView imageView;

		PV_VK_RUN(vkCreateImageView(device, &viewInfo, allocnullptr, &imageView));

		return imageView;
	}

	void createImage(uint32_t width, uint32_t height, uint32_t depth, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory)
	{
		VkImageCreateInfo imageInfo = {};
		imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageInfo.imageType = VK_IMAGE_TYPE_2D;
		imageInfo.extent.width = width;
		imageInfo.extent.height = height;
		imageInfo.extent.depth = depth;
		imageInfo.mipLevels = 1; // @TODO MipMaps
		imageInfo.arrayLayers = 1;

		imageInfo.format = VK_FORMAT_R8G8B8A8_UNORM; // Not all formats are supported, @ROBUSTNESS
		imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;  // @Investigate
		imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED; // will discard texels
		imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE; // only used by 1 queue family: graphics
		imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		imageInfo.flags = 0; // Optional

		PV_VK_RUN(vkCreateImage(device, &imageInfo, allocnullptr, &this->textureImage));

		// Allocate Memory for Image
		VkMemoryRequirements memRequirements;
		vkGetImageMemoryRequirements(device, this->textureImage, &memRequirements);

		VkMemoryAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memRequirements.size;
		allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		PV_VK_RUN(vkAllocateMemory(device, &allocInfo, allocnullptr, &textureImageMemory));

		// bind image to a block of memory (Set texture object's data ptr)
		vkBindImageMemory(device, textureImage, textureImageMemory, 0);
	}

	void createVertexBuffer()
	{
		VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

		// @SPEED treated as a temporary, this may we wasteful on startup
		// so we could pool these and use them over and over again in some
		// cases
		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;
		createBuffer(
			bufferSize,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			stagingBuffer, stagingBufferMemory);

		void* gpuData;
		vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &gpuData);
		memcpy(gpuData, vertices.data(), static_cast<size_t>(bufferSize));
		vkUnmapMemory(device, stagingBufferMemory);


		createBuffer(
			bufferSize,
			VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			vertexBuffer,
			vertexBufferMemory);

		copyBuffer(stagingBuffer, vertexBuffer, bufferSize);

		// @Speed treated as temporary, this may be wasteful on startup
		vkDestroyBuffer(device, stagingBuffer, allocnullptr);
		vkFreeMemory(device, stagingBufferMemory, allocnullptr);
	}
	void createIndexBuffer()
	{
		VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();

		// @SPEED treated as a temporary, this may we wasteful on startup
		// so we could pool these and use them over and over again in some
		// cases
		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;
		createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

		void* data;
		vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
		memcpy(data, indices.data(), (size_t)bufferSize);
		vkUnmapMemory(device, stagingBufferMemory);

		createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, indexBuffer, indexBufferMemory);

		copyBuffer(stagingBuffer, indexBuffer, bufferSize);

		// @Speed treated as temporary, this may be wasteful on startup
		vkDestroyBuffer(device, stagingBuffer, nullptr);
		vkFreeMemory(device, stagingBufferMemory, nullptr);
	}
	
	void createUniformBuffer()
	{
		VkDeviceSize bufferSize = sizeof(UniformBufferObject);
		createBuffer(
			bufferSize,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			uniformBuffer,
			uniformBufferMemory);
	}
	void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemeory)
	{
		VkBufferCreateInfo bufferInfo = {};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = size;
		bufferInfo.usage = usage;
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		PV_VK_RUN(vkCreateBuffer(device, &bufferInfo, allocnullptr, &buffer));

		VkMemoryRequirements memRequirements;
		vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

		// The number of operations we can allocate memory on the GPU is very
		// limited (<4k count). This is why we may need to use an allocator.
		// Open Src Helper: https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator
		VkMemoryAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memRequirements.size;
		allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

		PV_VK_RUN(vkAllocateMemory(device, &allocInfo, allocnullptr, &bufferMemeory));

		vkBindBufferMemory(device, buffer, bufferMemeory, 0);
	}

	// Buffer copy requires that we submit the command to copy through a command queue
	void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
	{
		VkCommandBuffer singleUseCommandBuffer = beginSingleTimeCommands();

		VkBufferCopy copyRegion = {};
		copyRegion.srcOffset = 0;
		copyRegion.dstOffset = 0;
		copyRegion.size = size;
		vkCmdCopyBuffer(singleUseCommandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

		endSingleTimeCommands(singleUseCommandBuffer);
	}

	void createDescriptorPool()
	{
		const VkDescriptorPoolSize poolSizes[] = 
		{ 
			// Type , Count
			{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1},
		};
		
		VkDescriptorPoolCreateInfo poolInfo = {};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.poolSizeCount = sizeof(poolSizes[0])/sizeof(poolSizes);
		poolInfo.pPoolSizes = poolSizes;
		poolInfo.maxSets = 1; // @DESCRIPTORPOOLSIZE

		PV_VK_RUN(vkCreateDescriptorPool(device, &poolInfo, allocnullptr, &descriptorPool));
	}
	void createDescriptorSet()
	{
		VkDescriptorSetLayout layouts[] = { descriptorSetLayout };
		VkDescriptorSetAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = descriptorPool;
		allocInfo.descriptorSetCount = 1;
		allocInfo.pSetLayouts = layouts;

		PV_VK_RUN(vkAllocateDescriptorSets(device, &allocInfo, &descriptorSet));

		VkDescriptorBufferInfo bufferInfo = {};
		bufferInfo.buffer = uniformBuffer;
		bufferInfo.offset = 0;
		bufferInfo.range = sizeof(UniformBufferObject);

		VkWriteDescriptorSet descWrite = {};
		descWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descWrite.dstSet = descriptorSet;
		descWrite.dstBinding = 0;
		descWrite.dstArrayElement = 0;

		descWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descWrite.descriptorCount = 1;

		descWrite.pBufferInfo = &bufferInfo;
		descWrite.pImageInfo = nullptr; // Optional
		descWrite.pTexelBufferView = nullptr; // Optional
		vkUpdateDescriptorSets(device, 1, &descWrite, 0, nullptr);
	}

	void createCommandBuffers()
	{
		commandBuffers.resize(swapChainFramebuffers.size());

		VkCommandBufferAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool = commandPool;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY; // secondary can be called from primary CBs
		allocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());

		PV_VK_RUN(vkAllocateCommandBuffers(device, &allocInfo, commandBuffers.data()));

		// @FUN, make opacity less than 1.0f and see what happens... Blur effect?
		const VkClearValue clearColor = { 0.07f, 0.08f, 0.12f, 1.0f };

		for (size_t i = 0; i < commandBuffers.size(); ++i)
		{
			VkCommandBufferBeginInfo beginInfo = {};
			beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
			beginInfo.pInheritanceInfo = nullptr; // Optional, for secondary command buffers
			
			PV_VK_RUN(vkBeginCommandBuffer(commandBuffers[i], &beginInfo));

			VkRenderPassBeginInfo renderPassInfo = {};
			renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			// setup frame buffer data
			renderPassInfo.renderPass = renderPass;
			renderPassInfo.framebuffer = swapChainFramebuffers[i];
			renderPassInfo.renderArea.offset = {0,0};
			renderPassInfo.renderArea.extent = swapChainExtent; // @TODO, make it so that we dont' need to remake all our command buffers on resize
			// Set Clear Color
			renderPassInfo.clearValueCount = 1;
			renderPassInfo.pClearValues = &clearColor;

			// begin render pass!
			vkCmdBeginRenderPass(commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

			// bind the graphics pipeline to the command buffer!
			vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

			const VkBuffer vertexbuffers[] = { vertexBuffer };
			const VkDeviceSize offsets[] = { 0 };
			const uint32_t vertexBufferCount = (sizeof(vertexbuffers) / sizeof(vertexbuffers[0]));
			size_t bindingCounter = 0;
			vkCmdBindVertexBuffers(commandBuffers[i], bindingCounter++, vertexBufferCount, vertexbuffers, offsets);

			// VK_INDEX_TYPE_UINT16 should be a field of the warpper since we don't need it for
			// models which are less than 65000 verticies which should be most things, @TODO @Speed
			vkCmdBindIndexBuffer(commandBuffers[i], indexBuffer, 0, VK_INDEX_TYPE_UINT16);

			vkCmdBindDescriptorSets(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);

			vkCmdDrawIndexed(commandBuffers[i], static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);

			// end the render pass
			vkCmdEndRenderPass(commandBuffers[i]);
			// end the command buffer, hopefully everythign worked...
			PV_VK_RUN(vkEndCommandBuffer(commandBuffers[i]));
		}


	}
	void createSemaphores()
	{
		VkSemaphoreCreateInfo semaphoreInfo = {};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		PV_VK_RUN(vkCreateSemaphore(device, &semaphoreInfo, allocnullptr, &imageAvailableSemaphore));
		PV_VK_RUN(vkCreateSemaphore(device, &semaphoreInfo, allocnullptr, &renderFinishedSemaphore));
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

	VkCommandBuffer beginSingleTimeCommands()
	{
		VkCommandBufferAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandPool = commandPool;
		allocInfo.commandBufferCount = 1;

		VkCommandBuffer commandBuffer;
		vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		vkBeginCommandBuffer(commandBuffer, &beginInfo);

		return commandBuffer;
	}
	// function is blocking to wait idle on the Graphics Queue
	// @SPEED This sort of thing should probably get its own queues...?
	// so that it doesn't block for so long OR we make a proper job system.
	void endSingleTimeCommands(VkCommandBuffer commandBuffer)
	{
		vkEndCommandBuffer(commandBuffer);

		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffer;

		vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
		vkQueueWaitIdle(graphicsQueue);

		vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
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
			VkExtent2D actualExtent = { static_cast<uint32_t>(pvWindowWidth), static_cast<uint32_t>(pvWindowHeight) };

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

	VkShaderModule createShaderModule(const std::vector<char>& byteCode)
	{
		VkShaderModuleCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.codeSize = byteCode.size();
		createInfo.pCode = reinterpret_cast<const uint32_t*>(byteCode.data());

		VkShaderModule shaderModule;
		PV_VK_RUN(vkCreateShaderModule(device, &createInfo, allocnullptr, &shaderModule));
		return shaderModule;
	}

	uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
	{
		VkPhysicalDeviceMemoryProperties physicalMemProperties;
		vkGetPhysicalDeviceMemoryProperties(physicalDevice, &physicalMemProperties);

		for (uint32_t i = 0; i < physicalMemProperties.memoryTypeCount; ++i)
		{
			if (
				typeFilter & (1 << i) &&
				(physicalMemProperties.memoryTypes[i].propertyFlags & properties) == properties
			   )
			{
				return i;
			}
		}

		throw std::runtime_error("failed to find suitable memory type on selected GPU");


		return -1U;
	}
#pragma endregion

	void runLoop() 
	{
		while (glfwWindowShouldClose(pvWindow) == false)
		{

			glfwPollEvents();
			glfwGetWindowSize(this->pvWindow, &pvWindowWidth, &pvWindowHeight);
			updateUniformBuffer();
			drawFrame();
		}

		// wait for the device to finish so that we can clean it up properly!
		vkDeviceWaitIdle(device);
	}

	const float fieldOfView = 45.0f;
	void updateUniformBuffer()
	{
		// @TODO make this into a more robust time tracking system. @TODO @ROBUST
		static auto firstFrameOfProgram = std::chrono::high_resolution_clock::now();
		auto currentTime = std::chrono::high_resolution_clock::now();
		float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - firstFrameOfProgram).count();

		// create Update UniformBufferObject
		UniformBufferObject ubo = {};

		ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		ubo.proj = glm::perspective(glm::radians(fieldOfView), swapChainExtent.width / (float)swapChainExtent.height, 0.1f, 100.0f);
		ubo.proj[1][1] *= -1.0f; // Invert Y coordinate for Vuklan, glm was made for OpenGL orginally

		// move ubo into GPUs
		void* gpuDataPtr;
		vkMapMemory(device, uniformBufferMemory, 0, sizeof(ubo), 0, &gpuDataPtr);
		memcpy(gpuDataPtr, &ubo, sizeof(ubo));
		vkUnmapMemory(device, uniformBufferMemory);

	}

	void drawFrame()
	{
		// @ TODO, UPDATE THE PROGRAM/GAME the rest of the stuff goes before here!

		// @TODO GAME!!
		// @TODO GAME!!
		// @TODO GAME!!


		// Should do this once everything else is setup in our world/graphics pipeline
		vkQueueWaitIdle(presentQueue);

		// STEP 1
		// aquire an image from the swap chain
		uint32_t imageIndex = -1;

		VkResult res;
		do
		{
			// using maxInt64 for timeout disables timeout... @SPEED use a fence?? May not be a problem b/c
			// we are tripple buffered so there should alwasy be a frame ready to be grabbed and rendered too
			res = vkAcquireNextImageKHR(device, swapChain, std::numeric_limits<uint64_t>::max(), imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);

			if (VK_ERROR_OUT_OF_DATE_KHR == res)
			{
				recreateSwapChain();
				continue;
			}

			// suboptimal and success are both viable success states
			if (VK_SUBOPTIMAL_KHR != res && VK_SUCCESS != res)
			{
				throw std::runtime_error("failed to acquire swap chain image!");
			}
		} while (false);


		// STEP 2
		// execute the command buffers with an image attachment in the framebuffer
		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

		VkSemaphore waitSemaphores[] = { imageAvailableSemaphore };
		VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

		// wait for semaphore to execute!
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = waitSemaphores;
		submitInfo.pWaitDstStageMask = waitStages;

		// command buffers to execute
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffers[imageIndex];

		// Once finished with command buffer, trigger this semaphore
		VkSemaphore signalSemaphores[] = { renderFinishedSemaphore };
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = signalSemaphores;

		PV_VK_RUN(vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE));

		// STEP 3
		// return the image to the swap chian for presentation
		VkSwapchainKHR swapChains[] = { swapChain };

		VkPresentInfoKHR presentInfo = {};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = signalSemaphores; // end of rending/finishing all the command buffers
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = swapChains;
		presentInfo.pImageIndices = &imageIndex;
		presentInfo.pResults = nullptr; // Optional, array of results for each swap chain we present into


		// Present the frame!
		res = vkQueuePresentKHR(presentQueue, &presentInfo);

		// if our swapChain is out of date OR suboptimal, recreate the swap chain
		if (VK_ERROR_OUT_OF_DATE_KHR == res || VK_SUBOPTIMAL_KHR == res)
		{
			recreateSwapChain();
		}
		else if (VK_SUCCESS != res)
		{
			throw std::runtime_error("failed to present swap chain image. Code: " + std::to_string(res));
		}
	}

#pragma region Cleanup
	void cleanup() 
	{
		// vulkan cleanup
		{
			// cleanup all resources related to the swap chain
			cleanupSwapChain();

			vkDestroyDescriptorPool(device, descriptorPool, allocnullptr);

			// free buffers
			{
				vkDestroyImage(device, textureImage, allocnullptr);
				vkFreeMemory(device, textureImageMemory, allocnullptr);

				vkDestroyDescriptorSetLayout(device, descriptorSetLayout, allocnullptr);
				vkDestroyBuffer(device, uniformBuffer, allocnullptr);
				vkFreeMemory(device, uniformBufferMemory, allocnullptr);

				vkDestroyBuffer(device, indexBuffer, allocnullptr);
				vkFreeMemory(device, indexBufferMemory, allocnullptr);
				
				vkDestroyBuffer(device, vertexBuffer, allocnullptr);
				vkFreeMemory(device, vertexBufferMemory, allocnullptr);
			}

			// clean up semaphores
			vkDestroySemaphore(device, imageAvailableSemaphore, allocnullptr);
			vkDestroySemaphore(device, renderFinishedSemaphore, allocnullptr);

			// clean up command pool
			vkDestroyCommandPool(device, commandPool, allocnullptr);

			// destroy logical device
			vkDestroyDevice(device, allocnullptr);

			if (enableValidationLayers)
			{
				DestroyDebugReportCallbackEXT(pvinstance, this->debugCallbackExt, allocnullptr);
			}

			vkDestroySurfaceKHR(pvinstance, surface, allocnullptr);
			vkDestroyInstance(pvinstance, allocnullptr);
		}

		// GLFW Cleanup
		{
			// destroy GLFW window
			glfwDestroyWindow(pvWindow);
			pvWindow = nullptr;
			// close down glfw
			glfwTerminate();
		}
	}
	static void DestroyDebugReportCallbackEXT(VkInstance instance, VkDebugReportCallbackEXT callback, const VkAllocationCallbacks* pAllocator)
	{
		{
			auto func = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugReportCallbackEXT");
			if (func == nullptr)
			{
				throw std::runtime_error("failed to load vkDestroyDebugReportCallbackEXT extension function in cleanup");
			}
			func(instance, callback, pAllocator);
		}
	}

	void cleanupSwapChain()
	{
		// cleanup framebuffer
		for (auto framebuffer : swapChainFramebuffers)
		{
			vkDestroyFramebuffer(device, framebuffer, allocnullptr);
		}
		swapChainFramebuffers.clear();

		// destroy the command buffers for every swap chain
		vkFreeCommandBuffers(device, commandPool, static_cast<uint32_t>(commandBuffers.size()), commandBuffers.data());
		commandBuffers.clear();

		// destroy graphics Pipeline
		vkDestroyPipeline(device, graphicsPipeline, allocnullptr);
		// Cleanup  PipelineLayout
		vkDestroyPipelineLayout(device, pipelineLayout, allocnullptr);
		// clenaup renderPass object
		vkDestroyRenderPass(device, renderPass, allocnullptr);

		// cleanup any imageviews connected to the swap chain
		for (auto imageView : swapChainImageViews)
		{
			vkDestroyImageView(device, imageView, allocnullptr);
		}
		swapChainImageViews.clear();


		// destroy swap chain
		vkDestroySwapchainKHR(device, swapChain, nullptr);
		swapChainImages.clear();
		
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
	return EXIT_SUCCESS;
}




	// CPU -> GPU Memory Mapping Example
	/*
	void createVertexBuffer()
	{
		VkBufferCreateInfo bufferInfo = {};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = sizeof(vertices[0]) * vertices.size();
		bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		PV_VK_RUN(vkCreateBuffer(device, &bufferInfo, allocnullptr, &vertexBuffer));

		VkMemoryRequirements bufferMemProperties;
		// The buffer size may differ from the given creatInfo above
		vkGetBufferMemoryRequirements(device, vertexBuffer, &bufferMemProperties);


		VkMemoryAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = bufferMemProperties.size;
		// memory we want needs to be visible and coherent for a vertex buffer
		allocInfo.memoryTypeIndex = findMemoryType(bufferMemProperties.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
		
		PV_VK_RUN(vkAllocateMemory(device, &allocInfo, allocnullptr, &vertexBufferMemory));

		// bind the vertext buffer to the memory buffer. This may the thought of
		// as assigning a pointer into the allocate buffer
		vkBindBufferMemory(device, vertexBuffer, vertexBufferMemory, 0);

		// map a ptr to gpu memory so we can put data into the vertex buffer, then unmap it
		{
			void* gpuMemPtr;
			// VK_WHOLE_SIZE may be used to specified to map all of the memory in the buffer
			// flags don't do anything as of vk1.1
			vkMapMemory(device, vertexBufferMemory, 0, bufferInfo.size, 0, &gpuMemPtr);

			// accessing data this way may not transfer/read everything over b/c of caching, usage of
			// vkFlushMappedMemoryRanges(...) after writing to memory and
			// vkInvalidateMappedMemoryRanges(...) while reading will fix this issue. 
			// Alternativly: We can also just make sure the memory is VK_MEMORY_PROPERTY_HOST_COHERENT_BIT as
			// we have done above when we chose the memoryTypeIndex

			// @SPEED, usage of flush/invalidate may be faster instead of the memory bit
			// Although this may not matter in the long run b/c of reasons...
			memcpy(gpuMemPtr, vertices.data(), static_cast<size_t>(bufferInfo.size));
			vkUnmapMemory(device, vertexBufferMemory);
		}
	}
	*/
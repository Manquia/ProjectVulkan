#pragma once

//
// Pre-Includes required: Vulkan, glm
//
#include <array>


// colored vertexes
// @TODO: This is a veriadic template version of this which we could use
// for all of our different vertex data. May be a consideraton in the future
// @POLISH?
struct Vertex
{
	glm::vec2 position;
	glm::vec3 color;
	//instance glm::vec3 globalColor;

	// @TODO Should be able to generate this information with a 
	// tag in JAI if we ever port this over...
	static  std::array<VkVertexInputBindingDescription, 2> getBindingDescription()
	{
		std::array<VkVertexInputBindingDescription, 2> bindingDescriptions = {};

		bindingDescriptions[0].binding = 0;
		bindingDescriptions[0].stride = sizeof(glm::vec2) + sizeof(glm::vec3);
		bindingDescriptions[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		bindingDescriptions[1].binding = 1;
		bindingDescriptions[1].stride = sizeof(glm::vec3);
		bindingDescriptions[1].inputRate = VK_VERTEX_INPUT_RATE_INSTANCE;


		return bindingDescriptions;
	}

	static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions()
	{
		std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions = {};

		// Per Vertex Data
		attributeDescriptions[0].binding = 0;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT; // matches glm::vec2
		attributeDescriptions[0].offset = offsetof(Vertex, position);
		
		attributeDescriptions[1].binding = 0;
		attributeDescriptions[1].location = 1;
		attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[1].offset = offsetof(Vertex, color);

		// Per Instance Data
		attributeDescriptions[1].binding = 1;
		attributeDescriptions[1].location = 0;
		attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[1].offset = 0;

		return attributeDescriptions;
	}
};
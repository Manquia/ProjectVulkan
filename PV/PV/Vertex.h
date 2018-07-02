#pragma once

//
// Pre-Includes required: Vulkan, glm/glm.hpp
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
	glm::vec2 texCoord;

	// @TODO Should be able to generate this information with a 
	// tag in JAI if we ever port this over...
	static  std::array<VkVertexInputBindingDescription, 1> getBindingDescription()
	{
		std::array<VkVertexInputBindingDescription, 1> bindingDescriptions = {};

		bindingDescriptions[0].binding = 0;
		bindingDescriptions[0].stride = sizeof(Vertex);
		bindingDescriptions[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

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

		attributeDescriptions[2].binding = 0;
		attributeDescriptions[2].location = 2;
		attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
		attributeDescriptions[2].offset = offsetof(Vertex, texCoord);

		return attributeDescriptions;
	}
};
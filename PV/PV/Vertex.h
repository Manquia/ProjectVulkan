#pragma once

//
// Includes: Vulkan, glm
//


// colored vertexes
// @TODO: This is a veriadic template version of this which we could use
// for all of our different vertex data. May be a consideraton in the future
// @POLISH?
struct Vertex
{
	glm::vec2 position;
	glm::vec3 color;

	// @TODO Should be able to generate this information with a 
	// tag in JAI if we ever port this over...
	static VkVertexInputBindingDescription getBindingDescription()
	{
		VkVertexInputBindingDescription bindingDescription = {};

		bindingDescription.binding = 0;
		bindingDescription.stride = sizeof(Vertex);
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		// @TODO @INSTANCING
		// inputRate could also be VK_VERTEXT_INPUT_RATE_INSTANCE which would feed new data
		// for the next instance

		return bindingDescription;
	}
};
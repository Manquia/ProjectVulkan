#pragma once

/*
	Include dependencies: glm, Vulkan
*/
#include "MultiArray.h"


struct Mesh 
{
	MultiArray<glm::vec3, uint32_t, glm::vec2> data;

	Mesh(uint32_t num_vertices, uint32_t num_indices)
		: data({ num_vertices, num_indices, num_vertices })
	{ }

	ArrayView<glm::vec3> positions() {
		return data.getView<0, glm::vec3>();
	}
	ArrayView<uint32_t> indices() {
		return data.getView<1, uint32_t>();
	}
	ArrayView<glm::vec2> uvs() {
		return data.getView<2, glm::vec2>();
	}

	// @MESH
	VkBuffer vertexBuffer;
	VkDeviceMemory vertexBufferMemory;

};

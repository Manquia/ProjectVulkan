#pragma once

/*
	Include dependencies: glm, Vulkan
*/
#include "MultiArray.h"


struct Mesh 
{
	// Warning: You must change the ArrayViews below if you
	// change this data layout
	MultiArray<
		glm::vec3, // position
		glm::vec3, // Normal
		glm::vec3, // tangent
		glm::vec3, // bitangent
		glm::vec2> // texCoord
		preVertData;

	Mesh(uint32_t num_vertices, uint32_t num_indices)
		: preVertData({ num_vertices, num_indices, num_vertices })
	{ }

	ArrayView<glm::vec3> positions() {
		return preVertData.getView<0, glm::vec3>();
	}
	ArrayView<glm::vec3> normals() {
		return preVertData.getView<1, glm::vec3>();
	}
	ArrayView<glm::vec3> tangent() {
		return preVertData.getView<2, glm::vec3>();
	}
	ArrayView<glm::vec3> bitangent() {
		return preVertData.getView<3, glm::vec3>();
	}
	ArrayView<glm::vec2> uvs() {
		return preVertData.getView<4, glm::vec2>();
	}

	// @MESH
	VkBuffer vertexBuffer;
	VkDeviceMemory vertexBufferMemory;
};

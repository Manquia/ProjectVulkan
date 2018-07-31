#pragma once

//
// Pre-Includes required: Vulkan, glm/glm.hpp, Macros.h
//
#include <array>
#include <unordered_map>
#include <typeinfo>
#include <typeindex>
#include "static_util.h"


template <class ... Args>
struct VertexData
{
	static const constexpr int s_num_params = sizeof...(Args);
	static const constexpr std::array<uint32_t, s_num_params> type_sizes = { sizeof(Args)... };
	//static const constexpr std::array<size_t, s_num_params>   type_offsets = 
	static const constexpr size_t type_size =	static_sum<sizeof(Args)...>::value;

	unsigned char data[type_size];

	static const std::array<const std::type_info*, s_num_params>& getTypeIds()
	{
		static std::array<const std::type_info*, s_num_params> type_info_array = { &typeid(Args)... };
		return type_info_array;
	}

	template<size_t ParameterIndex>
	static const size_t GetOffsetOfData()
	{
		size_t partialSum = 0;
		for (int i = 0; i <= ParameterIndex; ++i)
		{
			partialSum += type_sizes[i];
		}
		return partialSum;
	}

};



// @HIDE ME
#define FORMAT(TYPE, FORMAT) { std::type_index(typeid(TYPE)), FORMAT },
static const std::unordered_map<std::type_index, VkFormat> TypeIdToVKFormatMap =
{
	FORMAT(float, VK_FORMAT_R32_SFLOAT)
	FORMAT(glm::vec2, VK_FORMAT_R32G32_SFLOAT)
	FORMAT(glm::vec3, VK_FORMAT_R32G32B32_SFLOAT)
	FORMAT(glm::vec4, VK_FORMAT_R32G32B32A32_SFLOAT)

	FORMAT(uint32_t, VK_FORMAT_R32_UINT)
	FORMAT(glm::quat, VK_FORMAT_R32G32B32A32_SFLOAT)


	//@Expansion add more formats for different types
};
#undef FORMAT


// returns VK_FORMAT_UNDEFINED if undefined
static VkFormat TypeIndexToVkFormat(std::type_index index)
{
	auto found = TypeIdToVKFormatMap.find(index);
	if (found == TypeIdToVKFormatMap.end())
		return VK_FORMAT_UNDEFINED;
	else
		return (*found).second;
}


template<int attributeCount>
struct InputDescription
{
	VkVertexInputBindingDescription								  binding;
	std::array<VkVertexInputAttributeDescription, attributeCount> attributes;
};

template<class VertexDataType>
InputDescription<VertexDataType::s_num_params> GetInputDescription(int binding, VkVertexInputRate rate)
{
	InputDescription<VertexDataType::s_num_params> in;
	auto typeIds = VertexDataType::getTypeIds();
		
	int offset = 0;

	for (int i = 0; i < VertexDataType::s_num_params; ++i)
	{
		in.attributes[i].location = i;
		in.attributes[i].binding = binding;
		in.attributes[i].offset = offset;
		in.attributes[i].format = TypeIndexToVkFormat(std::type_index(*typeIds[i]));

		// See TypeIdToVKFormatMap to add in the new format for you type
		PV_ASSERT(in.attributes[i].format != VK_FORMAT_UNDEFINED,
			std::string("TypeIdToVKFormatMap doesn't contain a map for the type of name: ") + std::string(typeIds[i]->name())
		);

		offset += VertexDataType::type_sizes[i];
	}

	in.binding.binding = binding;
	in.binding.inputRate = rate;
	in.binding.stride = offset;
	return in;
}



// colored vertexes
// @TODO: This is a veriadic template version of this which we could use
// for all of our different vertex data. May be a consideraton in the future
// @POLISH?
struct Vertex
{
	glm::vec3 position;
	glm::vec3 color;
	glm::vec2 texCoord;
	
	bool operator==(const Vertex& other) const 
	{
		return  position == other.position &&
				color == other.color &&
				texCoord == other.texCoord;
	}

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
		attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT; // matches glm::vec2
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

namespace std {
	template<> struct hash<Vertex> {
		size_t operator()(Vertex const& vertex) const {
			return ((hash<glm::vec3>()(vertex.position) ^
				(hash<glm::vec3>()(vertex.color) << 1)) >> 1) ^
				(hash<glm::vec2>()(vertex.texCoord) << 1);
		}
	};
}
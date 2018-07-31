#pragma once


// Include Dependencies: tiny_obj_loader.h, MultiArray, glm

#include "Vertex.h"


struct LoadedModelData
{
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
};


void LoadModelData(std::string modelPath, LoadedModelData* out_data)
{
	std::string err;
	if (!tinyobj::LoadObj(&(out_data->attrib), &(out_data->shapes), &(out_data->materials), &err, modelPath.c_str())) {
		throw std::runtime_error(err);
	}
}




template<uint32_t flags>
void DoLoad()
{
	const constexpr bool normals = flags & 1;
	const constexpr bool tangents = flags & 2;
	const constexpr bool bitangents = flags & 4;
	const constexpr bool colors = flags & 8;
	const constexpr bool reduction = flags & 16;




}

decltype(DoLoad<0>)* loadFunctions[] =
{
	DoLoad<0>,
	DoLoad<1>,
	DoLoad<2>,
	DoLoad<3>,
	DoLoad<4>,
	DoLoad<5>,
	DoLoad<6>,
	DoLoad<7>,
	DoLoad<8>,
	DoLoad<9>,
	DoLoad<10>,
	DoLoad<11>,
	DoLoad<12>,
	DoLoad<13>,
	DoLoad<14>,
	DoLoad<15>,
	DoLoad<16>,
	DoLoad<17>,
	DoLoad<18>,
	DoLoad<19>,
	DoLoad<20>,
	DoLoad<21>,
	DoLoad<22>,
	DoLoad<23>,
	DoLoad<24>,
	DoLoad<25>,
	DoLoad<26>,
	DoLoad<27>,
	DoLoad<28>,
	DoLoad<29>,
	DoLoad<30>,
	DoLoad<31>,
};


template <typename ... Args>
struct VertexBuffer
{
	std::vector<VertexData<Args...>> buffer;

	void foo()
	{

	}
};


void LoadMesh(const LoadedModelData* data, bool normals, bool tangents, bool bitangents, bool colors)
{
	VertexData<glm::vec3> vData;
	std::vector<decltype(data)> dataBuffer;


	uint32_t functionPtrFlag = 0;
	{
		functionPtrFlag += normals		? 1 : 0;
		functionPtrFlag += tangents		? 2 : 0;
		functionPtrFlag += bitangents	? 4 : 0;
		functionPtrFlag += colors		? 8 : 0;
	}
	auto loadFunc = loadFunctions[functionPtrFlag];

	Mesh m(2, 2);


	loadFunc();

}




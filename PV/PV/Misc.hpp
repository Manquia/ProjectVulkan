#pragma once

#include <vector>
#include <fstream>
#include <stdexcept>

#pragma region HelperFreeFunctions

std::vector<char> readBinaryFile(const std::string& filename)
{
	// read file, starting from the end
	std::ifstream file(filename, std::ios::ate | std::ios::binary);
	
	// failed to open file?
	if (!file.is_open())
	{
		throw std::runtime_error("failed to open file");
	}

	size_t fileSize = (size_t)file.tellg();
	std::vector<char> binaryBuffer(fileSize);

	// move file pointer to begining of file
	file.seekg(0);
	file.read(binaryBuffer.data(), fileSize);
	file.close();

	return binaryBuffer;
}


#pragma endregion
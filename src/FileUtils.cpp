#include "FileUtils.h"

#include <fstream>
#include <stdexcept>

namespace Vulkan3DEngine
{
	std::vector<char> FileUtils::readBinaryFile(const std::string& filePath)
	{
		std::ifstream file(filePath, std::ios::ate | std::ios::binary);
		if (!file.is_open()) {
			throw std::runtime_error("Failed to open binary file: " + filePath);
		}

		size_t fileSize = static_cast<size_t>(file.tellg());
		std::vector<char> buff(fileSize);

		file.seekg(0);
		file.read(buff.data(), fileSize);
		file.close();

		return buff;
	}
}
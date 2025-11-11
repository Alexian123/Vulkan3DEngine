#pragma once

#include <string>
#include <vector>

namespace Vulkan3DEngine
{

	class FileUtils
	{
	public:
		static std::vector<char> readBinaryFile(const std::string& filePath);
	};

}
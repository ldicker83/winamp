#include <libopenmpt/libopenmpt.h>
#include <exception>
#include <filesystem>
#include <fstream>
#include <system_error>
#include <vector>

openmpt_module *OpenMod(const wchar_t *filename)
{
	// this is called through the plug-in API, so nothing can be allowed to throw past here
	try
	{
		const std::filesystem::path path(filename);

		std::error_code error;
		const auto size = std::filesystem::file_size(path, error);
		if (error)
			return nullptr;

		std::vector<char> data(static_cast<size_t>(size));
		std::ifstream file(path, std::ios::binary);
		if (!file.read(data.data(), static_cast<std::streamsize>(data.size())))
			return nullptr;

		return openmpt_module_create_from_memory2(data.data(), data.size(), nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr);
	}
	catch (const std::exception &)
	{
		return nullptr;
	}
}

#include <cctype>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>

#include "nlohmann/json.hpp"

using json = nlohmann::json;

static std::vector<uint8_t> read_file(const std::string& path)
{
	std::ifstream f(path, std::ios::binary);
	if (!f)
	{
		throw std::runtime_error("Failed to open " + path);
	}

	return std::vector<uint8_t>(std::istreambuf_iterator<char>(f), {});
}

static std::string symbol_name(const std::string& name, const std::string& stage)
{
	return name + "_" + stage + "_spv";
}

int main(int argc, char** argv)
{
	if (argc != 3)
	{
		std::cerr << "usage: spv2h <manifest.json> <output.h>\n";
		return 1;
	}

	std::ifstream mf(argv[1]);
	if (!mf)
	{
		std::cerr << "Failed to open manifest\n";
		return 1;
	}

	json manifest;
	mf >> manifest;

	std::ofstream out(argv[2]);
	if (!out)
	{
		std::cerr << "Failed to open output header\n";
		return 1;
	}

	out << "#ifndef SPEL_GFX_INTERNAL_SHADERS\n"
		   "#define SPEL_GFX_INTERNAL_SHADERS\n\n";

	for (const auto& shader : manifest["shaders"])
	{
		std::string name = shader["name"];
		std::string stage = shader["stage"];
		std::string path = shader["path"];

		auto data = read_file(path);
		auto sym = symbol_name(name, stage);

		out << "unsigned char " << sym << "[] = {\n    ";

		for (size_t i = 0; i < data.size(); ++i)
		{
			out << "0x" << std::hex << std::uppercase << (data[i] >> 4) << (data[i] & 0xF)
				<< std::dec;

			if (i + 1 != data.size())
				out << ", ";

			if ((i + 1) % 12 == 0)
				out << "\n    ";
		}

		out << "\n};\n";
		out << "unsigned int " << sym << "_len = " << data.size() << ";\n\n";
	}

	out << "#endif // SPEL_GFX_INTERNAL_SHADERS\n";
	return 0;
}

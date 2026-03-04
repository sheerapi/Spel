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
	if (argc != 4)
	{
		std::cerr << "usage: spv2h <manifest.json> <output.h> <output.c>\n";
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

	std::ofstream header_out(argv[2]);
	if (!header_out)
	{
		std::cerr << "Failed to open output header\n";
		return 1;
	}

	std::ofstream source_out(argv[3]);
	if (!source_out)
	{
		std::cerr << "Failed to open output source\n";
		return 1;
	}

	std::string header_name = argv[2];
	const auto slash = header_name.find_last_of("/\\");
	if (slash != std::string::npos)
	{
		header_name = header_name.substr(slash + 1);
	}

	header_out << "#ifndef SPEL_GFX_INTERNAL_SHADERS\n"
	              "#define SPEL_GFX_INTERNAL_SHADERS\n\n"
                  "#ifdef __cplusplus\nextern \"C\" {\n#endif\n\n";

	source_out << "#include \"" << header_name << "\"\n\n";

	for (const auto& shader : manifest["shaders"])
	{
		std::string name = shader["name"];
		std::string stage = shader["stage"];
		std::string path = shader["path"];

		auto data = read_file(path);
		auto sym = symbol_name(name, stage);

		header_out << "extern unsigned char " << sym << "[];\n";
		header_out << "extern unsigned int " << sym << "_len;\n\n";

		source_out << "unsigned char " << sym << "[] = {\n    ";

		for (size_t i = 0; i < data.size(); ++i)
		{
			source_out << "0x" << std::hex << std::uppercase << (data[i] >> 4)
				  << (data[i] & 0xF) << std::dec;

			if (i + 1 != data.size())
				source_out << ", ";

			if ((i + 1) % 12 == 0)
				source_out << "\n    ";
		}

		source_out << "\n};\n";
		source_out << "unsigned int " << sym << "_len = " << data.size()
			   << ";\n\n";
	}

	header_out << "#ifdef __cplusplus\n}\n#endif\n\n"
			   "#endif // SPEL_GFX_INTERNAL_SHADERS\n";
	return 0;
}

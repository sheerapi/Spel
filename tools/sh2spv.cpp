#include "glslang/Public/ShaderLang.h"
#include <csignal>
#include <map>
#ifdef SPEL_SYSTEM_GLSLANG
#	include "glslang/SPIRV/GlslangToSpv.h"
#	include "glslang/SPIRV/disassemble.h"
#else
#	include "SPIRV/GlslangToSpv.h"
#	include "SPIRV/disassemble.h"
#endif

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

auto args_has(int argc, const char** argv, const char* arg) -> int
{
	for (int i = 1; i < argc; i++)
	{
		if (strncmp(arg, argv[i], strlen(arg)) == 0)
		{
			return i;
		}
	}

	return -1;
}

struct sampler_info
{
	uint32_t spirv_id;
	uint32_t binding;
};

constexpr uint32_t OP_TYPE_SAMPLER = 26;
constexpr uint32_t OP_TYPE_SAMPLED_IMAGE = 27;
constexpr uint32_t OP_TYPE_POINTER = 32;
constexpr uint32_t OP_VARIABLE = 59;
constexpr uint32_t OP_DECORATE = 71;
constexpr uint32_t DECORATION_BINDING = 33;
constexpr uint32_t DECORATION_LOCATION = 30;

auto find_samplers(const std::vector<uint32_t>& spirv) -> std::vector<sampler_info>
{
	std::vector<sampler_info> samplers;
	std::unordered_map<uint32_t, uint32_t> id_to_binding;
	std::unordered_map<uint32_t, uint32_t> type_ids;
	std::unordered_map<uint32_t, bool> is_sampler_type;

	size_t idx = 5;

	while (idx < spirv.size())
	{
		uint16_t opcode = spirv[idx] & 0xFFFF;
		uint16_t word_count = spirv[idx] >> 16;

		if (opcode == OP_DECORATE && spirv[idx + 2] == DECORATION_BINDING)
		{
			uint32_t target_id = spirv[idx + 1];
			uint32_t binding = spirv[idx + 3];
			id_to_binding[target_id] = binding;
		}
		else if (opcode == OP_TYPE_SAMPLED_IMAGE || opcode == OP_TYPE_SAMPLER)
		{
			uint32_t result_id = spirv[idx + 1];
			is_sampler_type[result_id] = true;
		}
		else if (opcode == OP_TYPE_POINTER)
		{

			uint32_t result_id = spirv[idx + 1];
			uint32_t pointee_type = spirv[idx + 3];

			if (is_sampler_type.count(pointee_type))
			{
				is_sampler_type[result_id] = true;
			}
		}
		else if (opcode == OP_VARIABLE)
		{

			uint32_t result_type = spirv[idx + 1];
			uint32_t result_id = spirv[idx + 2];

			if (is_sampler_type.count(result_type) && id_to_binding.count(result_id))
			{
				samplers.push_back({result_id, id_to_binding[result_id]});
			}
		}

		idx += word_count;
	}

	return samplers;
}

auto find_decoration_insert_point(const std::vector<uint32_t>& spirv) -> size_t
{
	size_t idx = 5;

	while (idx < spirv.size())
	{
		uint16_t opcode = spirv[idx] & 0xFFFF;
		uint16_t word_count = spirv[idx] >> 16;

		if (opcode >= 19 && opcode <= 68)
		{
			return idx;
		}

		idx += word_count;
	}

	return idx;
}

void add_sampler_locations(
	std::vector<uint32_t>& spirv,
	const std::vector<std::pair<uint32_t, uint32_t>>& idLocationPairs)
{
	size_t insert_pos = find_decoration_insert_point(spirv);

	std::vector<uint32_t> all_decorations;

	for (const auto& [sampler_id, location] : idLocationPairs)
	{
		all_decorations.push_back((4 << 16) | OP_DECORATE);
		all_decorations.push_back(sampler_id);
		all_decorations.push_back(DECORATION_LOCATION);
		all_decorations.push_back(location);
	}

	spirv.insert(spirv.begin() + insert_pos, all_decorations.begin(),
				 all_decorations.end());
}

auto string_split(const std::string& str, const std::string& delimiter)
	-> std::vector<std::string>
{
	size_t pos_start = 0;
	size_t pos_end;
	size_t delim_len = delimiter.length();
	std::string token;
	std::vector<std::string> res;

	while ((pos_end = str.find(delimiter, pos_start)) != std::string::npos)
	{
		token = str.substr(pos_start, pos_end - pos_start);
		pos_start = pos_end + delim_len;
		res.push_back(token);
	}

	res.push_back(str.substr(pos_start));
	return res;
}

auto trim(const std::string& text) -> std::string
{
	const auto first = text.find_first_not_of(" \t\r\n");
	if (first == std::string::npos)
	{
		return "";
	}

	const auto last = text.find_last_not_of(" \t\r\n");
	return text.substr(first, last - first + 1);
}

struct stage_config
{
	EShLanguage lang;
	std::string suffix;
};

struct shader_manifest_entry
{
	std::string name;
	std::string stage;
	std::string path;
};

auto stage_from_char(char stage) -> std::optional<stage_config>
{
	switch (stage)
	{
	case 'v':
		return stage_config{EShLangVertex, "vert"};
	case 'f':
		return stage_config{EShLangFragment, "frag"};
	case 'g':
		return stage_config{EShLangGeometry, "geo"};
	case 'c':
		return stage_config{EShLangCompute, "comp"};
	case 't':
		return stage_config{EShLangTessControl, "tesc"};
	case 'e':
		return stage_config{EShLangTessEvaluation, "tese"};
	default:
		return std::nullopt;
	}
}

auto stage_from_name(std::string stage_name) -> std::optional<stage_config>
{
	std::transform(stage_name.begin(), stage_name.end(), stage_name.begin(),
				   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

	static const std::unordered_map<std::string, stage_config> stage_map = {
		{"vertex", {EShLangVertex, "vert"}},
		{"vert", {EShLangVertex, "vert"}},
		{"fragment", {EShLangFragment, "frag"}},
		{"frag", {EShLangFragment, "frag"}},
		{"pixel", {EShLangFragment, "frag"}},
		{"geometry", {EShLangGeometry, "geo"}},
		{"geo", {EShLangGeometry, "geo"}},
		{"compute", {EShLangCompute, "comp"}},
		{"comp", {EShLangCompute, "comp"}},
		{"tesscontrol", {EShLangTessControl, "tesc"}},
		{"tess_control", {EShLangTessControl, "tesc"}},
		{"tesc", {EShLangTessControl, "tesc"}},
		{"tessevaluation", {EShLangTessEvaluation, "tese"}},
		{"tess_evaluation", {EShLangTessEvaluation, "tese"}},
		{"tese", {EShLangTessEvaluation, "tese"}},
	};

	auto stage = stage_map.find(stage_name);
	if (stage == stage_map.end())
	{
		return std::nullopt;
	}

	return stage->second;
}

auto parse_stage_line(const std::string& line) -> std::optional<std::string>
{
	auto trimmed_line = trim(line);

	if (trimmed_line.rfind("#pragma", 0) != 0)
	{
		return std::nullopt;
	}

	auto lower_line = trimmed_line;
	std::transform(lower_line.begin(), lower_line.end(), lower_line.begin(),
				   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

	if (lower_line.rfind("#pragma shader_stage", 0) != 0)
	{
		return std::nullopt;
	}

	const auto open = lower_line.find('(');
	const auto close = lower_line.find(')', open + 1);

	if (open == std::string::npos || close == std::string::npos || close <= open + 1)
	{
		return std::nullopt;
	}

	auto stage = trim(trimmed_line.substr(open + 1, close - open - 1));
	if (stage.empty())
	{
		return std::nullopt;
	}

	return stage;
}

struct stage_section
{
	std::string stage_name;
	size_t start = 0;
	size_t end = 0;
};

struct stage_parse_result
{
	size_t prefix_length = 0;
	std::vector<stage_section> sections;
};

auto find_stage_sections(const std::string& source) -> stage_parse_result
{
	stage_parse_result result{};
	size_t line_start = 0;
	bool seen_stage = false;

	while (line_start < source.size())
	{
		size_t line_end = source.find('\n', line_start);
		if (line_end == std::string::npos)
		{
			line_end = source.size();
		}

		const auto line = source.substr(line_start, line_end - line_start);
		const auto parsed_stage = parse_stage_line(line);

		const size_t next_line_start =
			(line_end == source.size()) ? source.size() : line_end + 1;

		if (parsed_stage.has_value())
		{
			if (!seen_stage)
			{
				result.prefix_length = line_start;
				seen_stage = true;
			}
			else
			{
				result.sections.back().end = line_start;
			}

			stage_section section{};
			section.stage_name = *parsed_stage;
			section.start = next_line_start;
			section.end = source.size();

			result.sections.push_back(section);
		}

		line_start = next_line_start;
	}

	if (!result.sections.empty())
	{
		result.sections.back().end = source.size();
	}

	return result;
}

class file_finder
{
public:
	auto findReadableFilepath(const std::string& filename) const -> std::string;
	std::string FindRelativeReadableFilepath(const std::string& requesting_file,
											 const std::string& filename) const;

	auto searchPath() -> std::vector<std::string>&
	{
		return _searchPath;
	}

private:
	std::vector<std::string> _searchPath;
};

class CustomIncluder : public glslang::TShader::Includer
{
public:
	explicit CustomIncluder(const file_finder* file_finder) : file_finder_(*file_finder)
	{
	}

	~CustomIncluder() override = default;

	IncludeResult* includeSystem(const char* headerName, const char* includerName,
								 size_t inclusionDepth) override
	{
		return readFile(headerName, includerName, false);
	}

	IncludeResult* includeLocal(const char* headerName, const char* includerName,
								size_t inclusionDepth) override
	{
		return readFile(headerName, includerName, true);
	}

	void releaseInclude(IncludeResult* result) override
	{
		if (result != nullptr)
		{
			delete[] static_cast<const char*>(result->headerData);
			delete result;
		}
	}

	const std::unordered_set<std::string>& file_path_trace() const
	{
		return included_files_;
	}

private:
	const file_finder& file_finder_;
	std::unordered_set<std::string> included_files_;

	IncludeResult* readFile(const char* headerName, const char* includerName, bool local)
	{
		std::string full_path =
			local ? file_finder_.FindRelativeReadableFilepath(includerName, headerName)
				  : file_finder_.findReadableFilepath(headerName);

		if (full_path.empty())
		{
			return new IncludeResult("", nullptr, 0, nullptr);
		}

		std::ifstream file(full_path, std::ios::binary | std::ios::ate);
		if (!file.is_open())
		{
			return new IncludeResult("", nullptr, 0, nullptr);
		}

		std::streamsize size = file.tellg();
		file.seekg(0, std::ios::beg);

		char* buffer = new char[size];
		if (!file.read(buffer, size))
		{
			delete[] buffer;
			return new IncludeResult("", nullptr, 0, nullptr);
		}

		included_files_.insert(full_path);

		return new IncludeResult(full_path, buffer, size, nullptr);
	}
};

TBuiltInResource GetDefaultResources()
{
	TBuiltInResource resources = {};
	resources.maxLights = 32;
	resources.maxClipPlanes = 6;
	resources.maxTextureUnits = 32;
	resources.maxTextureCoords = 32;
	resources.maxVertexAttribs = 64;
	resources.maxVertexUniformComponents = 4096;
	resources.maxVaryingFloats = 64;
	resources.maxVertexTextureImageUnits = 32;
	resources.maxCombinedTextureImageUnits = 80;
	resources.maxTextureImageUnits = 32;
	resources.maxFragmentUniformComponents = 4096;
	resources.maxDrawBuffers = 32;
	resources.maxVertexUniformVectors = 128;
	resources.maxVaryingVectors = 8;
	resources.maxFragmentUniformVectors = 16;
	resources.maxVertexOutputVectors = 16;
	resources.maxFragmentInputVectors = 15;
	resources.minProgramTexelOffset = -8;
	resources.maxProgramTexelOffset = 7;
	resources.maxClipDistances = 8;
	resources.maxComputeWorkGroupCountX = 65535;
	resources.maxComputeWorkGroupCountY = 65535;
	resources.maxComputeWorkGroupCountZ = 65535;
	resources.maxComputeWorkGroupSizeX = 1024;
	resources.maxComputeWorkGroupSizeY = 1024;
	resources.maxComputeWorkGroupSizeZ = 64;
	resources.maxComputeUniformComponents = 1024;
	resources.maxComputeTextureImageUnits = 16;
	resources.maxComputeImageUniforms = 8;
	resources.maxComputeAtomicCounters = 8;
	resources.maxComputeAtomicCounterBuffers = 1;
	resources.maxVaryingComponents = 60;
	resources.maxVertexOutputComponents = 64;
	resources.maxGeometryInputComponents = 64;
	resources.maxGeometryOutputComponents = 128;
	resources.maxFragmentInputComponents = 128;
	resources.maxImageUnits = 8;
	resources.maxCombinedImageUnitsAndFragmentOutputs = 8;
	resources.maxCombinedShaderOutputResources = 8;
	resources.maxImageSamples = 0;
	resources.maxVertexImageUniforms = 0;
	resources.maxTessControlImageUniforms = 0;
	resources.maxTessEvaluationImageUniforms = 0;
	resources.maxGeometryImageUniforms = 0;
	resources.maxFragmentImageUniforms = 8;
	resources.maxCombinedImageUniforms = 8;
	resources.maxGeometryTextureImageUnits = 16;
	resources.maxGeometryOutputVertices = 256;
	resources.maxGeometryTotalOutputComponents = 1024;
	resources.maxGeometryUniformComponents = 1024;
	resources.maxGeometryVaryingComponents = 64;
	resources.maxTessControlInputComponents = 128;
	resources.maxTessControlOutputComponents = 128;
	resources.maxTessControlTextureImageUnits = 16;
	resources.maxTessControlUniformComponents = 1024;
	resources.maxTessControlTotalOutputComponents = 4096;
	resources.maxTessEvaluationInputComponents = 128;
	resources.maxTessEvaluationOutputComponents = 128;
	resources.maxTessEvaluationTextureImageUnits = 16;
	resources.maxTessEvaluationUniformComponents = 1024;
	resources.maxTessPatchComponents = 120;
	resources.maxPatchVertices = 32;
	resources.maxTessGenLevel = 64;
	resources.maxViewports = 16;
	resources.maxVertexAtomicCounters = 0;
	resources.maxTessControlAtomicCounters = 0;
	resources.maxTessEvaluationAtomicCounters = 0;
	resources.maxGeometryAtomicCounters = 0;
	resources.maxFragmentAtomicCounters = 8;
	resources.maxCombinedAtomicCounters = 8;
	resources.maxAtomicCounterBindings = 1;
	resources.maxVertexAtomicCounterBuffers = 0;
	resources.maxTessControlAtomicCounterBuffers = 0;
	resources.maxTessEvaluationAtomicCounterBuffers = 0;
	resources.maxGeometryAtomicCounterBuffers = 0;
	resources.maxFragmentAtomicCounterBuffers = 1;
	resources.maxCombinedAtomicCounterBuffers = 1;
	resources.maxAtomicCounterBufferSize = 16384;
	resources.maxTransformFeedbackBuffers = 4;
	resources.maxTransformFeedbackInterleavedComponents = 64;
	resources.maxCullDistances = 8;
	resources.maxCombinedClipAndCullDistances = 8;
	resources.maxSamples = 4;
	resources.maxMeshOutputVerticesNV = 256;
	resources.maxMeshOutputPrimitivesNV = 512;
	resources.maxMeshWorkGroupSizeX_NV = 32;
	resources.maxMeshWorkGroupSizeY_NV = 1;
	resources.maxMeshWorkGroupSizeZ_NV = 1;
	resources.maxTaskWorkGroupSizeX_NV = 32;
	resources.maxTaskWorkGroupSizeY_NV = 1;
	resources.maxTaskWorkGroupSizeZ_NV = 1;
	resources.maxMeshViewCountNV = 4;
	resources.limits.nonInductiveForLoops = true;
	resources.limits.whileLoops = true;
	resources.limits.doWhileLoops = true;
	resources.limits.generalUniformIndexing = true;
	resources.limits.generalAttributeMatrixVectorIndexing = true;
	resources.limits.generalVaryingIndexing = true;
	resources.limits.generalSamplerIndexing = true;
	resources.limits.generalVariableIndexing = true;
	resources.limits.generalConstantMatrixVectorIndexing = true;

	return resources;
}

auto write_manifest(const std::vector<shader_manifest_entry>& entries,
					const std::filesystem::path& manifest_path) -> bool
{
	std::ofstream manifest(manifest_path);
	if (!manifest.is_open())
	{
		std::cerr << "failed to open manifest file: " << manifest_path << "\n";
		return false;
	}

	manifest << "{\n";
	manifest << "  \"shaders\": [\n";

	for (size_t i = 0; i < entries.size(); ++i)
	{
		const auto& entry = entries[i];
		manifest << "	{\n";
		manifest << "		\"name\": \""
				 << entry.name.substr(0, entry.name.find("_" + entry.stage)) << "\",\n";
		manifest << "		\"stage\": \"" << entry.stage << "\",\n";
		manifest << "		\"path\": \"" << entry.path << "\"\n";
		manifest << "	}";

		if (i < entries.size() - 1)
		{
			manifest << ",";
		}
		manifest << "\n";
	}

	manifest << "  ]\n";
	manifest << "}\n";

	return true;
}

struct resource_binding
{
	uint32_t set;
	uint32_t binding;
	std::string name;
};

auto validate_bindings(const std::vector<uint32_t>& spirv) -> bool
{
	std::map<std::pair<uint32_t, uint32_t>, std::string> binding_map;

	constexpr uint32_t DECORATION_DESCRIPTOR_SET = 34;

	std::unordered_map<uint32_t, uint32_t> id_to_binding;
	std::unordered_map<uint32_t, uint32_t> id_to_set;
	std::unordered_map<uint32_t, std::string> id_to_name;

	size_t idx = 5;

	while (idx < spirv.size())
	{
		uint16_t opcode = spirv[idx] & 0xFFFF;
		uint16_t word_count = spirv[idx] >> 16;

		if (opcode == OP_DECORATE)
		{
			uint32_t target_id = spirv[idx + 1];
			uint32_t decoration = spirv[idx + 2];

			if (decoration == DECORATION_BINDING)
			{
				id_to_binding[target_id] = spirv[idx + 3];
			}
			else if (decoration == DECORATION_DESCRIPTOR_SET)
			{
				id_to_set[target_id] = spirv[idx + 3];
			}
		}

		idx += word_count;
	}

	bool valid = true;
	for (const auto& [id, binding] : id_to_binding)
	{
		uint32_t set = id_to_set.contains(id) ? id_to_set[id] : 0;
		auto key = std::make_pair(set, binding);

		if (binding_map.contains(key))
		{
			fprintf(stderr, "Error: Duplicate binding: set=%u, binding=%u\n", set,
					binding);
			fprintf(stderr, "  Conflicts: '%s' and resource ID %u\n",
					binding_map[key].c_str(), id);
			valid = false;
		}
		else
		{
			binding_map[key] =
				id_to_name.contains(id) ? id_to_name[id] : std::to_string(id);
		}
	}

	return valid;
}

auto main(int argc, const char** argv) -> int
{
	if (argc < 2)
	{
		fprintf(stderr,
				"usage: %s <input1> [input2 ...] [-o <output>] [-O<level>] [-s<stage>] "
				"[-D<defines>] [-I<includes>] [-Werror] [-e <entry>] [--out-dir <dir>]\n",
				argv[0]);
		return -1;
	}

	std::vector<std::string> input_files;
	for (int i = 1; i < argc; ++i)
	{
		if (argv[i][0] == '-')
		{
			break;
		}
		input_files.push_back(argv[i]);
	}

	if (input_files.empty())
	{
		fprintf(stderr, "No input files specified\n");
		return -1;
	}

	const bool multiple_inputs = input_files.size() > 1;
	std::vector<std::string> defines;
	std::vector<std::string> includes;

	char optimization = 'p';
	char default_stage = 'v';
	char language = 'g';
	bool warningsAsErrors{false};
	bool hasOutDir{false};
	std::filesystem::path out_dir;

	const char* entry = "main";

	std::string output_base;

	if (args_has(argc, argv, "-O") != -1)
	{
		optimization = argv[args_has(argc, argv, "-O")][2];
	}

	if (args_has(argc, argv, "-s") != -1)
	{
		default_stage = argv[args_has(argc, argv, "-s")][2];
	}

	if (args_has(argc, argv, "--out-dir") != -1)
	{
		out_dir = argv[args_has(argc, argv, "--out-dir") + 1];
		hasOutDir = true;
	}

	if (args_has(argc, argv, "-o") != -1)
	{
		output_base = argv[args_has(argc, argv, "-o") + 1];
	}

	if (args_has(argc, argv, "-D") != -1)
	{
		defines =
			string_split(std::string(argv[args_has(argc, argv, "-D")]).substr(2), ",");
	}

	if (args_has(argc, argv, "-I") != -1)
	{
		includes =
			string_split(std::string(argv[args_has(argc, argv, "-I")]).substr(2), ",");
	}

	if (args_has(argc, argv, "-Werror") != -1)
	{
		warningsAsErrors = true;
	}

	if (args_has(argc, argv, "-e") != -1)
	{
		entry = argv[args_has(argc, argv, "-e") + 1];
	}

	if (hasOutDir && !std::filesystem::exists(out_dir))
	{
		std::filesystem::create_directories(out_dir);
	}

	glslang::InitializeProcess();

	file_finder finder;

	for (auto& folder : includes)
	{
		finder.searchPath().push_back(folder);
	}

	std::vector<shader_manifest_entry> manifest_entries;

	int result = 0;
	for (const auto& filename : input_files)
	{
		if (filename == output_base || filename == out_dir)
		{
			continue;
		}

		FILE* file = fopen(filename.c_str(), "r");
		if (file == NULL)
		{
			perror(filename.c_str());
			result = -1;
			continue;
		}

		long length = 0;
		fseek(file, 0, SEEK_END);
		length = ftell(file);
		fseek(file, 0, SEEK_SET);
		std::string source;

		if (length > 0)
		{
			source.resize(length);
			fread(source.data(), 1, length, file);
		}
		fclose(file);

		char stage = default_stage;
		char file_language = language;

		auto name =
			string_split(std::filesystem::path(filename).filename().string(), ".");

		std::string extension;
		if (name.size() >= 3)
		{
			extension = name[name.size() - 2];

			if (name[name.size() - 1] == "hlsl")
			{
				file_language = 'h';
			}
		}
		else
		{
			extension = name[name.size() - 1];
		}

		if (args_has(argc, argv, "-s") == -1)
		{
			if (extension == "vert")
			{
				stage = 'v';
			}
			else if (extension == "frag")
			{
				stage = 'f';
			}
			else if (extension == "geo")
			{
				stage = 'g';
			}
			else if (extension == "comp")
			{
				stage = 'c';
			}
			else if (extension == "tesc")
			{
				stage = 't';
			}
			else if (extension == "tese")
			{
				stage = 'e';
			}
		}

		EShLanguage lang;
		switch (stage)
		{
		case 'v':
		default:
			lang = EShLangVertex;
			break;
		case 'f':
			lang = EShLangFragment;
			break;
		case 'g':
			lang = EShLangGeometry;
			break;
		case 'c':
			lang = EShLangCompute;
			break;
		case 't':
			lang = EShLangTessControl;
			break;
		case 'e':
			lang = EShLangTessEvaluation;
			break;
		}

		const auto stage_sections = find_stage_sections(source);
		const bool has_explicit_stage = stage_sections.sections.size() > 0;

		const auto build_output_path =
			[&](const std::string& stage_suffix, bool multi_stage_output,
				const std::string& current_file) -> std::filesystem::path
		{
			std::filesystem::path base_path;

			if (multiple_inputs || multi_stage_output)
			{

				auto input_path = std::filesystem::path(current_file).filename();
				auto stem = input_path.stem().string();

				if (multi_stage_output)
				{
					auto path = std::filesystem::path(stem + "." + stage_suffix + ".spv");
					return hasOutDir ? out_dir / path : path;
				}
				else
				{
					auto path = input_path.replace_extension("spv");
					return hasOutDir ? out_dir / path : path;
				}
			}
			else
			{

				if (!output_base.empty())
				{
					auto path = std::filesystem::path(output_base);
					return hasOutDir ? out_dir / path : path;
				}

				auto input_path = std::filesystem::path(current_file).filename();
				return hasOutDir ? out_dir / input_path.replace_extension("spv")
								 : input_path.replace_extension("spv");
			}
		};

		auto compile_and_write = [&](const std::string& shaderSource,
									 const stage_config& stageInfo,
									 const std::filesystem::path& outputPath,
									 const std::string& shaderName) -> bool
		{
			glslang::TShader shader(stageInfo.lang);

			const char* source_ptr = shaderSource.c_str();
			shader.setStrings(&source_ptr, 1);
			shader.setEntryPoint(entry);
			shader.setSourceEntryPoint(entry);

			std::string preamble;
			for (const auto& def : defines)
			{
				auto definition = string_split(def, "=");
				if (definition.size() == 2)
				{
					preamble += "#define " + definition[0] + " " + definition[1] + "\n";
				}
				else
				{
					preamble += "#define " + definition[0] + "\n";
				}
			}

			if (!preamble.empty())
			{
				shader.setPreamble(preamble.c_str());
			}

			if (file_language == 'h')
			{
				shader.setEnvInput(glslang::EShSourceHlsl, stageInfo.lang,
								   glslang::EShClientVulkan, 100);
			}
			else
			{
				shader.setEnvInput(glslang::EShSourceGlsl, stageInfo.lang,
								   glslang::EShClientVulkan, 100);
			}
			shader.setEnvClient(glslang::EShClientVulkan, glslang::EShTargetVulkan_1_0);
			shader.setEnvTarget(glslang::EShTargetSpv, glslang::EShTargetSpv_1_0);

			shader.setAutoMapLocations(true);
			shader.setAutoMapBindings(true);

			auto messages = (EShMessages)(EShMsgSpvRules | EShMsgVulkanRules);
			if (warningsAsErrors)
			{
				messages = (EShMessages)(messages | EShMsgKeepUncalled);
			}

			TBuiltInResource resources = GetDefaultResources();
			CustomIncluder includer(&finder);

			if (!shader.parse(&resources, 100, true, messages, includer))
			{
				std::cerr << "failed to compile " << shaderName << ":\n";
				std::cerr << shader.getInfoLog() << "\n";
				std::cerr << shader.getInfoDebugLog() << "\n";
				return false;
			}

			glslang::TProgram program;
			program.addShader(&shader);

			if (!program.link(messages))
			{
				std::cerr << "failed to link " << shaderName << ":\n";
				std::cerr << program.getInfoLog() << "\n";
				std::cerr << program.getInfoDebugLog() << "\n";
				return false;
			}

			std::vector<unsigned int> spirv;
			spv::SpvBuildLogger logger;
			glslang::SpvOptions spvOptions;

			spvOptions.validate = true;

			std::vector<unsigned int> spirv_unoptimized;
			glslang::GlslangToSpv(*program.getIntermediate(stageInfo.lang),
								  spirv_unoptimized);

			if (!validate_bindings(spirv_unoptimized))
			{
				result = -1;
				return false;
			}

			switch (optimization)
			{
			case '0':
				spvOptions.optimizeSize = false;
				spvOptions.disableOptimizer = true;
				break;
			case 's':
			case 'z':
				spvOptions.optimizeSize = true;
				spvOptions.disableOptimizer = false;
				break;
			case 'p':
			default:
				spvOptions.optimizeSize = false;
				spvOptions.disableOptimizer = false;
				break;
			}

			glslang::GlslangToSpv(*program.getIntermediate(stageInfo.lang), spirv,
								  &logger, &spvOptions);

			auto samplers = find_samplers(spirv);

			std::vector<std::pair<uint32_t, uint32_t>> id_location_pairs;
			id_location_pairs.reserve(samplers.size());

			for (const auto& sampler : samplers)
			{
				id_location_pairs.emplace_back(sampler.spirv_id, sampler.binding);
			}

			add_sampler_locations(spirv, id_location_pairs);

			std::ofstream f(outputPath, std::ios::binary | std::ios::out);
			if (!f.is_open())
			{
				std::cerr << "failed to open output file: " << outputPath << "\n";
				return false;
			}

			f.write(reinterpret_cast<const char*>(spirv.data()),
					spirv.size() * sizeof(unsigned int));

			manifest_entries.push_back(shader_manifest_entry{
				.name = shaderName,
				.stage = stageInfo.suffix,
				.path = hasOutDir
							? std::filesystem::relative(outputPath, out_dir).string()
							: outputPath.string()});

			return true;
		};

		if (has_explicit_stage)
		{
			for (const auto& section : stage_sections.sections)
			{
				auto stage_info = stage_from_name(section.stage_name);

				if (!stage_info.has_value())
				{
					std::cerr << "unknown shader stage in pragma: " << section.stage_name
							  << "\n";
					result = -1;
					break;
				}

				std::string stage_source = source.substr(0, stage_sections.prefix_length);
				stage_source.append(
					source.substr(section.start, section.end - section.start));

				auto output_path = build_output_path(stage_info->suffix, true, filename);
				auto shader_name = std::filesystem::path(filename).stem().string() + "_" +
								   stage_info->suffix;

				if (!compile_and_write(stage_source, *stage_info, output_path,
									   shader_name))
				{
					result = -1;
					break;
				}
			}
		}
		else
		{
			auto stage_info = stage_from_char(stage).value_or(stage_config{lang, ""});
			auto output_path = build_output_path(stage_info.suffix, false, filename);
			auto shader_name = std::filesystem::path(filename).stem().string();

			if (!compile_and_write(source, stage_info, output_path, shader_name))
			{
				result = -1;
			}
		}
	}

	if (multiple_inputs && !manifest_entries.empty())
	{
		std::filesystem::path manifest_path;
		if (!output_base.empty())
		{
			manifest_path = std::filesystem::path(output_base).replace_extension("json");
			if (hasOutDir)
			{
				manifest_path = out_dir / manifest_path;
			}
		}
		else
		{
			manifest_path = hasOutDir ? (out_dir / "shaders.json") : "shaders.json";
		}

		if (!write_manifest(manifest_entries, manifest_path))
		{
			std::cerr << "failed to write manifest\n";
			result = -1;
		}
	}

	glslang::FinalizeProcess();
	return result;
}

std::string MaybeSlash(const std::string_view& path)
{
	return (path.empty() || path.back() == '/') ? "" : "/";
}

std::string file_finder::findReadableFilepath(const std::string& filename) const
{
	static const auto for_reading = std::ios_base::in;
	std::filebuf opener;
	for (const auto& prefix : _searchPath)
	{
		const std::string prefixed_filename = prefix + MaybeSlash(prefix) + filename;
		if (opener.open(prefixed_filename, for_reading))
			return prefixed_filename;
	}
	return "";
}

std::string file_finder::FindRelativeReadableFilepath(const std::string& requesting_file,
													  const std::string& filename) const
{
	std::string dir_name(requesting_file);

	size_t last_slash = requesting_file.find_last_of("/\\");
	if (last_slash != std::string::npos)
	{
		dir_name =
			std::string(requesting_file.c_str(), requesting_file.c_str() + last_slash);
	}

	if (dir_name.size() == requesting_file.size())
	{
		dir_name.clear();
	}

	static const auto for_reading = std::ios_base::in;
	std::filebuf opener;
	const std::string relative_filename = dir_name + MaybeSlash(dir_name) + filename;
	if (opener.open(relative_filename, for_reading))
		return relative_filename;

	return findReadableFilepath(filename);
}
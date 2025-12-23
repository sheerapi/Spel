#include "shaderc/shaderc.hpp"
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <libgen.h>
#include <optional>
#include <shaderc/env.h>
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
	shaderc_shader_kind kind;
	std::string suffix;
};

auto stage_from_char(char stage) -> std::optional<stage_config>
{
	switch (stage)
	{
	case 'v':
		return stage_config{shaderc_vertex_shader, "vert"};
	case 'f':
		return stage_config{shaderc_fragment_shader, "frag"};
	case 'g':
		return stage_config{shaderc_geometry_shader, "geo"};
	case 'c':
		return stage_config{shaderc_compute_shader, "comp"};
	case 't':
		return stage_config{shaderc_tess_control_shader, "tesc"};
	case 'e':
		return stage_config{shaderc_tess_evaluation_shader, "tese"};
	default:
		return std::nullopt;
	}
}

auto stage_from_name(std::string stage_name) -> std::optional<stage_config>
{
	std::transform(stage_name.begin(), stage_name.end(), stage_name.begin(),
				   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

	static const std::unordered_map<std::string, stage_config> stage_map = {
		{"vertex", {shaderc_vertex_shader, "vert"}},
		{"vert", {shaderc_vertex_shader, "vert"}},
		{"fragment", {shaderc_fragment_shader, "frag"}},
		{"frag", {shaderc_fragment_shader, "frag"}},
		{"pixel", {shaderc_fragment_shader, "frag"}},
		{"geometry", {shaderc_geometry_shader, "geo"}},
		{"geo", {shaderc_geometry_shader, "geo"}},
		{"compute", {shaderc_compute_shader, "comp"}},
		{"comp", {shaderc_compute_shader, "comp"}},
		{"tesscontrol", {shaderc_tess_control_shader, "tesc"}},
		{"tess_control", {shaderc_tess_control_shader, "tesc"}},
		{"tesc", {shaderc_tess_control_shader, "tesc"}},
		{"tessevaluation", {shaderc_tess_evaluation_shader, "tese"}},
		{"tess_evaluation", {shaderc_tess_evaluation_shader, "tese"}},
		{"tese", {shaderc_tess_evaluation_shader, "tese"}},
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

		const size_t next_line_start = (line_end == source.size()) ? source.size() : line_end + 1;

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
	// Searches for a read-openable file based on filename, which must be
	// non-empty.  The search is attempted on filename prefixed by each element of
	// search_path() in turn.  The first hit is returned, or an empty string if
	// there are no hits.  Search attempts treat their argument the way
	// std::fopen() treats its filename argument, ignoring whether the path is
	// absolute or relative.
	//
	// If a search_path() element is non-empty and not ending in a slash, then a
	// slash is inserted between it and filename before its search attempt. An
	// empty string in search_path() means that the filename is tried as-is.
	auto findReadableFilepath(const std::string& filename) const -> std::string;

	// Searches for a read-openable file based on filename, which must be
	// non-empty. The search is first attempted as a path relative to
	// the requesting_file parameter. If no file is found relative to the
	// requesting_file then this acts as FindReadableFilepath does. If
	// requesting_file does not contain a '/' or a '\' character then it is
	// assumed to be a filename and the request will be relative to the
	// current directory.
	std::string FindRelativeReadableFilepath(const std::string& requesting_file,
											 const std::string& filename) const;

	// Search path for Find().  Users may add/remove elements as desired.
	auto searchPath() -> std::vector<std::string>&
	{
		return _searchPath;
	}

private:
	std::vector<std::string> _searchPath;
};

class file_includer : public shaderc::CompileOptions::IncluderInterface
{
public:
	explicit file_includer(const file_finder* file_finder) : file_finder_(*file_finder)
	{
	}

	~file_includer() override;

	// Resolves a requested source file of a given type from a requesting
	// source into a shaderc_include_result whose contents will remain valid
	// until it's released.
	shaderc_include_result* GetInclude(const char* requested_source,
									   shaderc_include_type type,
									   const char* requesting_source,
									   size_t include_depth) override;
	// Releases an include result.
	void ReleaseInclude(shaderc_include_result* include_result) override;

	// Returns a reference to the member storing the set of included files.
	const std::unordered_set<std::string>& file_path_trace() const
	{
		return included_files_;
	}

private:
	// Used by GetInclude() to get the full filepath.
	const file_finder& file_finder_;
	// The full path and content of a source file.
	struct FileInfo
	{
		const std::string full_path;
		std::vector<char> contents;
	};

	// The set of full paths of included files.
	std::unordered_set<std::string> included_files_;
};

auto main(int argc, const char** argv) -> int
{
	if (argc < 2)
	{
		fprintf(stderr, "usage: %s <input> <output>\n", argv[0]);
		return -1;
	}

	const char* filename = argv[1];
	std::vector<std::string> defines;
	std::vector<std::string> includes;

	FILE* file = fopen(filename, "r");
	if (file == NULL)
	{
		perror(filename);
		return -1;
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

	char optimization = 'p';
	char stage = 'v';
	char language = 'g';
	bool warningsAsErrors{false};

	const char* entry = "main";

	std::string output;

	if (args_has(argc, argv, "-O") != -1)
	{
		optimization = argv[args_has(argc, argv, "-O")][2];
	}

	if (args_has(argc, argv, "-s") != -1)
	{
		stage = argv[args_has(argc, argv, "-s")][2];
	}
	else
	{
		auto name =
			string_split(std::filesystem::path(filename).filename().string(), ".");

		std::string extension;
		if (name.size() >= 3)
		{
			extension = name[name.size() - 2];

			if (name[name.size() - 1] == "hlsl")
			{
				language = 'h';
			}
		}
		else
		{
			extension = name[name.size() - 1];
		}

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

	if (args_has(argc, argv, "-o") != -1)
	{
		output = (char*)argv[args_has(argc, argv, "-o") + 1];
	}
	else
	{
		output = std::filesystem::path(filename).filename().replace_extension("spv");
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
		entry = (char*)argv[args_has(argc, argv, "-e") + 1];
	}

	shaderc::Compiler compiler;
	shaderc::CompileOptions options;

	options.SetSourceLanguage(language == 'g' ? shaderc_source_language_glsl
											  : shaderc_source_language_hlsl);

	options.SetAutoMapLocations(true);

	if (warningsAsErrors)
	{
		options.SetWarningsAsErrors();
	}

	switch (optimization)
	{
	case '0':
		options.SetOptimizationLevel(shaderc_optimization_level_zero);
		break;
	case 'p':
		options.SetOptimizationLevel(shaderc_optimization_level_performance);
		break;
	case 's':
	case 'z':
		options.SetOptimizationLevel(shaderc_optimization_level_size);
		break;
	default:
		break;
	}

	shaderc_shader_kind kind;
	switch (stage)
	{
	case 'v':
	default:
		kind = shaderc_vertex_shader;
		break;
	case 'f':
		kind = shaderc_fragment_shader;
		break;
	case 'g':
		kind = shaderc_geometry_shader;
		break;
	case 'c':
		kind = shaderc_compute_shader;
		break;
	case 't':
		kind = shaderc_tess_control_shader;
		break;
	case 'e':
		kind = shaderc_tess_evaluation_shader;
		break;
	}

	file_finder finder;

	for (auto& folder : includes)
	{
		finder.searchPath().push_back(folder);
	}

	for (auto& def : defines)
	{
		auto definition = string_split(def, "=");

		if (definition.size() == 2)
		{
			options.AddMacroDefinition(definition[0], definition[1]);
		}
		else
		{
			options.AddMacroDefinition(definition[0]);
		}
	}
	options.SetIncluder(std::make_unique<file_includer>(&finder));

	const auto stage_sections = find_stage_sections(source);
	const bool has_multi_stage = stage_sections.sections.size() > 1;
	const bool has_explicit_stage = stage_sections.sections.size() > 0;

	const auto build_output_path =
		[&](const std::string& stage_suffix, bool multi_stage_output) -> std::filesystem::path {
		if (!multi_stage_output && args_has(argc, argv, "-o") != -1)
		{
			return std::filesystem::path(output);
		}

		auto input_path = std::filesystem::path(filename).filename();
		if (multi_stage_output)
		{
			if (args_has(argc, argv, "-o") != -1)
			{
				std::filesystem::path out_path(output);
				auto base_dir = out_path.parent_path();
				auto stem = out_path.stem().string();
				auto ext = out_path.extension().string();

				if (ext.empty())
				{
					ext = ".spv";
				}

				return base_dir / std::filesystem::path(stem + "." + stage_suffix + ext);
			}

			auto stem = input_path.stem().string();
			return std::filesystem::path(stem + "." + stage_suffix + ".spv");
		}

		return input_path.replace_extension("spv");
	};

	auto compile_and_write = [&](const std::string& shader_source, const stage_config& stage_info,
								 const std::filesystem::path& output_path) -> bool {
		auto result = compiler.CompileGlslToSpv(shader_source.c_str(), shader_source.size(),
												stage_info.kind, filename, entry, options);

		if (result.GetCompilationStatus() != shaderc_compilation_status_success)
		{
			std::cerr << result.GetErrorMessage() << "\n";
			return false;
		}

		std::ofstream f(output_path, std::ios::binary | std::ios::out);
		if (!f.is_open())
		{
			std::cerr << "Failed to open output file: " << output_path << "\n";
			return false;
		}

		const auto byte_count =
			static_cast<std::streamsize>((result.cend() - result.cbegin()) * sizeof(uint32_t));

		f.write(reinterpret_cast<const char*>(&(*result.cbegin())), byte_count);
		return true;
	};

	if (has_explicit_stage)
	{
		for (const auto& section : stage_sections.sections)
		{
			auto stage_info = stage_from_name(section.stage_name);

			if (!stage_info.has_value())
			{
				std::cerr << "Unknown shader stage in pragma: " << section.stage_name << "\n";
				return -1;
			}

			std::string stage_source = source.substr(0, stage_sections.prefix_length);
			stage_source.append(source.substr(section.start, section.end - section.start));

			auto output_path = build_output_path(stage_info->suffix,
												 has_multi_stage || has_explicit_stage);

			if (!compile_and_write(stage_source, *stage_info, output_path))
			{
				return -1;
			}
		}
	}
	else
	{
		auto stage_info = stage_from_char(stage).value_or(stage_config{kind, ""});
		auto output_path = build_output_path(stage_info.suffix, false);

		if (!compile_and_write(source, stage_info, output_path))
		{
			return -1;
		}
	}

	return 0;
}

shaderc_include_result* MakeErrorIncludeResult(const char* message)
{
	return new shaderc_include_result{"", 0, message, strlen(message)};
}

file_includer::~file_includer() = default;

bool ReadFile(const std::string& input_file_name, std::vector<char>* input_data)
{
	std::istream* stream = &std::cin;
	std::ifstream input_file;
	if (input_file_name != "-")
	{
		input_file.open(input_file_name, std::ios_base::binary);
		stream = &input_file;
		if (input_file.fail())
		{
			std::cerr << "glslc: error: cannot open input file: '" << input_file_name
					  << "'";
			std::cerr << '\n';
			return false;
		}
	}
	*input_data = std::vector<char>((std::istreambuf_iterator<char>(*stream)),
									std::istreambuf_iterator<char>());
	return true;
}

shaderc_include_result* file_includer::GetInclude(const char* requested_source,
												 shaderc_include_type include_type,
												 const char* requesting_source, size_t)
{

	const std::string full_path =
		(include_type == shaderc_include_type_relative)
			? file_finder_.FindRelativeReadableFilepath(requesting_source,
														requested_source)
			: file_finder_.findReadableFilepath(requested_source);

	if (full_path.empty())
		return MakeErrorIncludeResult("Cannot find or open include file.");

	// In principle, several threads could be resolving includes at the same
	// time.  Protect the included_files.

	// Read the file and save its full path and contents into stable addresses.
	FileInfo* new_file_info = new FileInfo{full_path, {}};
	if (!ReadFile(full_path, &(new_file_info->contents)))
	{
		return MakeErrorIncludeResult("Cannot read file");
	}

	included_files_.insert(full_path);

	return new shaderc_include_result{
		new_file_info->full_path.data(), new_file_info->full_path.length(),
		new_file_info->contents.data(), new_file_info->contents.size(), new_file_info};
}

void file_includer::ReleaseInclude(shaderc_include_result* include_result)
{
	FileInfo* info = static_cast<FileInfo*>(include_result->user_data);
	delete info;
	delete include_result;
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

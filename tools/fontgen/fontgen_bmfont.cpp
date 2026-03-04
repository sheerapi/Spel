#include "../fontgen.h"

#include <fstream>
#include <sstream>
#include <unordered_map>

#include "stb_image.h"
#include "../stb_image_write.h"

struct BMChar
{
	uint32_t id;
	int x, y, width, height;
	int xoffset, yoffset;
	int xadvance;
	int page;
};

struct BMKerning
{
	uint32_t first, second;
	int amount;
};

struct BMFont
{
	int atlas_w, atlas_h;
	float line_height;
	float base;
	std::vector<std::string> pages;
	std::vector<BMChar> chars;
	std::vector<BMKerning> kerning;
};

static std::unordered_map<std::string, std::string> parse_kv(const std::string& line)
{
	std::unordered_map<std::string, std::string> kv;
	std::istringstream ss(line);
	std::string token;
	while (ss >> token)
	{
		auto eq = token.find('=');
		if (eq == std::string::npos)
			continue;
		std::string key = token.substr(0, eq);
		std::string val = token.substr(eq + 1);

		if (val.size() >= 2 && val.front() == '"' && val.back() == '"')
			val = val.substr(1, val.size() - 2);
		kv[key] = val;
	}
	return kv;
}

static int kv_int(const std::unordered_map<std::string, std::string>& kv, const char* key,
				  int def = 0)
{
	auto it = kv.find(key);
	return it != kv.end() ? std::stoi(it->second) : def;
}

static std::string kv_str(const std::unordered_map<std::string, std::string>& kv,
						  const char* key, const char* def = "")
{
	auto it = kv.find(key);
	return it != kv.end() ? it->second : def;
}

static bool parse_text(const std::string& path, BMFont& out)
{
	std::ifstream f(path);
	if (!f)
		return false;

	std::string line;
	while (std::getline(f, line))
	{
		if (line.empty())
			continue;

		std::istringstream ls(line);
		std::string tag;
		ls >> tag;
		std::string rest;
		std::getline(ls, rest);
		auto kv = parse_kv(rest);

		if (tag == "common")
		{
			out.atlas_w = kv_int(kv, "scaleW");
			out.atlas_h = kv_int(kv, "scaleH");
			out.line_height = (float)kv_int(kv, "lineHeight");
			out.base = (float)kv_int(kv, "base");
		}
		else if (tag == "page")
		{
			int id = kv_int(kv, "id");
			if (id >= (int)out.pages.size())
				out.pages.resize(id + 1);
			out.pages[id] = kv_str(kv, "file");
		}
		else if (tag == "char")
		{
			BMChar c;
			c.id = (uint32_t)kv_int(kv, "id");
			c.x = kv_int(kv, "x");
			c.y = kv_int(kv, "y");
			c.width = kv_int(kv, "width");
			c.height = kv_int(kv, "height");
			c.xoffset = kv_int(kv, "xoffset");
			c.yoffset = kv_int(kv, "yoffset");
			c.xadvance = kv_int(kv, "xadvance");
			c.page = kv_int(kv, "page");
			out.chars.push_back(c);
		}
		else if (tag == "kerning")
		{
			BMKerning k;
			k.first = (uint32_t)kv_int(kv, "first");
			k.second = (uint32_t)kv_int(kv, "second");
			k.amount = kv_int(kv, "amount");
			if (k.amount != 0)
				out.kerning.push_back(k);
		}
	}
	return !out.chars.empty();
}

static std::string xml_attr(const std::string& tag, const char* attr)
{
	std::string key = std::string(attr) + "=\"";
	auto pos = tag.find(key);
	if (pos == std::string::npos)
		return "";
	pos += key.size();
	auto end = tag.find('"', pos);
	return tag.substr(pos, end - pos);
}

static int xml_int(const std::string& tag, const char* attr, int def = 0)
{
	auto v = xml_attr(tag, attr);
	return v.empty() ? def : std::stoi(v);
}

static bool parse_xml(const std::string& path, BMFont& out)
{
	std::ifstream f(path);
	if (!f)
		return false;

	std::string content((std::istreambuf_iterator<char>(f)),
						std::istreambuf_iterator<char>());

	size_t pos = 0;
	while (pos < content.size())
	{
		auto open = content.find('<', pos);
		if (open == std::string::npos)
			break;
		auto close = content.find('>', open);
		if (close == std::string::npos)
			break;

		std::string tag = content.substr(open + 1, close - open - 1);
		pos = close + 1;

		std::string name;
		std::istringstream ts(tag);
		ts >> name;

		if (name == "common")
		{
			out.atlas_w = xml_int(tag, "scaleW");
			out.atlas_h = xml_int(tag, "scaleH");
			out.line_height = (float)xml_int(tag, "lineHeight");
			out.base = (float)xml_int(tag, "base");
		}
		else if (name == "page")
		{
			int id = xml_int(tag, "id");
			if (id >= (int)out.pages.size())
				out.pages.resize(id + 1);
			out.pages[id] = xml_attr(tag, "file");
		}
		else if (name == "char")
		{
			BMChar c;
			c.id = (uint32_t)xml_int(tag, "id");
			c.x = xml_int(tag, "x");
			c.y = xml_int(tag, "y");
			c.width = xml_int(tag, "width");
			c.height = xml_int(tag, "height");
			c.xoffset = xml_int(tag, "xoffset");
			c.yoffset = xml_int(tag, "yoffset");
			c.xadvance = xml_int(tag, "xadvance");
			c.page = xml_int(tag, "page");
			out.chars.push_back(c);
		}
		else if (name == "kerning")
		{
			BMKerning k;
			k.first = (uint32_t)xml_int(tag, "first");
			k.second = (uint32_t)xml_int(tag, "second");
			k.amount = xml_int(tag, "amount");
			if (k.amount != 0)
				out.kerning.push_back(k);
		}
	}
	return !out.chars.empty();
}

static bool parse_bmfont(const std::string& path, BMFont& out)
{

	FILE* f = fopen(path.c_str(), "rb");
	if (!f)
		return false;
	char buf[6] = {};
	fread(buf, 1, 5, f);
	fclose(f);

	bool is_xml = (buf[0] == '<');
	return is_xml ? parse_xml(path, out) : parse_text(path, out);
}

struct StitchedAtlas
{
	std::vector<uint8_t> pixels;
	int width, height;

	std::vector<int> page_offsets_x;
};

static bool stitch_pages(const std::string& fnt_dir,
						 const std::vector<std::string>& pages, StitchedAtlas& out)
{
	struct Page
	{
		uint8_t* px;
		int w, h;
	};
	std::vector<Page> loaded;

	int total_w = 0, max_h = 0;
	for (auto& name : pages)
	{
		std::string full = fnt_dir + "/" + name;
		int w, h, ch;
		uint8_t* px = stbi_load(full.c_str(), &w, &h, &ch, 4);
		if (!px)
		{
			fprintf(stderr, "Failed to load page: %s\n", full.c_str());
			for (auto& p : loaded)
				stbi_image_free(p.px);
			return false;
		}
		loaded.push_back({px, w, h});
		out.page_offsets_x.push_back(total_w);
		total_w += w;
		max_h = std::max(max_h, h);
	}

	out.width = total_w;
	out.height = max_h;
	out.pixels.assign(total_w * max_h * 4, 0);

	for (int p = 0; p < (int)loaded.size(); p++)
	{
		int ox = out.page_offsets_x[p];
		for (int y = 0; y < loaded[p].h; y++)
		{
			memcpy(out.pixels.data() + (y * total_w + ox) * 4,
				   loaded[p].px + y * loaded[p].w * 4, loaded[p].w * 4);
		}
		stbi_image_free(loaded[p].px);
	}
	return true;
}

bool fontgen_bmfont(const BMFontConfig& cfg)
{
	BMFont bm;
	if (!parse_bmfont(cfg.input, bm))
	{
		fprintf(stderr, "Failed to parse BMFont: %s\n", cfg.input.c_str());
		return false;
	}

	printf("BMFont: %d glyphs, %d pages, %d kerning pairs\n", (int)bm.chars.size(),
		   (int)bm.pages.size(), (int)bm.kerning.size());

	std::string fnt_dir = cfg.input.substr(0, cfg.input.find_last_of("/\\"));
	StitchedAtlas atlas;
	if (!stitch_pages(fnt_dir, bm.pages, atlas))
		return false;

	std::vector<spel_font_glyph> glyphs;
	glyphs.reserve(bm.chars.size());

	float aw = (float)atlas.width;
	float ah = (float)atlas.height;
	float em = bm.line_height;

	for (auto& c : bm.chars)
	{
		spel_font_glyph g = {};
		g.codepoint = c.id;

		int page_ox = atlas.page_offsets_x[c.page];

		g.uv_x = (page_ox + c.x) / aw;
		g.uv_y = c.y / ah;
		g.uv_w = c.width / aw;
		g.uv_h = c.height / ah;

		g.plane_x = c.xoffset / em;
		g.plane_y = c.yoffset / em;
		g.plane_w = c.width / em;
		g.plane_h = c.height / em;
		g.advance = c.xadvance / em;

		glyphs.push_back(g);
	}

	std::vector<spel_font_kerning> kerning;
	if (!cfg.skip_kerning)
	{
		kerning.reserve(bm.kerning.size());
		for (auto& k : bm.kerning)
		{
			spel_font_kerning kr;
			kr.codepoint_a = k.first;
			kr.codepoint_b = k.second;
			kr.advance = k.amount / em;
			kerning.push_back(kr);
		}
	}

	std::vector<uint8_t> png_bytes;
	stbi_write_png_to_func(
		[](void* ctx, void* data, int size)
		{
			auto* buf = (std::vector<uint8_t>*)ctx;
			buf->insert(buf->end(), (uint8_t*)data, (uint8_t*)data + size);
		},
		&png_bytes, atlas.width, atlas.height, 4, atlas.pixels.data(), atlas.width * 4);

	spel_font_header header = {};
	header.magic = SPFN_MAGIC;
	header.version = SPFN_VERSION;
	header.channels = 4;
	header.atlas_width = (uint16_t)atlas.width;
	header.atlas_height = (uint16_t)atlas.height;
	header.glyph_count = (uint16_t)glyphs.size();
	header.kerning_count = (uint16_t)kerning.size();
	header.image_size = (uint32_t)png_bytes.size();
	header.em_size = em;
	header.ascender = bm.base / em;
	header.descender = (bm.line_height - bm.base) / em;
	header.line_height = 1.0f;
	header.sdf_range = 0.0f;
	header.font_type = SPFN_TYPE_BITMAP;

	std::string out_path = cfg.output + ".spfn";
	FILE* f = fopen(out_path.c_str(), "wb");
	if (!f)
	{
		fprintf(stderr, "Failed to open: %s\n", out_path.c_str());
		return false;
	}

	fwrite(&header, sizeof(header), 1, f);
	fwrite(glyphs.data(), sizeof(spel_font_glyph), glyphs.size(), f);
	fwrite(kerning.data(), sizeof(spel_font_kerning), kerning.size(), f);
	fwrite(png_bytes.data(), 1, png_bytes.size(), f);
	fclose(f);

	printf("Wrote %s (%zu glyphs, %zu kerning pairs, %dx%d atlas)\n", out_path.c_str(),
		   glyphs.size(), kerning.size(), atlas.width, atlas.height);
	return true;
}


#define STB_RECT_PACK_IMPLEMENTATION
#include "stb_rect_pack.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include "fontgen.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

static constexpr int GLYPH_PADDING = 2;
static constexpr int ATLAS_MAX_H = 65536;
static constexpr int CP_MAX = 0x10FFFF;

[[noreturn]] static void die(const char* msg)
{
	std::fprintf(stderr, "error: %s\n", msg);
	std::exit(1);
}

static std::vector<unsigned char> read_file(const char* path)
{
	FILE* f = std::fopen(path, "rb");
	if (!f)
	{
		std::fprintf(stderr, "error: cannot open '%s'\n", path);
		std::exit(1);
	}
	std::fseek(f, 0, SEEK_END);
	long sz = std::ftell(f);
	std::rewind(f);
	std::vector<unsigned char> buf(static_cast<size_t>(sz));
	if (std::fread(buf.data(), 1, sz, f) != static_cast<size_t>(sz))
		die("short read on font file");
	std::fclose(f);
	return buf;
}

static std::string replace_ext(const std::string& path, const std::string& ext)
{
	auto dot = path.rfind('.');
	auto sep = path.find_last_of("/\\");
	if (dot != std::string::npos && (sep == std::string::npos || dot > sep))
		return path.substr(0, dot) + ext;
	return path + ext;
}

static bool parse_int(const char* s, int& out)
{
	char* end = nullptr;
	long v = std::strtol(s, &end, 0);
	if (end == s || *end != '\0')
		return false;
	out = static_cast<int>(v);
	return true;
}

struct GlyphInfo
{
	int codepoint = 0;
	int glyph_idx = 0;
	int bmp_w = 0;
	int bmp_h = 0;
	int xoff = 0;
	int yoff = 0;
	int advance = 0;
	int cell_x = 0;
	int cell_y = 0;
	std::vector<unsigned char> bitmap;
};

static std::vector<int> enumerate_mapped(const stbtt_fontinfo& font, int cp_first,
										 int cp_last)
{
	std::vector<int> result;
	const bool show_progress = (cp_last - cp_first) > 65535;
	const int milestone = (cp_last - cp_first + 1) / 20;

	for (int cp = cp_first; cp <= cp_last; ++cp)
	{
		if (show_progress && milestone > 0 && ((cp - cp_first) % milestone == 0))
		{
			int pct = (int)(100LL * (cp - cp_first) / (cp_last - cp_first + 1));
			std::fprintf(stderr, "\r  scanning cmap ... %3d%%", pct);
			std::fflush(stderr);
		}
		if (stbtt_FindGlyphIndex(&font, cp) > 0)
			result.push_back(cp);
	}
	if (show_progress)
		std::fprintf(stderr, "\r  scanning cmap ... 100%%\n");
	return result;
}

int main(int argc, char* argv[])
{
	if (argc < 2)
	{
		std::fprintf(stderr,
				 "usage: %s <font.ttf> [-o out.png] [-s font_px] [-w atlas_w] [-r "
				 "first last] [-p padding] [--spfn]\n"
				 "  -r accepts decimal or 0x-prefixed hex  e.g.  -r 0x0020 0x04FF\n"
				 "  -p is 2 by default\n"
				 "  --spfn  also emit packed .spfn/.png using stb_rect_pack\n",
				 argv[0]);
		return 1;
	}

	const char* font_path = argv[1];
	std::string out_png = replace_ext(font_path, "_atlas.png");
	float font_size = 32.0f;
	int atlas_w = 1024;
	int cp_first = 0;
	int cp_last = CP_MAX;
	int padding = GLYPH_PADDING;
	bool spfn_mode = false;

	for (int i = 2; i < argc; ++i)
	{
		if (std::strcmp(argv[i], "-o") == 0 && i + 1 < argc)
			out_png = argv[++i];
		else if (std::strcmp(argv[i], "-s") == 0 && i + 1 < argc)
			font_size = static_cast<float>(std::atof(argv[++i]));
		else if (std::strcmp(argv[i], "-w") == 0 && i + 1 < argc)
			atlas_w = std::atoi(argv[++i]);
		else if (std::strcmp(argv[i], "-p") == 0 && i + 1 < argc)
			padding = std::atoi(argv[++i]);
		else if (std::strcmp(argv[i], "-r") == 0 && i + 2 < argc)
		{
			if (!parse_int(argv[i + 1], cp_first) || !parse_int(argv[i + 2], cp_last))
				die("invalid -r arguments (use decimal or 0x hex)");
			i += 2;
		}
		else if (std::strcmp(argv[i], "--spfn") == 0)
			spfn_mode = true;
		else
		{
			std::fprintf(stderr, "unknown option: %s\n", argv[i]);
			return 1;
		}
	}

	if (font_size < 2.f || font_size > 512.f)
		die("-s font_size must be in [2, 512]");
	if (atlas_w < 32 || atlas_w > 16384)
		die("-w atlas_width must be in [32, 16384]");
	if (cp_first < 0 || cp_last > CP_MAX || cp_first > cp_last)
		die("-r range invalid");

	auto font_data = read_file(font_path);

	stbtt_fontinfo font{};
	if (!stbtt_InitFont(&font, font_data.data(), 0))
		die("stbtt_InitFont failed - is this a valid TTF/OTF?");

	const float scale = stbtt_ScaleForPixelHeight(&font, font_size);

	int ascent_i, descent_i, line_gap_i;
	stbtt_GetFontVMetrics(&font, &ascent_i, &descent_i, &line_gap_i);
	const int ascent = static_cast<int>(ascent_i * scale + 0.5f);
	const int descent = static_cast<int>(descent_i * scale - 0.5f);

	std::fprintf(stderr, "font   : %s\n", font_path);
	std::fprintf(stderr, "range  : U+%04X .. U+%04X\n", cp_first, cp_last);

	std::vector<int> mapped = enumerate_mapped(font, cp_first, cp_last);
	if (mapped.empty())
		die("no glyphs found in the requested codepoint range");
	std::fprintf(stderr, "mapped : %zu codepoints have glyphs\n", mapped.size());

	std::vector<GlyphInfo> glyphs;
	glyphs.reserve(mapped.size());

	for (int cp : mapped)
	{
		GlyphInfo g;
		g.codepoint = cp;
		g.glyph_idx = stbtt_FindGlyphIndex(&font, cp);

		int adv_i, lsb_i;
		stbtt_GetGlyphHMetrics(&font, g.glyph_idx, &adv_i, &lsb_i);
		g.advance = static_cast<int>(adv_i * scale + 0.5f);

		unsigned char* bmp = stbtt_GetGlyphBitmap(&font, 0, scale, g.glyph_idx, &g.bmp_w,
												  &g.bmp_h, &g.xoff, &g.yoff);

		if (bmp && g.bmp_w > 0 && g.bmp_h > 0)
		{
			g.bitmap.assign(bmp, bmp + g.bmp_w * g.bmp_h);
			stbtt_FreeBitmap(bmp, nullptr);
			glyphs.push_back(std::move(g));
		}
		else
		{
			if (bmp)
				stbtt_FreeBitmap(bmp, nullptr);
		}
	}

	if (glyphs.empty())
		die("no glyphs with visible bitmaps in the requested range");

	const int N = static_cast<int>(glyphs.size());
	std::fprintf(stderr, "visible: %d glyphs with bitmaps\n", N);

	constexpr int CH = 4;
	const float em_size = static_cast<float>(ascent - descent);

	if (spfn_mode)
	{
		std::fprintf(stderr, "mode   : stb_rect_pack -> .spfn\n");

		std::vector<stbrp_rect> rects(N);
		for (int i = 0; i < N; ++i)
		{
			rects[i].id = i;
			rects[i].w = glyphs[i].bmp_w + padding * 2;
			rects[i].h = glyphs[i].bmp_h + padding * 2;
			if (rects[i].w > atlas_w)
				die("a glyph exceeds atlas width; increase -w or reduce -s");
		}

		std::vector<stbrp_node> nodes(static_cast<size_t>(atlas_w));
		stbrp_context ctx;
		stbrp_init_target(&ctx, atlas_w, ATLAS_MAX_H, nodes.data(), atlas_w);
		stbrp_pack_rects(&ctx, rects.data(), N);

		int atlas_h = 0;
		bool all_packed = true;
		for (const auto& r : rects)
		{
			if (!r.was_packed)
			{
				all_packed = false;
				break;
			}
			atlas_h = std::max(atlas_h, r.y + r.h);
		}
		if (!all_packed || atlas_h <= 0 || atlas_h > ATLAS_MAX_H)
			die("stb_rect_pack failed - atlas too small; increase -w");

		std::fprintf(stderr, "atlas  : %d x %d px (packed)\n", atlas_w, atlas_h);

		std::vector<unsigned char> atlas_buf(
			static_cast<size_t>(atlas_w) * atlas_h * CH, 0);

		for (const auto& r : rects)
		{
			const auto& g = glyphs[r.id];
			const int dst_x = r.x + padding;
			const int dst_y = r.y + padding;

			for (int row = 0; row < g.bmp_h; ++row)
				for (int col = 0; col < g.bmp_w; ++col)
				{
					const int ax = dst_x + col;
					const int ay = dst_y + row;
					const int dst = (ay * atlas_w + ax) * CH;
					atlas_buf[dst + 0] = 255;
					atlas_buf[dst + 1] = 255;
					atlas_buf[dst + 2] = 255;
					atlas_buf[dst + 3] = g.bitmap[row * g.bmp_w + col];
				}
		}

		std::vector<unsigned char> png_bytes;
		stbi_write_png_to_func(
			[](void* ctx, void* data, int size)
			{
				auto* buf = static_cast<std::vector<unsigned char>*>(ctx);
				buf->insert(buf->end(), static_cast<unsigned char*>(data),
							static_cast<unsigned char*>(data) + size);
			},
			&png_bytes, atlas_w, atlas_h, CH, atlas_buf.data(), atlas_w * CH);

		if (FILE* pf = std::fopen(out_png.c_str(), "wb"))
		{
			if (!png_bytes.empty())
				std::fwrite(png_bytes.data(), 1, png_bytes.size(), pf);
			std::fclose(pf);
		}
		else
			die("failed to open output png");

		std::fprintf(stderr, "wrote  %s\n", out_png.c_str());

		std::vector<spel_font_glyph> spfn_glyphs;
		spfn_glyphs.reserve(glyphs.size());

		for (const auto& r : rects)
		{
			const auto& g = glyphs[r.id];
			spel_font_glyph rec{};
			rec.codepoint = static_cast<uint32_t>(g.codepoint);
			rec.uv_x = static_cast<float>(r.x + padding) / static_cast<float>(atlas_w);
			rec.uv_y = static_cast<float>(r.y + padding) / static_cast<float>(atlas_h);
			rec.uv_w = static_cast<float>(g.bmp_w) / static_cast<float>(atlas_w);
			rec.uv_h = static_cast<float>(g.bmp_h) / static_cast<float>(atlas_h);
			rec.plane_x = static_cast<float>(g.xoff) / em_size;
			rec.plane_y = static_cast<float>(g.yoff) / em_size;
			rec.plane_w = static_cast<float>(g.bmp_w) / em_size;
			rec.plane_h = static_cast<float>(g.bmp_h) / em_size;
			rec.advance = static_cast<float>(g.advance) / em_size;
			spfn_glyphs.push_back(rec);
		}

		const std::string out_spfn = replace_ext(out_png, ".spfn");

		spel_font_header header{};
		header.magic = SPFN_MAGIC;
		header.version = SPFN_VERSION;
		header.channels = 4;
		header.font_type = SPFN_TYPE_BITMAP;
		header.atlas_width = static_cast<uint16_t>(atlas_w);
		header.atlas_height = static_cast<uint16_t>(atlas_h);
		header.glyph_count = static_cast<uint16_t>(spfn_glyphs.size());
		header.kerning_count = 0;
		header.image_size = static_cast<uint32_t>(png_bytes.size());
		header.em_size = em_size;
		header.ascender = static_cast<float>(ascent) / em_size;
		header.descender = static_cast<float>(-descent) / em_size;
		header.line_height = 1.0f;
		header.sdf_range = 0.0f;

		if (FILE* f = std::fopen(out_spfn.c_str(), "wb"))
		{
			std::fwrite(&header, sizeof(header), 1, f);
			std::fwrite(spfn_glyphs.data(), sizeof(spel_font_glyph), spfn_glyphs.size(),
						f);
			if (!png_bytes.empty())
				std::fwrite(png_bytes.data(), 1, png_bytes.size(), f);
			std::fclose(f);
		}
		else
			die("failed to open output spfn");

		std::fprintf(stderr, "wrote  %s\n", out_spfn.c_str());
		std::fprintf(stderr, "done.\n");
		return 0;
	}
	else
	{
		int max_bmp_w = 0;
		for (const auto& g : glyphs)
			max_bmp_w = std::max(max_bmp_w, g.bmp_w);

		const int cell_w = max_bmp_w + padding;
		const int cell_h = (ascent - descent) + padding;

		std::fprintf(stderr, "cell   : %d x %d px  (max_bmp_w=%d, line_h=%d)\n", cell_w,
					 cell_h, max_bmp_w, ascent - descent);

		const int cols = std::max(1, atlas_w / cell_w);
		const int actual_atlas_w = cols * cell_w;
		const int rows = (N + cols - 1) / cols;
		const int atlas_h = rows * cell_h;

		if (atlas_h > ATLAS_MAX_H)
			die("atlas would exceed ATLAS_MAX_H - use a smaller -s, wider -w, or "
				"narrower -r");

		std::fprintf(stderr, "grid   : %d cols x %d rows\n", cols, rows);
		std::fprintf(stderr, "atlas  : %d x %d px\n", actual_atlas_w, atlas_h);

		for (int i = 0; i < N; ++i)
		{
			glyphs[i].cell_x = (i % cols) * cell_w;
			glyphs[i].cell_y = (i / cols) * cell_h;
		}

		std::vector<unsigned char> atlas_buf(
			static_cast<size_t>(actual_atlas_w) * atlas_h * CH, 0);

		for (const auto& g : glyphs)
		{
			const int pad_x = (cell_w - g.bmp_w) / 2;
			const int pad_y = (cell_h - g.bmp_h) / 2;

			for (int row = 0; row < g.bmp_h; ++row)
				for (int col = 0; col < g.bmp_w; ++col)
				{
					const int ax = g.cell_x + pad_x + col;
					const int ay = g.cell_y + pad_y + row;
					if (ax >= actual_atlas_w || ay >= atlas_h)
						continue;

					const int dst = (ay * actual_atlas_w + ax) * CH;
					atlas_buf[dst + 0] = 255;
					atlas_buf[dst + 1] = 255;
					atlas_buf[dst + 2] = 255;
					atlas_buf[dst + 3] = g.bitmap[row * g.bmp_w + col];
				}
		}

		if (!stbi_write_png(out_png.c_str(), actual_atlas_w, atlas_h, CH,
							atlas_buf.data(), actual_atlas_w * CH))
			die("stbi_write_png failed");

		std::fprintf(stderr, "wrote  %s\n", out_png.c_str());

		const std::string out_txt = replace_ext(out_png, ".txt");
		if (FILE* meta = std::fopen(out_txt.c_str(), "w"))
		{
			std::fprintf(
				meta,
				"# font_atlas metadata\n"
				"# font        : %s\n"
				"# font_size   : %.1f px\n"
				"# atlas       : %d x %d\n"
				"# cp_range    : U+%04X .. U+%04X\n"
				"# ascent      : %d   descent : %d\n"
				"# cell        : %d x %d px\n"
				"# padding     : %dpx\n"
				"# grid        : %d cols x %d rows\n"
				"# glyphs      : %d visible (whitespace/control excluded)\n"
				"#\n"
				"# cell_x/y = top-left of the cell in the atlas\n"
				"# pad_x/y  = offset of bitmap top-left within the cell (centering)\n"
				"#\n"
				"# codepoint  glyph_idx  cell_x  cell_y  pad_x  pad_y  bmp_w  bmp_h  "
				"xoff  yoff  advance\n",
				font_path, font_size, actual_atlas_w, atlas_h, cp_first, cp_last, ascent,
				descent, cell_w, cell_h, padding, cols, rows, N);

			for (const auto& g : glyphs)
			{
				const int pad_x = (cell_w - g.bmp_w) / 2;
				const int pad_y = (cell_h - g.bmp_h) / 2;
				std::fprintf(meta,
							 "  U+%06X   %6d     %5d   %5d   %4d   %4d   %4d   %4d  %4d  "
							 "%4d    %4d\n",
							 g.codepoint, g.glyph_idx, g.cell_x, g.cell_y, pad_x, pad_y,
							 g.bmp_w, g.bmp_h, g.xoff, g.yoff, g.advance);
			}
			std::fclose(meta);
			std::fprintf(stderr, "wrote  %s\n", out_txt.c_str());
		}

		std::fprintf(stderr, "done.\n");
		return 0;
	}
}

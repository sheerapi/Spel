#include "msdf-atlas-gen/FontGeometry.h"
#include "msdf-atlas-gen/GlyphGeometry.h"
#include "msdf-atlas-gen/Padding.h"
#include "msdf-atlas-gen/msdf-atlas-gen.h"

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../stb_image_write.h"

#include "../fontgen.h"

struct Config
{
	std::string input;
	std::string output;
	int atlas_size = 512;
	float em_size = 32.0f;
	float sdf_range = 4.0f;
	bool mtsdf = false;
	bool digits_only = false;
	bool kerning = false;
	int type = 0; // 0 msdf, 1 bmfont, 2 bitmap

	GridConfig grid;
	BMFontConfig bmfont;
};

static void print_usage(const char* prog)
{
	fprintf(
		stderr,
		"Usage: %s -i input -o output [-s atlas_size] [-e em_size] "
		"[-r sdf_range] [--mtsdf] [--digits-only] [--kerning] [--grid] [--grid-size "
		"<cols>x<rows>] [--cell <w>x<h>] [--first codepoint]\n"
		"\n"
		"  -i  input TTF, BMFont, or bitmap file\n"
		"  -o  output base path (produces output.spfn + output.png)\n"
		"  -s  atlas texture size in px (default: 512)\n"
		"  -e  em size in px (default: 32)\n"
		"  -r  SDF range in px (default: 4)\n"
		"  --mtsdf       use 4-channel MTSDF (better quality, larger atlas)\n"
		"  --digits-only only pack digits 0-9, colon and period\n"
		"  --kerning     include kerning pairs (larger font file)"
		"  --grid        the input file is a bitmap font (.png, see spel-ttf2png "
		"with --spfn mode)\n"
		"  --grid-size   amount of rows and columns, only valid in grid mode\n"
		"  --cell        size of every cell in grid mode, only valid in grid mode\n"
		"  --first       codepoint from where the font starts, only valid in grid mode\n",
		prog);
}

static Config parse_args(int argc, char** argv)
{
	Config cfg;
	for (int i = 1; i < argc; i++)
	{
		if (!strcmp(argv[i], "-i") && i + 1 < argc)
			cfg.input = argv[++i];
		else if (!strcmp(argv[i], "-o") && i + 1 < argc)
			cfg.output = argv[++i];
		else if (!strcmp(argv[i], "-s") && i + 1 < argc)
			cfg.atlas_size = atoi(argv[++i]);
		else if (!strcmp(argv[i], "-e") && i + 1 < argc)
			cfg.em_size = (float)atof(argv[++i]);
		else if (!strcmp(argv[i], "-r") && i + 1 < argc)
			cfg.sdf_range = (float)atof(argv[++i]);
		else if (!strcmp(argv[i], "--mtsdf"))
			cfg.mtsdf = true;
		else if (!strcmp(argv[i], "--digits-only"))
			cfg.digits_only = true;
		else if (!strcmp(argv[i], "--grid"))
			cfg.type = 2;
		else if (!strcmp(argv[i], "--grid-size"))
		{
			auto size = std::string(argv[i++]);
			cfg.grid.cols = atoi(size.substr(0, size.find('x') - 1).c_str());
			cfg.grid.rows = atoi(size.substr(size.find('x') + 1).c_str());
		}
		else if (!strcmp(argv[i], "--cell"))
		{
			auto size = std::string(argv[i++]);
			cfg.grid.cell_w = atoi(size.substr(0, size.find('x') - 1).c_str());
			cfg.grid.cell_h = atoi(size.substr(size.find('x') + 1).c_str());
		}
		else if (!strcmp(argv[i], "--first"))
		{
			char* end = nullptr;
			cfg.grid.first_codepoint = strtol(argv[i++], &end, 0);
		}
		else if (!strcmp(argv[i], "--kerning"))
			cfg.kerning = true;
		else
		{
			fprintf(stderr, "Unknown argument: %s\n", argv[i]);
			exit(1);
		}
	}
	if (cfg.input.empty() || cfg.output.empty())
	{
		print_usage(argv[0]);
		exit(1);
	}
	return cfg;
}

static std::vector<msdf_atlas::unicode_t> build_charset(bool digits_only)
{
	std::vector<msdf_atlas::unicode_t> chars;
	if (digits_only)
	{
		for (char c : std::string("0123456789:.-+ "))
			chars.push_back((msdf_atlas::unicode_t)c);
	}
	else
	{

		for (uint32_t c = 0x20; c < 0x7F; c++)
			chars.push_back(c);

		for (uint32_t c = 0xA0; c < 0x180; c++)
			chars.push_back(c);
	}
	return chars;
}

int main(int argc, char** argv)
{
	Config cfg = parse_args(argc, argv);

	if (cfg.type == 2)
	{
		cfg.grid.input = cfg.input;
		cfg.grid.output = cfg.output;
		return static_cast<int>(fontgen_grid(cfg.grid));
	}

	if (cfg.input.rfind("fnt") != std::string::npos)
	{
		cfg.bmfont.input = cfg.input;
		cfg.bmfont.output = cfg.output;
		cfg.bmfont.skip_kerning = !cfg.kerning;
		return static_cast<int>(fontgen_bmfont(cfg.bmfont));
	}

	msdfgen::FreetypeHandle* ft = msdfgen::initializeFreetype();
	if (!ft)
	{
		fprintf(stderr, "Failed to init Freetype\n");
		return 1;
	}

	msdfgen::FontHandle* font = msdfgen::loadFont(ft, cfg.input.c_str());
	if (!font)
	{
		fprintf(stderr, "Failed to load font: %s\n", cfg.input.c_str());
		msdfgen::deinitializeFreetype(ft);
		return 1;
	}

	std::vector<msdf_atlas::GlyphGeometry> glyphs;
	msdf_atlas::FontGeometry font_geometry(&glyphs);

	auto charset_cps = build_charset(cfg.digits_only);
	msdf_atlas::Charset charset;
	for (auto cp : charset_cps)
		charset.add(cp);

	int glyphs_loaded = font_geometry.loadCharset(font, 1.0, charset);
	printf("Loaded %d / %zu glyphs\n", glyphs_loaded, charset_cps.size());

	const double max_corner_angle = 3.0;
	for (auto& g : glyphs)
		g.edgeColoring(&msdfgen::edgeColoringInkTrap, max_corner_angle, 0);

	msdf_atlas::TightAtlasPacker packer;
	packer.setDimensions(cfg.atlas_size, cfg.atlas_size);
	packer.setPixelRange(cfg.sdf_range);
	packer.setMiterLimit(1.0);
	packer.setOuterPixelPadding(msdf_atlas::Padding(1));

	int remaining = packer.pack(glyphs.data(), (int)glyphs.size());
	if (remaining > 0)
	{
		fprintf(stderr,
				"Warning: %d glyphs didn't fit in %dx%d atlas. "
				"Try a larger -s value.\n",
				remaining, cfg.atlas_size, cfg.atlas_size);
	}

	int atlas_w = 0, atlas_h = 0;
	packer.getDimensions(atlas_w, atlas_h);
	printf("Atlas size: %dx%d\n", atlas_w, atlas_h);

	const int channels = cfg.mtsdf ? 4 : 3;

	msdf_atlas::GeneratorAttributes gen_attrs;
	gen_attrs.config.overlapSupport = true;
	gen_attrs.scanlinePass = true;

	int image_size = 0;
	uint8_t* image;

	if (cfg.mtsdf)
	{
		msdf_atlas::ImmediateAtlasGenerator<float, 4, msdf_atlas::mtsdfGenerator,
											msdf_atlas::BitmapAtlasStorage<float, 4>>
			generator(atlas_w, atlas_h);
		generator.setAttributes(gen_attrs);
		generator.setThreadCount(4);
		generator.generate(glyphs.data(), (int)glyphs.size());

		auto storage = (msdf_atlas::BitmapAtlasStorage<float, 4>)generator.atlasStorage();
		msdfgen::BitmapConstRef<float, 4> bmp = storage;

		std::vector<uint8_t> pixels(atlas_w * atlas_h * 4);
		for (int i = 0; i < atlas_w * atlas_h * 4; i++)
			pixels[i] = (uint8_t)msdfgen::clamp(bmp.pixels[i] * 255.f + .5f, 0.f, 255.f);

		image = stbi_write_png_to_mem(pixels.data(), atlas_w * 4, atlas_w, atlas_h, 4,
									  &image_size);
	}
	else
	{
		msdf_atlas::ImmediateAtlasGenerator<float, 3, msdf_atlas::msdfGenerator,
											msdf_atlas::BitmapAtlasStorage<float, 3>>
			generator(atlas_w, atlas_h);
		generator.setAttributes(gen_attrs);
		generator.setThreadCount(4);
		generator.generate(glyphs.data(), (int)glyphs.size());

		auto storage = (msdf_atlas::BitmapAtlasStorage<float, 3>)generator.atlasStorage();
		msdfgen::BitmapConstRef<float, 3> bmp = storage;

		std::vector<uint8_t> pixels(atlas_w * atlas_h * 3);
		for (int i = 0; i < atlas_w * atlas_h * 3; i++)
			pixels[i] = (uint8_t)msdfgen::clamp(bmp.pixels[i] * 255.f + .5f, 0.f, 255.f);

		image = stbi_write_png_to_mem(pixels.data(), atlas_w * 3, atlas_w, atlas_h, 3,
									  &image_size);
	}

	auto metrics = font_geometry.getMetrics();

	std::vector<spel_font_kerning> kerning_pairs;
	if (cfg.kerning)
	{
		for (auto& ga : glyphs)
		{
			for (auto& gb : glyphs)
			{
				double kern = 0.0;
				if (font_geometry.getAdvance(kern, ga.getCodepoint(),
											 gb.getCodepoint()) &&
					kern != 0.0)
				{
					if (std::abs(kern) < 0.01)
					{
						continue;
					}

					spel_font_kerning kp;
					kp.codepoint_a = ga.getCodepoint();
					kp.codepoint_b = gb.getCodepoint();
					kp.advance = (float)kern;
					kerning_pairs.push_back(kp);
				}
			}
		}
	}

	std::vector<spel_font_glyph> glyph_records;
	glyph_records.reserve(glyphs.size());

	for (auto& g : glyphs)
	{
		if (g.isWhitespace())
		{

			spel_font_glyph rec = {};
			rec.codepoint = g.getCodepoint();
			double uvx;
			double uvy;
			double uvw;
			double uvh;
			g.getQuadAtlasBounds(uvx, uvy, uvw, uvh);
			rec.uv_x = uvx;
			rec.uv_y = uvy;
			rec.uv_w = uvw;
			rec.uv_h = uvh;
			rec.advance = (float)g.getAdvance();
			glyph_records.push_back(rec);
			continue;
		}

		spel_font_glyph rec = {};
		rec.codepoint = g.getCodepoint();

		double ax, ay, aw, ah;
		g.getQuadAtlasBounds(ax, ay, aw, ah);
		rec.uv_x = (float)(ax / atlas_w);
		rec.uv_y = (float)(ay / atlas_h);
		rec.uv_w = (float)((aw - ax) / atlas_w);
		rec.uv_h = (float)((ah - ay) / atlas_h);

		double px, py, pw, ph;
		g.getQuadPlaneBounds(px, py, pw, ph);
		rec.plane_x = (float)px;
		rec.plane_y = (float)py;
		rec.plane_w = (float)(pw - px);
		rec.plane_h = (float)(ph - py);

		rec.advance = (float)g.getAdvance();
		glyph_records.push_back(rec);
	}

	spel_font_header header = {};
	header.magic = SPFN_MAGIC;
	header.version = SPFN_VERSION;
	header.channels = (uint8_t)channels;
	header.atlas_width = (uint16_t)atlas_w;
	header.atlas_height = (uint16_t)atlas_h;
	header.glyph_count = (uint16_t)glyph_records.size();
	header.kerning_count = (uint16_t)kerning_pairs.size();
	header.em_size = cfg.em_size;
	header.ascender = (float)metrics.ascenderY;
	header.descender = (float)metrics.descenderY;
	header.line_height = (float)metrics.lineHeight;
	header.sdf_range = cfg.sdf_range;
	header.image_size = image_size;

	std::string spfn_path = cfg.output + ".spfn";
	FILE* f = fopen(spfn_path.c_str(), "wb");
	if (!f)
	{
		fprintf(stderr, "Failed to open output: %s\n", spfn_path.c_str());
		return 1;
	}

	fwrite(&header, sizeof(header), 1, f);
	fwrite(glyph_records.data(), sizeof(spel_font_glyph), glyph_records.size(), f);
	fwrite(kerning_pairs.data(), sizeof(spel_font_kerning), kerning_pairs.size(), f);
	fwrite(image, image_size, 1, f);
	fclose(f);

	free(image);

	printf("Wrote %s (%zu glyphs, %zu kerning pairs)\n", spfn_path.c_str(),
		   glyph_records.size(), kerning_pairs.size());

	msdfgen::destroyFont(font);
	msdfgen::deinitializeFreetype(ft);
	return 0;
}
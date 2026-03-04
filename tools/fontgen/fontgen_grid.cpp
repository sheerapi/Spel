#include "../fontgen.h"

#include "stb_image.h"
#include "../stb_image_write.h"

bool fontgen_grid(const GridConfig& cfg)
{

	int src_w, src_h, src_ch;
	uint8_t* src = stbi_load(cfg.input.c_str(), &src_w, &src_h, &src_ch, 4);
	if (!src)
	{
		fprintf(stderr, "Failed to load image: %s\n", cfg.input.c_str());
		return false;
	}

	int cell_w = cfg.cell_w > 0 ? cfg.cell_w : src_w / cfg.cols;
	int cell_h = cfg.cell_h > 0 ? cfg.cell_h : src_h / cfg.rows;

	int glyph_count = cfg.cols * cfg.rows;
	printf("Grid: %dx%d cells, cell size: %dx%d, %d glyphs\n", cfg.cols, cfg.rows, cell_w,
		   cell_h, glyph_count);

	std::vector<spel_font_glyph> glyphs;
	glyphs.reserve(glyph_count);

	float atlas_w = (float)src_w;
	float atlas_h = (float)src_h;

	for (int i = 0; i < glyph_count; i++)
	{
		int col = i % cfg.cols;
		int row = i / cfg.cols;

		spel_font_glyph g = {};
		g.codepoint = (uint32_t)(cfg.first_codepoint + i);
		g.uv_x = (col * cell_w) / atlas_w;
		g.uv_y = (row * cell_h) / atlas_h;
		g.uv_w = cell_w / atlas_w;
		g.uv_h = cell_h / atlas_h;

		g.plane_x = 0.0f;
		g.plane_y = 0.0f;
		g.plane_w = 1.0f;
		g.plane_h = 1.0f;
		g.advance = 1.0f;

		glyphs.push_back(g);
	}

	std::vector<uint8_t> png_bytes;
	stbi_write_png_to_func(
		[](void* ctx, void* data, int size)
		{
			auto* buf = (std::vector<uint8_t>*)ctx;
			buf->insert(buf->end(), (uint8_t*)data, (uint8_t*)data + size);
		},
		&png_bytes, src_w, src_h, 4, src, src_w * 4);
	stbi_image_free(src);

	spel_font_header header = {};
	header.magic = SPFN_MAGIC;
	header.version = SPFN_VERSION;
	header.channels = 4;
	header.atlas_width = (uint16_t)src_w;
	header.atlas_height = (uint16_t)src_h;
	header.glyph_count = (uint16_t)glyphs.size();
	header.kerning_count = 0;
	header.image_size = (uint32_t)png_bytes.size();
	header.em_size = (float)cell_h;
	header.ascender = 1.0f;
	header.descender = 0.0f;
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

	fwrite(png_bytes.data(), 1, png_bytes.size(), f);
	fclose(f);

	printf("Wrote %s (%d glyphs)\n", out_path.c_str(), (int)glyphs.size());
	return true;
}
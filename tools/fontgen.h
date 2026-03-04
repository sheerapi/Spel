#pragma once

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

// ─── .spfn format ─────────────────────────────────────────────────────────────

#define SPFN_MAGIC 0x4E465053 // "SPFN"
#define SPFN_VERSION 1

#define SPFN_TYPE_SDF 0
#define SPFN_TYPE_MSDF 1
#define SPFN_TYPE_BITMAP 2

#pragma pack(push, 1)

using spel_font_header = struct
{
	uint32_t magic;
	uint8_t version;
	uint8_t channels;  // 1=SDF, 3=MSDF, 4=MTSDF/bitmap
	uint8_t font_type; // SPFN_TYPE_*
	uint16_t atlas_width;
	uint16_t atlas_height;
	uint16_t glyph_count;
	uint32_t kerning_count;
	uint32_t image_size; // embedded PNG bytes, 0 = external .png
	float em_size;		 // px size atlas was generated at
	float ascender;		 // em units
	float descender;	 // em units (negative)
	float line_height;	 // em units
	float sdf_range;	 // px range (0 for bitmap)
	uint8_t padding[7];
}; // 48 bytes

using spel_font_glyph = struct
{
	uint32_t codepoint;
	float uv_x, uv_y;		// atlas UV top-left  (normalized)
	float uv_w, uv_h;		// atlas UV size       (normalized)
	float plane_x, plane_y; // quad offset from cursor (em units)
	float plane_w, plane_h; // quad size               (em units)
	float advance;			// horizontal advance       (em units)
	uint16_t padding[4];
};			// 44 bytes

using spel_font_kerning = struct
{
	uint32_t codepoint_a;
	uint32_t codepoint_b;
	float advance; // kerning advance (em units)
	uint8_t padding[4];
}; // 12 bytes

#pragma pack(pop)

// ─── configs ──────────────────────────────────────────────────────────────────

struct GridConfig
{
	std::string input;
	std::string output;
	int cols = 16;
	int rows = 8;
	int cell_w = 0; // 0 = infer from image
	int cell_h = 0;
	uint32_t first_codepoint = 0x20;
};

struct BMFontConfig
{
	std::string input;
	std::string output;
	bool skip_kerning = false;
};

// ─── entry points ─────────────────────────────────────────────────────────────

bool fontgen_grid(const GridConfig& cfg);
bool fontgen_bmfont(const BMFontConfig& cfg);
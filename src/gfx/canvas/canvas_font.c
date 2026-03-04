#include "core/log.h"
#include "core/memory.h"
#include "gfx/canvas/canvas_internal.h"
#include "gfx/gfx_canvas.h"
#include <string.h>

spel_api spel_font spel_font_create(spel_gfx_context gfx, const uint8_t* data,
									size_t dataSize)
{
	spel_font font = spel_memory_malloc(sizeof(*font), SPEL_MEM_TAG_GFX);
	font->ctx = gfx; // not the canvas ctx due to recursive inclusion

	if (dataSize < sizeof(spel_font_header))
	{
		spel_error(SPEL_ERR_INVALID_ARGUMENT, "i expected more data, got %d", dataSize);
		spel_memory_free(font);
		goto fail;
	}

	size_t offset = 0;

	memcpy(&font->header, data, sizeof(spel_font_header));
	offset += sizeof(spel_font_header);

	if (font->header.magic != SPFN_MAGIC)
	{
		spel_error(SPEL_ERR_INVALID_ARGUMENT,
				   "this is anything but an spël font file! magic: %d",
				   font->header.magic);
		goto fail;
	}

	if (font->header.version != SPFN_VERSION)
	{
		spel_error(SPEL_ERR_INVALID_ARGUMENT, "unsupported spël font file version %d",
				   font->header.version);
		goto fail;
	}

	font->glyphs = spel_memory_malloc(font->header.glyph_count * sizeof(spel_font_glyph),
									  SPEL_MEM_TAG_GFX);

	memcpy(font->glyphs, data + offset,
		   font->header.glyph_count * sizeof(spel_font_glyph));
	offset += font->header.glyph_count * sizeof(spel_font_glyph);

	if (font->header.kerning_count != 0)
	{
		font->kerning = spel_memory_malloc(
			font->header.kerning_count * sizeof(spel_font_kerning), SPEL_MEM_TAG_GFX);
		memcpy(font->kerning, data + offset,
			   font->header.kerning_count * sizeof(spel_font_kerning));
		offset += font->header.kerning_count * sizeof(spel_font_kerning);
	}

	spel_gfx_texture_load_desc desc = {
		.format = SPEL_GFX_TEXTURE_FMT_UNKNOWN,
		.usage = SPEL_GFX_TEXTURE_USAGE_SAMPLED,
		.mip_count = 1,
		.srgb = false,
	};

	font->atlas =
		spel_gfx_texture_load_data(gfx, (char*)data + offset, font->header.image_size, &desc);

	if (font->atlas == spel_gfx_texture_checker_get(gfx))
	{
		spel_error(SPEL_ERR_INVALID_RESOURCE, "failed to create font atlas");
		goto fail;
	}

	return font;

fail:
	spel_memory_free(font->glyphs);
	spel_memory_free(font->kerning);
	spel_memory_free(font);
	return NULL;
}

spel_api void spel_font_destroy(spel_font font)
{
	spel_gfx_texture_destroy(font->atlas);
	if (font->glyphs)
	{
		spel_memory_free(font->glyphs);
	}
	if (font->kerning)
	{
		spel_memory_free(font->kerning);
	}
	spel_memory_free(font);
}

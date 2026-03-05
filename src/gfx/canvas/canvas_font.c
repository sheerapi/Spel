#include "core/log.h"
#include "core/memory.h"
#include "gfx/canvas/canvas_internal.h"
#include "gfx/gfx_canvas.h"
#include "gfx/gfx_internal.h"
#include "gfx_internal_shaders.h"
#include "utils/internal/stb_image.h"
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

	// Load atlas. For bitmap fonts we patch the pixel data so fully transparent pixels
	// don't look like solid white quads (shader uses RGB==RGB check).
	spel_gfx_texture_load_desc desc = {
		.format = SPEL_GFX_TEXTURE_FMT_UNKNOWN,
		.usage = SPEL_GFX_TEXTURE_USAGE_SAMPLED,
		.mip_count = 1,
		.srgb = false,
	};

	if (font->header.font_type == SPFN_TYPE_BITMAP && font->header.channels == 4)
	{
		int w, h, comp;
		stbi_uc* pixels = stbi_load_from_memory((uint8_t*)data + offset,
												(int)font->header.image_size, &w, &h, &comp, 4);
		if (!pixels)
		{
			font->atlas = spel_gfx_texture_checker_get(gfx);
		}
		else
		{
			for (int i = 0; i < w * h; i++)
			{
				uint8_t* p = pixels + (i * 4);
				if (p[3] == 0)
				{
					// break RGB equality so shader falls back to alpha (which is 0 here)
					p[2] = 1;
				}
			}

			spel_gfx_texture_desc tex = {
				.type = SPEL_GFX_TEXTURE_2D,
				.format = SPEL_GFX_TEXTURE_FMT_RGBA8_UNORM,
				.usage = desc.usage,
				.width = (uint32_t)w,
				.height = (uint32_t)h,
				.depth = 1,
				.mip_count = desc.mip_count,
				.data = pixels,
				.data_size = (size_t)w * h * 4,
			};

			font->atlas = spel_gfx_texture_create(gfx, &tex);
			stbi_image_free(pixels);
		}
	}
	else
	{
		font->atlas = spel_gfx_texture_load_data(gfx, (char*)data + offset,
												 font->header.image_size, &desc);
	}

	if (font->atlas == spel_gfx_texture_checker_get(gfx))
	{
		spel_error(SPEL_ERR_INVALID_RESOURCE, "failed to create font atlas");
		goto fail;
	}

	memset(font->ascii_index, -1, sizeof(font->ascii_index));

	int ext_count = 0;
	for (int i = 0; i < font->header.glyph_count; i++)
	{
		uint32_t cp = font->glyphs[i].codepoint;
		if (cp < 128)
			font->ascii_index[cp] = i;
		else
			ext_count++;
	}

	font->ext_count = ext_count;
	font->ext_codepoints =
		ext_count ? spel_memory_malloc(ext_count * sizeof(uint32_t), SPEL_MEM_TAG_GFX)
				  : NULL;
	font->ext_indices =
		ext_count ? spel_memory_malloc(ext_count * sizeof(int), SPEL_MEM_TAG_GFX) : NULL;

	int j = 0;
	for (int i = 0; i < font->header.glyph_count; i++)
	{
		uint32_t cp = font->glyphs[i].codepoint;
		if (cp >= 128)
		{
			font->ext_codepoints[j] = cp;
			font->ext_indices[j] = i;
			j++;
		}
	}

	for (int a = 1; a < ext_count; a++)
	{
		uint32_t kcp = font->ext_codepoints[a];
		int ki = font->ext_indices[a];
		int b = a - 1;
		while (b >= 0 && font->ext_codepoints[b] > kcp)
		{
			font->ext_codepoints[b + 1] = font->ext_codepoints[b];
			font->ext_indices[b + 1] = font->ext_indices[b];
			b--;
		}
		font->ext_codepoints[b + 1] = kcp;
		font->ext_indices[b + 1] = ki;
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
	if (font->ext_count)
	{
		spel_memory_free(font->ext_codepoints);
		spel_memory_free(font->ext_indices);
	}

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

void spel_canvas_emit_glyph(spel_font font, const spel_font_glyph* g, float cx, float cy,
							float scale, spel_color color)
{
	// Bitmap fonts are authored with Y-down uv/planes; MSDF fonts are Y-up.
	bool y_up = font->header.font_type != SPFN_TYPE_BITMAP;

	float x0 = cx + (g->plane_x * scale);
	float x1 = x0 + (g->plane_w * scale);

	float y0, y1;
	if (y_up)
	{
		y0 = cy - (g->plane_y + g->plane_h) * scale;
		y1 = cy - (g->plane_y * scale);
	}
	else
	{
		y0 = cy + (g->plane_y * scale);
		y1 = y0 + (g->plane_h * scale);
	}

	float u0 = g->uv_x;
	float v0 = g->uv_y;
	float u1 = g->uv_x + g->uv_w;
	float v1 = g->uv_y + g->uv_h;

	// apply canvas transform
	spel_mat3 t = spel.gfx->canvas_ctx->transforms[spel.gfx->canvas_ctx->transform_top];

	spel_vec2 p00 = spel_mat3_transform_point(t, (spel_vec2){x0, y0});
	spel_vec2 p10 = spel_mat3_transform_point(t, (spel_vec2){x1, y0});
	spel_vec2 p11 = spel_mat3_transform_point(t, (spel_vec2){x1, y1});
	spel_vec2 p01 = spel_mat3_transform_point(t, (spel_vec2){x0, y1});

	uint32_t base = spel.gfx->canvas_ctx->vert_count;

	spel_canvas_vertex* verts = spel.gfx->canvas_ctx->verts + base;
	if (y_up)
	{
		verts[0] = (spel_canvas_vertex){p00, {u0, v1}, color};
		verts[1] = (spel_canvas_vertex){p10, {u1, v1}, color};
		verts[2] = (spel_canvas_vertex){p11, {u1, v0}, color};
		verts[3] = (spel_canvas_vertex){p01, {u0, v0}, color};
	}
	else
	{
		verts[0] = (spel_canvas_vertex){p00, {u0, v0}, color};
		verts[1] = (spel_canvas_vertex){p10, {u1, v0}, color};
		verts[2] = (spel_canvas_vertex){p11, {u1, v1}, color};
		verts[3] = (spel_canvas_vertex){p01, {u0, v1}, color};
	}

	uint32_t* idx = spel.gfx->canvas_ctx->indices + spel.gfx->canvas_ctx->index_count;
	idx[0] = base + 0;
	idx[1] = base + 1;
	idx[2] = base + 2;
	idx[3] = base + 0;
	idx[4] = base + 2;
	idx[5] = base + 3;

	spel.gfx->canvas_ctx->vert_count += 4;
	spel.gfx->canvas_ctx->index_count += 6;
}

void spel_canvas_draw_text_internal(const char* text, spel_vec2 position, float maxWidth)
{
	if (!text || !*text)
	{
		return;
	}

	spel_font font = spel.gfx->canvas_ctx->font;
	if (!font)
	{
		spel_error(SPEL_ERR_INVALID_ARGUMENT, "no font set");
		return;
	}

	// Glyph metrics are stored in "em" units (1 = design height), so scale is just pixels per em.
	float scale = spel.gfx->canvas_ctx->font_size;
	float line_h = font->header.line_height * scale;
	spel_color col = spel.gfx->canvas_ctx->color;

	// Compute per-draw smoothing based on rendered pixel height:
	//   - sdf_range is the pixel distance encoded in the atlas at em_size.
	//   - scale/em_size converts that range into current screen pixels.
	//   - smoothing is half a pixel in the distance domain.
	float px_range = font->header.sdf_range > 0.0f ? font->header.sdf_range : 1.0f;
	float range_screen = px_range * (spel.gfx->canvas_ctx->font_size / font->header.em_size);
	float smoothing = 0.5f / range_screen;
	smoothing = spel_math_clamp(smoothing, 0.01f, 0.6f);

	spel.gfx->canvas_ctx->font_data.sdf_smoothing = smoothing;
	spel.gfx->canvas_ctx->font_data.sdf_threshold = 0.5f;

	spel.gfx->canvas_ctx->font_data.mode =
		(font->header.font_type == SPFN_TYPE_BITMAP)
			? SPFN_TYPE_BITMAP
			: (font->header.channels == 3 ? SPFN_TYPE_MSDF : SPFN_TYPE_MTSDF);

	if (spel_canvas_check_batch(font->atlas, SPEL_CANVAS_TEXT, spel.gfx->canvas_ctx))
	{
		if (spel.gfx->canvas_ctx->default_shader)
		{
			if (spel.gfx->shaders[4] == NULL)
			{
				spel_gfx_shader_desc vertex_desc;
				vertex_desc.shader_source = SPEL_GFX_SHADER_STATIC;
				vertex_desc.debug_name = "spel_internal_text_frag";
				vertex_desc.source = spel_internal_text_frag_spv;
				vertex_desc.source_size = spel_internal_text_frag_spv_len;

				spel.gfx->shaders[4] = spel_gfx_shader_create(spel.gfx, &vertex_desc);
				spel.gfx->shaders[4]->internal = true;
			}

			spel_gfx_shader shader = spel.gfx->canvas_ctx->pipeline_desc.fragment_shader;

			spel.gfx->canvas_ctx->pipeline_desc.fragment_shader = spel.gfx->shaders[4];
			spel.gfx->canvas_ctx->pipeline =
				spel_gfx_pipeline_create(spel.gfx, &spel.gfx->canvas_ctx->pipeline_desc);

			spel.gfx->canvas_ctx->pipeline_desc.fragment_shader = shader;

			if (spel.gfx->canvas_ctx->font_ubuffer.buffer == NULL)
			{
				spel.gfx->canvas_ctx->font_ubuffer = spel_gfx_uniform_buffer_create(
					spel.gfx->canvas_ctx->pipeline, "DrawData");
			}
		}

		spel_gfx_sampler_filter min = spel.gfx->canvas_ctx->sampler_desc.min;
		spel.gfx->canvas_ctx->sampler_desc.min =
			spel.gfx->canvas_ctx->font_data.mode == SPFN_TYPE_BITMAP
				? SPEL_GFX_SAMPLER_FILTER_NEAREST
				: SPEL_GFX_SAMPLER_FILTER_LINEAR;

		spel.gfx->canvas_ctx->sampler_desc.mag =
			spel.gfx->canvas_ctx->font_data.mode == SPFN_TYPE_BITMAP
				? SPEL_GFX_SAMPLER_FILTER_NEAREST
				: SPEL_GFX_SAMPLER_FILTER_LINEAR;

		spel.gfx->canvas_ctx->sampler =
			spel_gfx_sampler_get(spel.gfx, &spel.gfx->canvas_ctx->sampler_desc);

		spel.gfx->canvas_ctx->sampler_desc.min = min;
		spel.gfx->canvas_ctx->sampler_desc.mag = min;

		spel.gfx->canvas_ctx->pipeline_dirty = true;
		spel.gfx->canvas_ctx->sampler_dirty = true;
	}

	float align_offset_x = 0.0F;
	bool needs_align = spel.gfx->canvas_ctx->text_align != SPEL_CANVAS_ALIGN_LEFT;

	if (needs_align)
	{
		// measure first line
		float line_w = 0.0f;
		const char* p = text;
		uint32_t prev = 0;
		while (*p && *p != '\n')
		{
			uint32_t cp = spel_canvas_font_utf8_next(&p);
			const spel_font_glyph* g = spel_canvas_font_find_glyph(font, cp);
			line_w += (g->advance + spel_canvas_font_kerning(font, prev, cp)) * scale;
			prev = cp;
		}
		uint8_t halign = spel.gfx->canvas_ctx->text_align;
		if (halign == SPEL_CANVAS_ALIGN_CENTER)
			align_offset_x = -line_w * 0.5f;
		else if (halign == SPEL_CANVAS_ALIGN_RIGHT)
			align_offset_x = -line_w;
	}

	// vertical alignment offset
	float align_offset_y = 0.0f;
	if (spel.gfx->canvas_ctx->text_align >= SPEL_CANVAS_ALIGN_TOP)
	{
		spel_vec2 total = spel_canvas_text_measure(text);
		uint8_t valign = spel.gfx->canvas_ctx->text_align;
		if (valign == SPEL_CANVAS_ALIGN_MIDDLE)
			align_offset_y = -total.y * 0.5f;
		else if (valign == SPEL_CANVAS_ALIGN_BOTTOM)
			align_offset_y = -total.y;
	}

	// ── layout + emit ─────────────────────────────────────────────────────────
	float cx = position.x + align_offset_x;
	float cy = position.y + align_offset_y;
	float line_x0 = cx; // remember line start for newlines + wrapping

	const char* p = text;
	uint32_t prev_cp = 0;

	while (*p)
	{
		// word wrap: find next word, check if it fits
		if (maxWidth > 0.0f && *p != '\n')
		{
			const char* word_end = p;
			float word_w = 0.0f;
			uint32_t wc_prev = prev_cp;
			while (*word_end && *word_end != ' ' && *word_end != '\n')
			{
				uint32_t wcp = spel_canvas_font_utf8_next(&word_end);
				const spel_font_glyph* wg = spel_canvas_font_find_glyph(font, wcp);
				word_w +=
					(wg->advance + spel_canvas_font_kerning(font, wc_prev, wcp)) * scale;
				wc_prev = wcp;
			}
			float used = cx - line_x0;
			if (used > 0.0f && used + word_w > maxWidth)
			{
				cx = line_x0;
				cy += line_h;
				prev_cp = 0;
				// skip leading space that caused wrap
				if (*p == ' ')
				{
					spel_canvas_font_utf8_next(&p);
					continue;
				}
			}
		}

		uint32_t cp = spel_canvas_font_utf8_next(&p);
		if (cp == 0)
			break;

		if (cp == '\n')
		{
			cx = line_x0;
			cy += line_h;
			// re-measure alignment for new line if needed
			if (needs_align)
			{
				float line_w = 0.0f;
				const char* lp = p;
				uint32_t lp_prev = 0;
				while (*lp && *lp != '\n')
				{
					uint32_t lcp = spel_canvas_font_utf8_next(&lp);
					const spel_font_glyph* lg = spel_canvas_font_find_glyph(font, lcp);
					line_w +=
						(lg->advance + spel_canvas_font_kerning(font, lp_prev, lcp)) *
						scale;
					lp_prev = lcp;
				}
				uint8_t halign = spel.gfx->canvas_ctx->text_align < SPEL_CANVAS_ALIGN_TOP;
				if (halign == SPEL_CANVAS_ALIGN_CENTER)
					cx = position.x - line_w * 0.5f;
				else if (halign == SPEL_CANVAS_ALIGN_RIGHT)
					cx = position.x - line_w;
			}
			prev_cp = 0;
			continue;
		}

		const spel_font_glyph* g = spel_canvas_font_find_glyph(font, cp);

		// apply kerning
		cx += spel_canvas_font_kerning(font, prev_cp, cp) * scale;

		// missing glyph -> advance by a sensible default (half an em) and skip
		if (g == NULL)
		{
			cx += 0.5f * scale;
			prev_cp = cp;
			continue;
		}

		// skip whitespace quads (no visible glyph) but still advance
		if (g->uv_w > 0.0f && g->uv_h > 0.0f)
		{
			spel_canvas_ensure_capacity(4, 6);
			spel_canvas_emit_glyph(font, g, cx, cy, scale, col);
		}

		cx += g->advance * scale;
		prev_cp = cp;
	}
}

void spel_canvas_draw_text(const char* text, spel_vec2 position)
{
	spel_canvas_draw_text_internal(text, position, 0.0F);
}

void spel_canvas_draw_text_wrapped(const char* text, spel_vec2 position, float maxWidth)
{
	spel_canvas_draw_text_internal(text, position, maxWidth);
}

spel_vec2 spel_canvas_text_measure(const char* text)
{
	if (!text || !spel.gfx->canvas_ctx->font)
		return (spel_vec2){0, 0};

	spel_font font = spel.gfx->canvas_ctx->font;
	float scale = spel.gfx->canvas_ctx->font_size;

	float x = 0.0f;
	float max_x = 0.0f;
	float y = font->header.line_height * scale;
	int lines = 1;

	const char* p = text;
	uint32_t prev_cp = 0;

	while (*p)
	{
		uint32_t cp = spel_canvas_font_utf8_next(&p);
		if (cp == 0)
			break;

		if (cp == '\n')
		{
			max_x = spel_math_maxf(max_x, x);
			x = 0.0f;
			lines++;
			prev_cp = 0;
			continue;
		}

		const spel_font_glyph* g = spel_canvas_font_find_glyph(font, cp);
		if (g)
			x += (g->advance + spel_canvas_font_kerning(font, prev_cp, cp)) * scale;
		else
			x += 0.5f * scale;
		prev_cp = cp;
	}

	max_x = spel_math_maxf(max_x, x);
	return (spel_vec2){max_x, lines * font->header.line_height * scale};
}

float spel_canvas_font_kerning(const spel_font font, uint32_t cp_a, uint32_t cp_b)
{
	if (!font->kerning || font->header.kerning_count == 0)
		return 0.0f;

	int lo = 0, hi = (int)font->header.kerning_count - 1;
	while (lo <= hi)
	{
		int mid = (lo + hi) / 2;
		spel_font_kerning* k = &font->kerning[mid];
		if (k->codepoint_a < cp_a)
			lo = mid + 1;
		else if (k->codepoint_a > cp_a)
			hi = mid - 1;
		else if (k->codepoint_b < cp_b)
			lo = mid + 1;
		else if (k->codepoint_b > cp_b)
			hi = mid - 1;
		else
			return k->advance;
	}
	return 0.0f;
}

const spel_font_glyph* spel_canvas_font_find_glyph(const spel_font FONT,
												   uint32_t codepoint)
{
	if (codepoint < 128 && FONT->ascii_index[codepoint] >= 0)
		return &FONT->glyphs[FONT->ascii_index[codepoint]];

	// binary search
	int lo = 0;
	int hi = (int)FONT->header.glyph_count - 1;
	while (lo <= hi)
	{
		int mid = (lo + hi) / 2;
		if (FONT->glyphs[mid].codepoint < codepoint)
			lo = mid + 1;
		else if (FONT->glyphs[mid].codepoint > codepoint)
			hi = mid - 1;
		else
			return &FONT->glyphs[mid];
	}

	return NULL;
}

uint32_t spel_canvas_font_utf8_next(const char** str)
{
	const uint8_t* s = (const uint8_t*)*str;
	if (!*s)
		return 0;

	uint32_t cp;
	int bytes;

	if (*s < 0x80)
	{
		cp = *s;
		bytes = 1;
	}
	else if (*s < 0xC0)
	{
		cp = 0xFFFD;
		bytes = 1;
	}
	else if (*s < 0xE0)
	{
		cp = *s & 0x1F;
		bytes = 2;
	}
	else if (*s < 0xF0)
	{
		cp = *s & 0x0F;
		bytes = 3;
	}
	else
	{
		cp = *s & 0x07;
		bytes = 4;
	}

	for (int i = 1; i < bytes; i++)
	{
		if ((s[i] & 0xC0) != 0x80)
		{
			*str += 1;
			return 0xFFFD;
		}
		cp = (cp << 6) | (s[i] & 0x3F);
	}

	*str += bytes;
	return cp;
}

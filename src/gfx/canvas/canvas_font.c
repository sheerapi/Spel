#include "core/log.h"
#include "core/memory.h"
#include "gfx/canvas/canvas_internal.h"
#include "gfx/gfx_canvas.h"
#include "gfx/gfx_internal.h"
#include "gfx_internal_shaders.h"
#include "utils/internal/stb_image.h"
#include <limits.h>
#include <stdarg.h>
#include <stddef.h>
#include <string.h>

static bool spel_font_can_read(size_t offset, size_t count, size_t total)
{
	return offset <= total && count <= (total - offset);
}

static int spel_font_mode(const spel_font font)
{
	switch (font->header.font_type)
	{
	case SPFN_TYPE_SDF:
		return SPFN_TYPE_SDF;
	case SPFN_TYPE_MSDF:
		return SPFN_TYPE_MSDF;
	case SPFN_TYPE_BITMAP:
		return SPFN_TYPE_BITMAP;
	case SPFN_TYPE_MTSDF:
		return SPFN_TYPE_MTSDF;
	default:
		if (font->header.channels == 1)
			return SPFN_TYPE_SDF;
		if (font->header.channels == 3)
			return SPFN_TYPE_MSDF;
		if (font->header.channels == 4)
			return SPFN_TYPE_MTSDF;
		return SPFN_TYPE_BITMAP;
	}
}

spel_api spel_font spel_font_create(spel_gfx_context gfx, const uint8_t* data,
									size_t dataSize)
{
	spel_font font = spel_memory_malloc(sizeof(*font), SPEL_MEM_TAG_GFX);
	if (font == NULL)
	{
		spel_error(SPEL_ERR_OOM, "failed to allocate font");
		return NULL;
	}
	memset(font, 0, sizeof(*font));
	font->ctx = gfx;

	if (data == NULL || dataSize < sizeof(spel_font_header))
	{
		spel_error(SPEL_ERR_INVALID_ARGUMENT, "i expected more data, got %d", dataSize);
		goto fail;
	}

	size_t offset = 0;

	if (!spel_font_can_read(offset, sizeof(spel_font_header), dataSize))
	{
		spel_error(SPEL_ERR_INVALID_ARGUMENT, "font header is truncated");
		goto fail;
	}

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

	size_t glyph_bytes =
		(size_t)font->header.glyph_count * sizeof(spel_font_glyph);
	if (!spel_font_can_read(offset, glyph_bytes, dataSize))
	{
		spel_error(SPEL_ERR_INVALID_ARGUMENT, "font glyph table is truncated");
		goto fail;
	}

	font->glyphs = spel_memory_malloc(glyph_bytes, SPEL_MEM_TAG_GFX);
	if (font->glyphs == NULL)
	{
		spel_error(SPEL_ERR_OOM, "failed to allocate glyph table");
		goto fail;
	}

	memcpy(font->glyphs, data + offset, glyph_bytes);
	offset += glyph_bytes;

	if (font->header.kerning_count != 0)
	{
		size_t kerning_bytes =
			(size_t)font->header.kerning_count * sizeof(spel_font_kerning);
		if (!spel_font_can_read(offset, kerning_bytes, dataSize))
		{
			spel_error(SPEL_ERR_INVALID_ARGUMENT, "font kerning table is truncated");
			goto fail;
		}

		font->kerning = spel_memory_malloc(kerning_bytes, SPEL_MEM_TAG_GFX);
		if (font->kerning == NULL)
		{
			spel_error(SPEL_ERR_OOM, "failed to allocate kerning table");
			goto fail;
		}

		memcpy(font->kerning, data + offset, kerning_bytes);
		offset += kerning_bytes;

		for (size_t a = 1; a < font->header.kerning_count; a++)
		{
			spel_font_kerning key = font->kerning[a];
			size_t b = a;
			while (b > 0)
			{
				spel_font_kerning prev = font->kerning[b - 1];
				bool out_of_order = prev.codepoint_a > key.codepoint_a ||
									(prev.codepoint_a == key.codepoint_a &&
									 prev.codepoint_b > key.codepoint_b);
				if (!out_of_order)
					break;

				font->kerning[b] = prev;
				b--;
			}
			font->kerning[b] = key;
		}
	}

	spel_gfx_texture_load_desc desc = {
		.format = SPEL_GFX_TEXTURE_FMT_UNKNOWN,
		.usage = SPEL_GFX_TEXTURE_USAGE_SAMPLED,
		.mip_count = 1,
		.srgb = false,
	};

	if (!spel_font_can_read(offset, font->header.image_size, dataSize))
	{
		spel_error(SPEL_ERR_INVALID_ARGUMENT, "font atlas image is truncated");
		goto fail;
	}

	if (font->header.font_type == SPFN_TYPE_BITMAP && font->header.channels == 4)
	{
		if (font->header.image_size > INT_MAX)
		{
			spel_error(SPEL_ERR_INVALID_ARGUMENT, "font atlas is too large");
			goto fail;
		}

		int w;
		int h;
		int comp;
		stbi_uc* pixels = stbi_load_from_memory(
			(uint8_t*)data + offset, (int)font->header.image_size, &w, &h, &comp, 4);
		if (!pixels)
		{
			font->atlas = spel_gfx_texture_checker_get(gfx);
		}
		else
		{
			for (int i = 0; i < w * h; i++)
			{
				uint8_t* p = pixels + ((ptrdiff_t)(i * 4));
				if (p[3] == 0)
				{

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
		{
			font->ascii_index[cp] = i;
		}
		else
		{
			ext_count++;
		}
	}

	font->ext_count = ext_count;
	font->ext_codepoints =
		ext_count ? spel_memory_malloc(ext_count * sizeof(uint32_t), SPEL_MEM_TAG_GFX)
				  : NULL;
	font->ext_indices =
		ext_count ? spel_memory_malloc(ext_count * sizeof(int), SPEL_MEM_TAG_GFX) : NULL;
	if (ext_count && (!font->ext_codepoints || !font->ext_indices))
	{
		spel_error(SPEL_ERR_OOM, "failed to allocate glyph extension index");
		goto fail;
	}

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
	if (font->atlas && font->atlas != spel_gfx_texture_checker_get(gfx))
	{
		spel_gfx_texture_destroy(font->atlas);
	}
	spel_memory_free(font->ext_codepoints);
	spel_memory_free(font->ext_indices);
	spel_memory_free(font->glyphs);
	spel_memory_free(font->kerning);
	spel_memory_free(font);
	return NULL;
}

spel_api void spel_font_destroy(spel_font font)
{
	if (font->internal)
	{
		spel_error(SPEL_ERR_INVALID_RESOURCE, "you cant destroy an internal font!");
	}

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

	bool y_up = font->header.font_type != SPFN_TYPE_BITMAP;

	float x0 = cx + (g->plane_x * scale);
	float x1 = x0 + (g->plane_w * scale);

	float y0;
	float y1;
	if (y_up)
	{
		y0 = cy - ((g->plane_y + g->plane_h) * scale);
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

	float scale = spel.gfx->canvas_ctx->font_size;
	float line_h = font->header.line_height * scale;
	spel_color col = spel.gfx->canvas_ctx->color;
	spel_canvas_context* ctx = spel.gfx->canvas_ctx;

	ctx->font_data.sdf_threshold = 0.5f;
	ctx->font_data.mode = spel_font_mode(font);

	if (spel.gfx->shaders[4] == NULL)
	{
		spel_gfx_shader_desc fragment_desc;
		fragment_desc.shader_source = SPEL_GFX_SHADER_STATIC;
		fragment_desc.debug_name = "spel_internal_text_frag";
		fragment_desc.source = spel_internal_text_frag_spv;
		fragment_desc.source_size = spel_internal_text_frag_spv_len;

		spel.gfx->shaders[4] = spel_gfx_shader_create(spel.gfx, &fragment_desc);
		if (spel.gfx->shaders[4] != NULL)
		{
			spel.gfx->shaders[4]->internal = true;
		}
	}

	if (spel.gfx->shaders[4] == NULL)
	{
		spel_error(SPEL_ERR_SHADER_FAILED, "failed to create internal text shader");
		return;
	}

	spel_gfx_shader previous_fragment = ctx->pipeline_desc.fragment_shader;
	if (ctx->pipeline_desc.fragment_shader != spel.gfx->shaders[4])
	{
		ctx->pipeline_desc.fragment_shader = spel.gfx->shaders[4];
		ctx->pipeline_dirty = true;
	}

	if (spel_canvas_check_batch(font->atlas, SPEL_CANVAS_TEXT, ctx))
	{
		spel_gfx_sampler_filter min = ctx->sampler_desc.min;
		spel_gfx_sampler_filter mag = ctx->sampler_desc.mag;
		ctx->sampler_desc.min =
			ctx->font_data.mode == SPFN_TYPE_BITMAP
				? SPEL_GFX_SAMPLER_FILTER_NEAREST
				: SPEL_GFX_SAMPLER_FILTER_LINEAR;

		ctx->sampler_desc.mag =
			ctx->font_data.mode == SPFN_TYPE_BITMAP
				? SPEL_GFX_SAMPLER_FILTER_NEAREST
				: SPEL_GFX_SAMPLER_FILTER_LINEAR;

		ctx->sampler = spel_gfx_sampler_get(spel.gfx, &ctx->sampler_desc);

		ctx->sampler_desc.min = min;
		ctx->sampler_desc.mag = mag;
	}

	if (ctx->font_ubuffer.buffer == NULL)
	{
		ctx->font_ubuffer = spel_gfx_uniform_buffer_create(ctx->pipeline, "DrawData");
		if (ctx->font_ubuffer.buffer == NULL)
		{
			spel_error(SPEL_ERR_INVALID_RESOURCE,
					   "text draw skipped: failed to create DrawData buffer");
			ctx->pipeline_desc.fragment_shader = previous_fragment;
			return;
		}
	}

	float align_offset_x = 0.0F;
	bool needs_align = ctx->text_align != SPEL_CANVAS_ALIGN_LEFT;

	if (needs_align)
	{

		float line_w = 0.0f;
		const char* p = text;
		uint32_t prev = 0;
		while (*p && *p != '\n')
		{
			uint32_t cp = spel_canvas_font_utf8_next(&p);
			const spel_font_glyph* g = spel_canvas_font_find_glyph(font, cp);
			float adv = g ? g->advance : (font->header.line_height * 0.5f);
			line_w += (adv + spel_canvas_font_kerning(font, prev, cp)) * scale;
			prev = cp;
		}
		uint8_t halign = ctx->text_align;
		if (halign == SPEL_CANVAS_ALIGN_CENTER)
			align_offset_x = -line_w * 0.5f;
		else if (halign == SPEL_CANVAS_ALIGN_RIGHT)
			align_offset_x = -line_w;
	}

	float align_offset_y = 0.0f;
	if (ctx->text_align >= SPEL_CANVAS_ALIGN_TOP)
	{
		spel_vec2 total = spel_canvas_text_measure(text);
		uint8_t valign = ctx->text_align;
		if (valign == SPEL_CANVAS_ALIGN_MIDDLE)
			align_offset_y = -total.y * 0.5f;
		else if (valign == SPEL_CANVAS_ALIGN_BOTTOM)
			align_offset_y = -total.y;
	}

	// Canvas coordinates are top-left origin (y goes down). For MSDF/SDF fonts the glyph
	// plane is baseline-relative (y goes up), so convert the provided top-left to a
	// baseline position. Some bitmap font sources (e.g. BMFont) provide top-relative
	// plane_y, so keep those in top-origin space.
	bool bitmap_top_origin = false;
	if (font->header.font_type == SPFN_TYPE_BITMAP)
	{
		float min_plane_y = 0.0f;
		bool have_plane_y = false;
		for (int i = 0; i < font->header.glyph_count; i++)
		{
			const spel_font_glyph* g = &font->glyphs[i];
			if (g->plane_h <= 0.0f)
				continue;

			if (!have_plane_y)
			{
				min_plane_y = g->plane_y;
				have_plane_y = true;
			}
			else if (g->plane_y < min_plane_y)
			{
				min_plane_y = g->plane_y;
			}

			if (min_plane_y < 0.0f)
				break;
		}

		bitmap_top_origin = have_plane_y && min_plane_y >= 0.0f;
	}

	float baseline_offset_y = bitmap_top_origin ? 0.0f : (font->header.ascender * scale);

	float cx = position.x + align_offset_x;
	float cy = position.y + align_offset_y + baseline_offset_y;
	float line_x0 = cx;

	const char* p = text;
	uint32_t prev_cp = 0;

	while (*p)
	{
		if (maxWidth > 0.0f && *p != '\n')
		{
			const char* word_end = p;
			float word_w = 0.0f;
			uint32_t wc_prev = prev_cp;
			while (*word_end && *word_end != ' ' && *word_end != '\n')
			{
				uint32_t wcp = spel_canvas_font_utf8_next(&word_end);
				const spel_font_glyph* wg = spel_canvas_font_find_glyph(font, wcp);
				float adv = wg ? wg->advance : (font->header.line_height * 0.5f);
				word_w += (adv + spel_canvas_font_kerning(font, wc_prev, wcp)) * scale;
				wc_prev = wcp;
			}
			float used = cx - line_x0;
			if (used > 0.0f && used + word_w > maxWidth)
			{
				cx = line_x0;
				cy += line_h;
				prev_cp = 0;

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

			if (needs_align)
			{
				float line_w = 0.0f;
				const char* lp = p;
				uint32_t lp_prev = 0;
				while (*lp && *lp != '\n')
				{
					uint32_t lcp = spel_canvas_font_utf8_next(&lp);
					const spel_font_glyph* lg = spel_canvas_font_find_glyph(font, lcp);
					float adv = lg ? lg->advance : (font->header.line_height * 0.5f);
					line_w +=
						(adv + spel_canvas_font_kerning(font, lp_prev, lcp)) * scale;
					lp_prev = lcp;
				}
				uint8_t halign = ctx->text_align;
				if (halign == SPEL_CANVAS_ALIGN_CENTER)
					cx = position.x - line_w * 0.5f;
				else if (halign == SPEL_CANVAS_ALIGN_RIGHT)
					cx = position.x - line_w;
			}
			prev_cp = 0;
			continue;
		}

		const spel_font_glyph* g = spel_canvas_font_find_glyph(font, cp);

		cx += spel_canvas_font_kerning(font, prev_cp, cp) * scale;

		if (g == NULL)
		{
			cx += 0.5f * scale;
			prev_cp = cp;
			continue;
		}

		if (g->uv_w > 0.0f && g->uv_h > 0.0f)
		{
			spel_canvas_ensure_capacity(4, 6);
			spel_canvas_emit_glyph(font, g, cx, cy, scale, col);
		}

		cx += g->advance * scale;
		prev_cp = cp;
	}

	ctx->pipeline_desc.fragment_shader = previous_fragment;
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
		float adv = g ? g->advance : (font->header.line_height * 0.5f);
		x += (adv + spel_canvas_font_kerning(font, prev_cp, cp)) * scale;
		prev_cp = cp;
	}

	max_x = spel_math_maxf(max_x, x);
	return (spel_vec2){max_x, lines * font->header.line_height * scale};
}

float spel_canvas_font_kerning(const spel_font font, uint32_t cpA, uint32_t cpB)
{
	if (!font->kerning || font->header.kerning_count == 0)
		return 0.0f;

	int lo = 0, hi = (int)font->header.kerning_count - 1;
	while (lo <= hi)
	{
		int mid = (lo + hi) / 2;
		spel_font_kerning* k = &font->kerning[mid];
		if (k->codepoint_a < cpA)
			lo = mid + 1;
		else if (k->codepoint_a > cpA)
			hi = mid - 1;
		else if (k->codepoint_b < cpB)
			lo = mid + 1;
		else if (k->codepoint_b > cpB)
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

	if (codepoint >= 128 && FONT->ext_count > 0 && FONT->ext_codepoints &&
		FONT->ext_indices)
	{
		int lo = 0;
		int hi = FONT->ext_count - 1;
		while (lo <= hi)
		{
			int mid = (lo + hi) / 2;
			uint32_t mid_cp = FONT->ext_codepoints[mid];
			if (mid_cp < codepoint)
				lo = mid + 1;
			else if (mid_cp > codepoint)
				hi = mid - 1;
			else
				return &FONT->glyphs[FONT->ext_indices[mid]];
		}
	}

	for (int i = 0; i < FONT->header.glyph_count; i++)
	{
		if (FONT->glyphs[i].codepoint == codepoint)
		{
			return &FONT->glyphs[i];
		}
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

void spel_canvas_font_set(spel_font font)
{
	if (font == NULL)
	{
		spel.gfx->canvas_ctx->font = spel.gfx->canvas_ctx->geist;
		return;
	}
	spel.gfx->canvas_ctx->font = font;
}

void spel_canvas_text_align_set(spel_canvas_text_align align)
{
	spel.gfx->canvas_ctx->text_align = align;
}

void spel_canvas_font_size_set(float size)
{
	spel.gfx->canvas_ctx->font_size = size;
}

spel_font spel_canvas_font_sans_serif()
{
	return spel.gfx->canvas_ctx->geist;
}

spel_font spel_canvas_font_monospace()
{
	return spel.gfx->canvas_ctx->vga;
}

void spel_canvas_print_internal(spel_vec2 position, float maxWidth, const char* fmt,
								va_list args)
{
	static char buf[1024];
	vsnprintf(buf, sizeof(buf), fmt, args);

	spel_canvas_draw_text_internal(buf, position, maxWidth);
}

void spel_canvas_print(spel_vec2 position, const char* fmt, ...)
{
	va_list argptr;
	va_start(argptr, fmt);
	spel_canvas_print_internal(position, 0, fmt, argptr);
	va_end(argptr);
}

void spel_canvas_print_wrapped(spel_vec2 position, float maxWidth, const char* fmt, ...)
{
	va_list argptr;
	va_start(argptr, fmt);
	spel_canvas_print_internal(position, maxWidth, fmt, argptr);
	va_end(argptr);
}

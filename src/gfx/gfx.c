#include "backends/gl/gfx_vtable_gl.h"
#include "core/entry.h"
#include "core/log.h"
#include "core/macros.h"
#include "core/types.h"
#include "core/window.h"
#include "gfx/gfx_buffer.h"
#include "gfx/gfx_cmdlist.h"
#include "gfx/gfx_commands.h"
#include "gfx/gfx_context.h"
#include "gfx/gfx_internal.h"
#include "gfx/gfx_types.h"
#include "utils/internal/xxhash.h"
#include "utils/path.h"
#define STB_IMAGE_IMPLEMENTATION
#define STBI_MALLOC(sz) spel_memory_malloc(sz, SPEL_MEM_TAG_GFX)
// stb expects realloc(ptr, new_size); previous macro had parameters flipped.
#define STBI_REALLOC(p, sz) spel_memory_realloc(p, sz, SPEL_MEM_TAG_GFX)
#define STBI_FREE(p) spel_memory_free(p)
#include "utils/internal/stb_image.h"
#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

void spel_gfx_context_default_data(spel_gfx_context ctx);
void spel_checker_rgba8_make(uint8_t* out, int width, int height, int tileSize);

sp_api void spel_gfx_context_conf(spel_gfx_backend backend)
{
	switch (backend)
	{
	case SPEL_GFX_BACKEND_OPENGL:
		spel_gfx_context_conf_gl();
		break;
	}
}

sp_api spel_gfx_context spel_gfx_context_create(spel_gfx_context_desc* desc)
{
	spel_gfx_context ctx =
		(spel_gfx_context)sp_malloc(sizeof(spel_gfx_context_t), SPEL_MEM_TAG_GFX);
	ctx->backend = desc->backend;
	ctx->debug = desc->debug;

	spel_vec2 fb = spel_window_framebuffer_size();
	ctx->fb_width = (int)fb.x;
	ctx->fb_height = (int)fb.y;

	// Init caches before any allocations performed inside backend creation.
	ctx->pipeline_cache.entries = NULL;
	ctx->pipeline_cache.capacity = 0;
	ctx->pipeline_cache.count = 0;

	ctx->sampler_cache.entries = NULL;
	ctx->sampler_cache.capacity = 0;
	ctx->sampler_cache.count = 0;

	for (size_t i = 0; i < sp_array_size(ctx->shaders); i++)
	{
		ctx->shaders[i] = NULL;
	}

	switch (ctx->backend)
	{
	case SPEL_GFX_BACKEND_OPENGL:
		spel_gfx_context_create_gl(ctx);
		break;
	}

	if (ctx->vt == NULL)
	{
		sp_error(SPEL_ERR_CONTEXT_FAILED, "backend creation failed");
		sp_free(ctx);
		return NULL;
	}

	spel_gfx_context_default_data(ctx);

	return ctx;
}

sp_api void spel_gfx_context_destroy(spel_gfx_context ctx)
{
	ctx->vt->ctx_destroy(ctx);
	sp_free(ctx);
}

sp_api void spel_gfx_frame_begin(spel_gfx_context ctx)
{
	ctx->vt->frame_begin(ctx);
}

sp_api void spel_gfx_frame_present(spel_gfx_context ctx)
{
	ctx->vt->frame_end(ctx);
}

sp_api spel_gfx_cmdlist spel_gfx_cmdlist_create(spel_gfx_context ctx)
{
	return ctx->vt->cmdlist_create(ctx);
}

sp_api void spel_gfx_cmdlist_destroy(spel_gfx_cmdlist cmdlist)
{
	cmdlist->ctx->vt->cmdlist_destroy(cmdlist);
}

sp_api void spel_gfx_cmdlist_submit(spel_gfx_cmdlist cmdlist)
{
	cmdlist->ctx->vt->cmdlist_submit(cmdlist);
}

sp_api spel_gfx_buffer spel_gfx_buffer_create(spel_gfx_context ctx,
											  const spel_gfx_buffer_desc* desc)
{
	return ctx->vt->buffer_create(ctx, desc);
}

sp_api void spel_gfx_buffer_destroy(spel_gfx_buffer buf)
{
	buf->ctx->vt->buffer_destroy(buf);
}

sp_api void spel_gfx_buffer_update(spel_gfx_buffer buf, const void* data, size_t size,
								   size_t offset)
{
	buf->ctx->vt->buffer_update(buf, data, size, offset);
}

sp_api void* spel_gfx_buffer_map(spel_gfx_buffer buf, size_t offset, size_t size,
								 spel_gfx_access access)
{
	return buf->ctx->vt->buffer_map(buf, offset, size, access);
}

sp_api void spel_gfx_buffer_unmap(spel_gfx_buffer buf)
{
	buf->ctx->vt->buffer_unmap(buf);
}

sp_api spel_gfx_shader spel_gfx_shader_create(spel_gfx_context ctx,
											  spel_gfx_shader_desc* desc)
{
	return ctx->vt->shader_create(ctx, desc);
}

sp_api void spel_gfx_shader_destroy(spel_gfx_shader shader)
{
	shader->ctx->vt->shader_destroy(shader);
}

sp_api spel_gfx_shader spel_gfx_shader_load(spel_gfx_context ctx, const char* path)
{
	if (!spel_path_exists(path))
	{
		sp_log(SPEL_SEV_ERROR, SPEL_ERR_FILE_NOT_FOUND, path, SPEL_DATA_STRING,
			   strlen(path), "file %s does not exist", path);
		return NULL;
	}

	char* buffer = NULL;
	long length = 0;
	FILE* f = fopen(path, "rb");

	if (f)
	{
		fseek(f, 0, SEEK_END);
		length = ftell(f);
		fseek(f, 0, SEEK_SET);
		buffer = sp_malloc(length, SPEL_MEM_TAG_GFX);
		if (buffer)
		{
			fread(buffer, 1, length, f);
		}
		fclose(f);
	}

	spel_gfx_shader_desc shader_desc;
	shader_desc.debug_name = spel_path_filename(path);
	shader_desc.source = buffer;
	shader_desc.source_size = length;

	spel_gfx_shader shader = spel_gfx_shader_create(spel.gfx, &shader_desc);
	sp_free(buffer);
	return shader;
}

sp_api void spel_gfx_cmd_clear(spel_gfx_cmdlist cl, spel_color color)
{
	uint64_t start_offset = cl->offset;
	spel_gfx_clear_cmd* cmd = (spel_gfx_clear_cmd*)cl->ctx->vt->cmdlist_alloc(
		cl, sizeof(*cmd), _Alignof(spel_gfx_clear_cmd));

	cmd->hdr.type = SPEL_GFX_CMD_CLEAR;
	cmd->hdr.size = (uint16_t)(cl->offset - start_offset);
	cmd->color = color;
}

sp_api void spel_gfx_cmd_bind_vertex(spel_gfx_cmdlist cl, uint32_t stream,
									 spel_gfx_buffer buf, size_t offset)
{
	uint64_t start_offset = cl->offset;
	spel_gfx_bind_vertex_cmd* cmd = (spel_gfx_bind_vertex_cmd*)cl->ctx->vt->cmdlist_alloc(
		cl, sizeof(*cmd), _Alignof(spel_gfx_bind_vertex_cmd));

	cmd->hdr.type = SPEL_GFX_CMD_BIND_VERTEX;
	cmd->hdr.size = (uint16_t)(cl->offset - start_offset);
	cmd->stream = stream;
	cmd->buf = buf;
	cmd->offset = offset;
}

sp_api void spel_gfx_cmd_bind_index(spel_gfx_cmdlist cl, spel_gfx_buffer buf,
									spel_gfx_index_type type, size_t offset)
{
	uint64_t start_offset = cl->offset;
	spel_gfx_bind_index_cmd* cmd = (spel_gfx_bind_index_cmd*)cl->ctx->vt->cmdlist_alloc(
		cl, sizeof(*cmd), _Alignof(spel_gfx_bind_index_cmd));

	cmd->hdr.type = SPEL_GFX_CMD_BIND_INDEX;
	cmd->hdr.size = (uint16_t)(cl->offset - start_offset);
	cmd->buf = buf;
	cmd->offset = offset;
	cmd->type = type;
}

sp_api void spel_gfx_cmd_bind_pipeline(spel_gfx_cmdlist cl, spel_gfx_pipeline pipeline)
{
	uint64_t start_offset = cl->offset;
	spel_gfx_bind_pipeline_cmd* cmd =
		(spel_gfx_bind_pipeline_cmd*)cl->ctx->vt->cmdlist_alloc(
			cl, sizeof(*cmd), _Alignof(spel_gfx_bind_pipeline_cmd));

	cmd->hdr.type = SPEL_GFX_CMD_BIND_PIPELINE;
	cmd->hdr.size = (uint16_t)(cl->offset - start_offset);
	cmd->pipeline = pipeline;
}

sp_api void spel_gfx_cmd_draw(spel_gfx_cmdlist cl, uint32_t vertexCount,
							  uint32_t firstVertex)
{
	uint64_t start_offset = cl->offset;
	spel_gfx_draw_cmd* cmd = (spel_gfx_draw_cmd*)cl->ctx->vt->cmdlist_alloc(
		cl, sizeof(*cmd), _Alignof(spel_gfx_draw_cmd));

	cmd->hdr.type = SPEL_GFX_CMD_DRAW;
	cmd->hdr.size = (uint16_t)(cl->offset - start_offset);
	cmd->vertex_count = vertexCount;
	cmd->first_vertex = firstVertex;
}

sp_api void spel_gfx_cmd_draw_indexed(spel_gfx_cmdlist cl, uint32_t indexCount,
									  uint32_t firstIndex, int32_t vertexOffset)
{
	uint64_t start_offset = cl->offset;
	spel_gfx_draw_indexed_cmd* cmd =
		(spel_gfx_draw_indexed_cmd*)cl->ctx->vt->cmdlist_alloc(
			cl, sizeof(*cmd), _Alignof(spel_gfx_draw_indexed_cmd));

	cmd->hdr.type = SPEL_GFX_CMD_DRAW_INDEXED;
	cmd->hdr.size = (uint16_t)(cl->offset - start_offset);
	cmd->index_count = indexCount;
	cmd->first_index = firstIndex;
	cmd->vertex_offset = vertexOffset;
}

sp_api spel_gfx_cmdlist spel_gfx_cmdlist_default(spel_gfx_context ctx)
{
	return ctx->cmdlist;
}

sp_api spel_gfx_pipeline spel_gfx_pipeline_create(spel_gfx_context ctx,
												  const spel_gfx_pipeline_desc* desc)
{
	return ctx->vt->pipeline_create(ctx, desc);
}

sp_hidden void spel_gfx_pipeline_cache_remove(spel_gfx_pipeline_cache* cache, uint64_t hash,
										   spel_gfx_pipeline pipeline)
{
	if (cache->capacity == 0)
	{
		return;
	}

	uint32_t mask = cache->capacity - 1;
	uint32_t index = (uint32_t)hash & mask;

	for (;;)
	{
		spel_gfx_pipeline_cache_entry* e = &cache->entries[index];

		if (!e->pipeline)
		{
			return; // not found
		}

		if (e->pipeline == pipeline)
		{
			cache->count--;
			e->pipeline = NULL;
			e->hash = 0;

			for (uint32_t next = (index + 1) & mask; cache->entries[next].pipeline;
				 next = (next + 1) & mask)
			{
				spel_gfx_pipeline_cache_entry entry = cache->entries[next];
				cache->entries[next].pipeline = NULL;
				cache->entries[next].hash = 0;

				uint32_t dest = (uint32_t)entry.hash & mask;
				while (cache->entries[dest].pipeline)
				{
					dest = (dest + 1) & mask;
				}

				cache->entries[dest] = entry;
			}

			return;
		}

		index = (index + 1) & mask;
	}
}

sp_api void spel_gfx_pipeline_destroy(spel_gfx_pipeline pipeline)
{
	spel_gfx_pipeline_cache_remove(&pipeline->ctx->pipeline_cache, pipeline->hash,
								   pipeline);
	spel_memory_free(pipeline->reflection.samplers);
	spel_memory_free(pipeline->reflection.storage);
	spel_memory_free(pipeline->reflection.uniforms);
	pipeline->ctx->vt->pipeline_destroy(pipeline);
}

sp_hidden spel_gfx_pipeline spel_gfx_pipeline_cache_get_or_create(
	spel_gfx_pipeline_cache* cache, uint64_t hash, spel_gfx_pipeline pipeline)
{
	if (cache->count * 10 >= cache->capacity * 7)
	{
		uint32_t old_capacity = cache->capacity;
		uint32_t new_capacity = old_capacity ? old_capacity * 2 : 8;

		spel_gfx_pipeline_cache_entry* new_entries =
			sp_malloc(new_capacity * sizeof(*new_entries), SPEL_MEM_TAG_GFX);
		memset(new_entries, 0, new_capacity * sizeof(*new_entries));

		for (uint32_t i = 0; i < old_capacity; ++i)
		{
			spel_gfx_pipeline_cache_entry* old = &cache->entries[i];

			if (!old->pipeline)
			{
				continue;
			}

			uint32_t index = (uint32_t)old->hash & (new_capacity - 1);

			while (new_entries[index].pipeline)
			{
				index = (index + 1) & (new_capacity - 1);
			}

			new_entries[index] = *old;
		}

		sp_free(cache->entries);
		cache->entries = new_entries;
		cache->capacity = new_capacity;
	}

	uint32_t mask = cache->capacity - 1;
	uint32_t index = (uint32_t)hash & (cache->capacity - 1);

	for (;;)
	{
		spel_gfx_pipeline_cache_entry* e = &cache->entries[index];

		if (e->pipeline == NULL)
		{
			e->hash = hash;
			e->pipeline = pipeline;
			cache->count++;
			return pipeline;
		}

		if (e->hash == hash)
		{
			return e->pipeline;
		}

		index = (index + 1) & mask;
	}
}

sp_api bool spel_gfx_texture_validate(const spel_gfx_texture_desc* desc)
{
	bool valid = true;

	if (desc->width <= 0 || desc->height <= 0 || desc->depth <= 0 || desc->mip_count <= 0)
	{
		valid = false;
	}

	switch (desc->type)
	{
	case SPEL_GFX_TEXTURE_2D:
		if (desc->depth != 1)
		{
			valid = false;
		}
		break;

	case SPEL_GFX_TEXTURE_2D_ARRAY:
	case SPEL_GFX_TEXTURE_3D:
		if (desc->depth <= 0)
		{
			valid = false;
		}
		break;

	case SPEL_GFX_TEXTURE_CUBE:
		if (desc->depth != 6)
		{
			valid = false;
		}
		break;
	}

	uint32_t max_mips = 1 + (uint32_t)floor(log2(fmax(desc->width, desc->height)));

	if (desc->mip_count > max_mips)
	{
		valid = false;
	}

	return valid;
}

sp_api spel_gfx_sampler spel_gfx_sampler_get(spel_gfx_context ctx,
											 const spel_gfx_sampler_desc* desc)
{
	spel_gfx_sampler_cache* cache = &ctx->sampler_cache;

	if (cache->count * 10 >= cache->capacity * 7)
	{
		uint32_t old_capacity = cache->capacity;
		uint32_t new_capacity = old_capacity ? old_capacity * 2 : 8;

		spel_gfx_sampler_cache_entry* new_entries =
			sp_malloc(new_capacity * sizeof(*new_entries), SPEL_MEM_TAG_GFX);
		memset(new_entries, 0, new_capacity * sizeof(*new_entries));

		for (uint32_t i = 0; i < old_capacity; ++i)
		{
			spel_gfx_sampler_cache_entry* old = &cache->entries[i];

			if (!old->sampler)
			{
				continue;
			}

			uint32_t index = (uint32_t)old->hash & (new_capacity - 1);

			while (new_entries[index].sampler)
			{
				index = (index + 1) & (new_capacity - 1);
			}

			new_entries[index] = *old;
		}

		sp_free(cache->entries);
		cache->entries = new_entries;
		cache->capacity = new_capacity;
	}

	uint32_t mask = cache->capacity - 1;
	uint64_t hash = XXH3_64bits(desc, sizeof(*desc));
	uint32_t index = (uint32_t)hash & (cache->capacity - 1);

	for (;;)
	{
		spel_gfx_sampler_cache_entry* e = &cache->entries[index];

		if (e->sampler == NULL)
		{
			e->hash = hash;
			e->sampler = ctx->vt->sampler_create(ctx, desc);
			cache->count++;
			return e->sampler;
		}

		if (e->hash == hash)
		{
			return e->sampler;
		}

		index = (index + 1) & mask;
	}
}

sp_api void spel_gfx_cmd_bind_texture(spel_gfx_cmdlist cl, uint32_t slot,
									  spel_gfx_texture texture)
{
	uint64_t start_offset = cl->offset;
	spel_gfx_bind_texture_cmd* cmd =
		(spel_gfx_bind_texture_cmd*)cl->ctx->vt->cmdlist_alloc(
			cl, sizeof(*cmd), _Alignof(spel_gfx_bind_texture_cmd));

	cmd->hdr.type = SPEL_GFX_CMD_BIND_TEXTURE;
	cmd->hdr.size = (uint16_t)(cl->offset - start_offset);
	cmd->texture = texture;
	cmd->slot = slot;
}

sp_api void spel_gfx_cmd_bind_sampler(spel_gfx_cmdlist cl, uint32_t slot,
									  spel_gfx_sampler sampler)
{
	uint64_t start_offset = cl->offset;
	spel_gfx_bind_sampler_cmd* cmd =
		(spel_gfx_bind_sampler_cmd*)cl->ctx->vt->cmdlist_alloc(
			cl, sizeof(*cmd), _Alignof(spel_gfx_bind_sampler_cmd));

	cmd->hdr.type = SPEL_GFX_CMD_BIND_SAMPLER;
	cmd->hdr.size = (uint16_t)(cl->offset - start_offset);
	cmd->sampler = sampler;
	cmd->slot = slot;
}

sp_api void spel_gfx_cmd_bind_image(spel_gfx_cmdlist cl, uint32_t slot,
									spel_gfx_texture texture, spel_gfx_sampler sampler)
{
	uint64_t start_offset = cl->offset;
	spel_gfx_bind_image_cmd* cmd = (spel_gfx_bind_image_cmd*)cl->ctx->vt->cmdlist_alloc(
		cl, sizeof(*cmd), _Alignof(spel_gfx_bind_image_cmd));

	cmd->hdr.type = SPEL_GFX_CMD_BIND_IMAGE;
	cmd->hdr.size = (uint16_t)(cl->offset - start_offset);
	cmd->texture = texture;
	cmd->sampler = sampler;
	cmd->slot = slot;
}

void spel_gfx_context_default_data(spel_gfx_context ctx)
{
	ctx->checkerboard = NULL;
	ctx->white_tex = NULL;
	ctx->cmdlist = spel_gfx_cmdlist_create(ctx);

	spel_gfx_sampler_desc desc = spel_gfx_sampler_default();
	ctx->default_sampler = spel_gfx_sampler_get(ctx, &desc);

	static uint8_t WHITE_TEX_DATA[4] = {0xFF, 0xFF, 0xFF, 0xFF};

	static const spel_gfx_texture_desc WHITE_TEX = {
		.type = SPEL_GFX_TEXTURE_2D,
		.format = SPEL_GFX_TEXTURE_FMT_RGBA8_UNORM,
		.usage = SPEL_GFX_TEXTURE_USAGE_SAMPLED,
		.width = 1,
		.height = 1,
		.depth = 1,
		.mip_count = 1,
		.data = WHITE_TEX_DATA,
		.data_size = sizeof(WHITE_TEX_DATA)};
	ctx->white_tex = spel_gfx_texture_create(ctx, &WHITE_TEX);

	static uint8_t checker_pixels[128 * 128 * 4];
	spel_checker_rgba8_make(checker_pixels, 128, 128, 16);

	static const spel_gfx_texture_desc CHECKER_DATA = {
		.type = SPEL_GFX_TEXTURE_2D,
		.format = SPEL_GFX_TEXTURE_FMT_RGBA8_UNORM,
		.usage = SPEL_GFX_TEXTURE_USAGE_SAMPLED,
		.width = 128,
		.height = 128,
		.depth = 1,
		.mip_count = 1,
		.data = checker_pixels,
		.data_size = sizeof(checker_pixels)};
	ctx->checkerboard = spel_gfx_texture_create(ctx, &CHECKER_DATA);

	ctx->white_tex->internal = true;
	ctx->checkerboard->internal = true;
}

void spel_checker_rgba8_make(uint8_t* out, int width, int height, int tileSize)
{
	for (int y = 0; y < height; y++)
	{
		for (int x = 0; x < width; x++)
		{
			int tx = x / tileSize;
			int ty = y / tileSize;
			int checker = (tx ^ ty) & 1;

			uint8_t c = checker ? 0x33 : 0x22;

			int i = (y * width + x) * 4;
			out[i + 0] = c;
			out[i + 1] = c;
			out[i + 2] = c;
			out[i + 3] = 255;
		}
	}
}

sp_api spel_gfx_texture spel_gfx_texture_create(spel_gfx_context ctx,
												const spel_gfx_texture_desc* desc)
{
	return ctx->vt->texture_create(ctx, desc);
}

sp_api void spel_gfx_texture_destroy(spel_gfx_texture texture)
{
	texture->ctx->vt->texture_destroy(texture);
}

sp_api spel_gfx_texture spel_gfx_texture_white_get(spel_gfx_context ctx)
{
	return ctx->white_tex;
}

sp_api spel_gfx_texture spel_gfx_texture_checker_get(spel_gfx_context ctx)
{
	return ctx->checkerboard;
}

sp_api spel_gfx_sampler_desc spel_gfx_sampler_default()
{
	static const spel_gfx_sampler_desc DEFAULT_SAMPLER = {
		.min = SPEL_GFX_SAMPLER_FILTER_LINEAR,
		.mag = SPEL_GFX_SAMPLER_FILTER_LINEAR,
		.mip = SPEL_GFX_SAMPLER_MIP_LINEAR,

		.wrap_u = SPEL_GFX_SAMPLER_WRAP_REPEAT,
		.wrap_v = SPEL_GFX_SAMPLER_WRAP_REPEAT,
		.wrap_w = SPEL_GFX_SAMPLER_WRAP_REPEAT,

		.lod_bias = 0.0F,
		.max_aniso = 1.0F};
	return DEFAULT_SAMPLER;
}

uint8_t* rgb_to_rgba(const uint8_t* src, int pixelCount)
{
	uint8_t* dst = sp_malloc((unsigned long)(pixelCount * 4), SPEL_MEM_TAG_GFX);
	if (!dst)
	{
		return NULL;
	}

	for (int i = 0; i < pixelCount; i++)
	{
		dst[(i * 4) + 0] = src[(i * 3) + 0]; // R
		dst[(i * 4) + 1] = src[(i * 3) + 1]; // G
		dst[(i * 4) + 2] = src[(i * 3) + 2]; // B
		dst[(i * 4) + 3] = 255;				 // A
	}

	return dst;
}

sp_api spel_gfx_texture spel_gfx_texture_load(spel_gfx_context ctx, const char* path,
											  const spel_gfx_texture_load_desc* desc)
{
	int w;
	int h;
	int comp;
	stbi_uc* pixels = stbi_load(path, &w, &h, &comp, 0);
	if (!pixels)
	{
		return spel_gfx_texture_checker_get(ctx);
	}

	uint8_t* upload_pixels = pixels;
	size_t upload_size = 0;

	spel_gfx_texture_desc tex = {0};
	tex.type = SPEL_GFX_TEXTURE_2D;
	tex.width = w;
	tex.height = h;
	tex.depth = 1;
	tex.mip_count =
		desc->mip_count ? desc->mip_count : (1 + (uint32_t)floor(log2(fmax(w, h))));
	tex.usage = desc->usage;

	if (desc->format != SPEL_GFX_TEXTURE_FMT_UNKNOWN)
	{
		tex.format = desc->format;
		upload_size = (size_t)w * h * comp;
	}
	else
	{
		if ((int)desc->srgb && comp < 3)
		{
			sp_error(SPEL_ERR_INVALID_ARGUMENT, "sRGB requested for non-RGB texture");
			stbi_image_free(pixels);
			return spel_gfx_texture_checker_get(ctx);
		}

		switch (comp)
		{
		case 1:
			tex.format = SPEL_GFX_TEXTURE_FMT_R8_UNORM;
			upload_size = (size_t)w * h;
			break;

		case 2:
			tex.format = SPEL_GFX_TEXTURE_FMT_RG8_UNORM;
			upload_size = (size_t)w * h * 2;
			break;

		case 3:
			upload_pixels = rgb_to_rgba(pixels, w * h);
			upload_size = (size_t)w * h * 4;
			stbi_image_free(pixels);
			pixels = NULL;

			tex.format = (int)desc->srgb ? SPEL_GFX_TEXTURE_FMT_RGBA8_SRGB
										 : SPEL_GFX_TEXTURE_FMT_RGBA8_UNORM;
			break;

		case 4:
			tex.format = (int)desc->srgb ? SPEL_GFX_TEXTURE_FMT_RGBA8_SRGB
										 : SPEL_GFX_TEXTURE_FMT_RGBA8_UNORM;
			upload_size = (size_t)w * h * 4;
			break;

		default:
			stbi_image_free(pixels);
			return spel_gfx_texture_checker_get(ctx);
		}
	}

	tex.data = upload_pixels;
	tex.data_size = upload_size;

	spel_gfx_texture out = spel_gfx_texture_create(ctx, &tex);

	if (upload_pixels && upload_pixels != pixels)
	{
		sp_free(upload_pixels);
	}

	if (pixels)
	{
		stbi_image_free(pixels);
	}

	return out;
}

sp_api spel_gfx_texture spel_gfx_texture_load_color(spel_gfx_context ctx,
													const char* path)
{
	spel_gfx_texture_load_desc desc = {.format = SPEL_GFX_TEXTURE_FMT_RGBA8_SRGB,
									   .usage = SPEL_GFX_TEXTURE_USAGE_SAMPLED,
									   .mip_count = 0,
									   .srgb = true};

	return spel_gfx_texture_load(ctx, path, &desc);
}

sp_api spel_gfx_texture spel_gfx_texture_load_linear(spel_gfx_context ctx,
													 const char* path)
{
	spel_gfx_texture_load_desc desc = {.format = SPEL_GFX_TEXTURE_FMT_RGBA8_UNORM,
									   .usage = SPEL_GFX_TEXTURE_USAGE_SAMPLED,
									   .mip_count = 0,
									   .srgb = false};

	return spel_gfx_texture_load(ctx, path, &desc);
}

sp_api void spel_gfx_cmd_viewport(spel_gfx_cmdlist cl, int x, int y, int width,
								  int height)
{
	uint64_t start_offset = cl->offset;
	spel_gfx_viewport_cmd* cmd = (spel_gfx_viewport_cmd*)cl->ctx->vt->cmdlist_alloc(
		cl, sizeof(*cmd), _Alignof(spel_gfx_viewport_cmd));

	cmd->hdr.type = SPEL_GFX_CMD_VIEWPORT;
	cmd->hdr.size = (uint16_t)(cl->offset - start_offset);
	cmd->x = x;
	cmd->y = y;
	cmd->width = width;
	cmd->height = height;
}

sp_api void spel_gfx_cmd_scissor(spel_gfx_cmdlist cl, int x, int y, int width, int height)
{
	uint64_t start_offset = cl->offset;
	spel_gfx_scissor_cmd* cmd = (spel_gfx_scissor_cmd*)cl->ctx->vt->cmdlist_alloc(
		cl, sizeof(*cmd), _Alignof(spel_gfx_scissor_cmd));

	cmd->hdr.type = SPEL_GFX_CMD_SCISSOR;
	cmd->hdr.size = (uint16_t)(cl->offset - start_offset);
	cmd->x = x;
	cmd->y = y;
	cmd->width = width;
	cmd->height = height;
}

#ifdef DEBUG
sp_hidden extern const char* spel_gfx_shader_type_str(spel_gfx_shader_stage stage)
{
	switch (stage)
	{
	case SPEL_GFX_SHADER_VERTEX:
		return "vertex";

	case SPEL_GFX_SHADER_FRAGMENT:
		return "fragment";

	case SPEL_GFX_SHADER_GEOMETRY:
		return "geometry";

	case SPEL_GFX_SHADER_COMPUTE:
		return "compute";
	}
}
#endif

sp_api spel_gfx_texture spel_gfx_texture_color_create(spel_gfx_context ctx,
													  spel_color color)
{
	const spel_gfx_texture_desc COLOR_TEX = {
		.type = SPEL_GFX_TEXTURE_2D,
		.format = SPEL_GFX_TEXTURE_FMT_RGBA8_UNORM,
		.usage = SPEL_GFX_TEXTURE_USAGE_SAMPLED,
		.width = 1,
		.height = 1,
		.depth = 1,
		.mip_count = 1,
		.data = &color,
		.data_size = sizeof(color)};
	return spel_gfx_texture_create(ctx, &COLOR_TEX);
}

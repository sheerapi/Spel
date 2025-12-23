#include "backends/gl/gfx_vtable_gl.h"
#include "core/entry.h"
#include "core/macros.h"
#include "core/types.h"
#include "gfx/gfx_buffer.h"
#include "gfx/gfx_cmdlist.h"
#include "gfx/gfx_commands.h"
#include "gfx/gfx_context.h"
#include "gfx/gfx_internal.h"
#include "gfx/gfx_types.h"
#include "utils/internal/xxhash.h"
#include "utils/path.h"
#include <math.h>
#include <stdio.h>

void spel_gfx_context_default_data(spel_gfx_context ctx);
void spel_checker_rgba8_make(uint8_t* out, int width, int height, int tileSize);

void spel_gfx_context_conf(spel_gfx_backend backend)
{
	switch (backend)
	{
	case SPEL_GFX_BACKEND_OPENGL:
		spel_gfx_context_conf_gl();
		break;
	}
}

spel_gfx_context spel_gfx_context_create(spel_gfx_context_desc* desc)
{
	spel_gfx_context ctx =
		(spel_gfx_context)sp_malloc(sizeof(spel_gfx_context_t), SPEL_MEM_TAG_GFX);
	ctx->backend = desc->backend;
	ctx->debug = desc->debug;

	for (size_t i = 0; i < sp_array_size(ctx->shaders); i++)
	{
		ctx->shaders[i] = nullptr;
	}

	switch (ctx->backend)
	{
	case SPEL_GFX_BACKEND_OPENGL:
		spel_gfx_context_create_gl(ctx);
		break;
	}

	spel_gfx_context_default_data(ctx);

	return ctx;
}

void spel_gfx_context_destroy(spel_gfx_context ctx)
{
	ctx->vt->ctx_destroy(ctx);
	sp_free(ctx);
}

void spel_gfx_frame_begin(spel_gfx_context ctx)
{
	ctx->vt->frame_begin(ctx);
}

void spel_gfx_frame_present(spel_gfx_context ctx)
{
	ctx->vt->frame_end(ctx);
}

spel_gfx_cmdlist spel_gfx_cmdlist_create(spel_gfx_context ctx)
{
	return ctx->vt->cmdlist_create(ctx);
}

void spel_gfx_cmdlist_destroy(spel_gfx_cmdlist cmdlist)
{
	cmdlist->ctx->vt->cmdlist_destroy(cmdlist);
}

void spel_gfx_cmdlist_submit(spel_gfx_cmdlist cmdlist)
{
	cmdlist->ctx->vt->cmdlist_submit(cmdlist);
}

spel_gfx_buffer spel_gfx_buffer_create(spel_gfx_context ctx,
									   const spel_gfx_buffer_desc* desc)
{
	return ctx->vt->buffer_create(ctx, desc);
}

void spel_gfx_buffer_destroy(spel_gfx_buffer buf)
{
	buf->ctx->vt->buffer_destroy(buf);
}

void spel_gfx_buffer_update(spel_gfx_buffer buf, const void* data, size_t size,
							size_t offset)
{
	buf->ctx->vt->buffer_update(buf, data, size, offset);
}

void* spel_gfx_buffer_map(spel_gfx_buffer buf, size_t offset, size_t size,
						  spel_gfx_buffer_access access)
{
	return buf->ctx->vt->buffer_map(buf, offset, size, access);
}

void spel_gfx_buffer_unmap(spel_gfx_buffer buf)
{
	buf->ctx->vt->buffer_unmap(buf);
}

spel_gfx_shader spel_gfx_shader_create(spel_gfx_context ctx,
									   const spel_gfx_shader_desc* desc)
{
	return ctx->vt->shader_create(ctx, desc);
}

void spel_gfx_shader_destroy(spel_gfx_shader shader)
{
	shader->ctx->vt->shader_destroy(shader);
}

spel_gfx_shader spel_gfx_shader_load(spel_gfx_context ctx, const char* path,
									 const char* entry, spel_gfx_shader_stage stage)
{
	if (!spel_path_exists(path))
	{
		char buffer[512];
		snprintf(buffer, 512, "file %s does not exist", path);
		spel_error(buffer);
		return nullptr;
	}

	char* buffer = nullptr;
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
	shader_desc.stage = stage;
	shader_desc.entry = entry;
	shader_desc.debug_name = spel_path_filename(path);
	shader_desc.source = buffer;
	shader_desc.source_size = length;

	spel_gfx_shader shader = spel_gfx_shader_create(spel.gfx, &shader_desc);
	sp_free(buffer);
	return shader;
}

void spel_gfx_cmd_clear(spel_gfx_cmdlist cl, spel_color color)
{
	uint64_t start_offset = cl->offset;
	spel_gfx_clear_cmd* cmd = (spel_gfx_clear_cmd*)cl->ctx->vt->cmdlist_alloc(
		cl, sizeof(*cmd), alignof(spel_gfx_clear_cmd));

	cmd->hdr.type = SPEL_GFX_CMD_CLEAR;
	cmd->hdr.size = (uint16_t)(cl->offset - start_offset);
	cmd->color = color;
}

void spel_gfx_cmd_bind_vertex(spel_gfx_cmdlist cl, uint32_t stream, spel_gfx_buffer buf,
							  size_t offset)
{
	uint64_t start_offset = cl->offset;
	spel_gfx_bind_vertex_cmd* cmd = (spel_gfx_bind_vertex_cmd*)cl->ctx->vt->cmdlist_alloc(
		cl, sizeof(*cmd), alignof(spel_gfx_bind_vertex_cmd));

	cmd->hdr.type = SPEL_GFX_CMD_BIND_VERTEX;
	cmd->hdr.size = (uint16_t)(cl->offset - start_offset);
	cmd->stream = stream;
	cmd->buf = buf;
	cmd->offset = offset;
}

void spel_gfx_cmd_bind_index(spel_gfx_cmdlist cl, spel_gfx_buffer buf,
							 spel_gfx_index_type type, size_t offset)
{
	uint64_t start_offset = cl->offset;
	spel_gfx_bind_index_cmd* cmd = (spel_gfx_bind_index_cmd*)cl->ctx->vt->cmdlist_alloc(
		cl, sizeof(*cmd), alignof(spel_gfx_bind_index_cmd));

	cmd->hdr.type = SPEL_GFX_CMD_BIND_INDEX;
	cmd->hdr.size = (uint16_t)(cl->offset - start_offset);
	cmd->buf = buf;
	cmd->offset = offset;
	cmd->type = type;
}

void spel_gfx_cmd_bind_pipeline(spel_gfx_cmdlist cl, spel_gfx_pipeline pipeline)
{
	uint64_t start_offset = cl->offset;
	spel_gfx_bind_pipeline_cmd* cmd =
		(spel_gfx_bind_pipeline_cmd*)cl->ctx->vt->cmdlist_alloc(
			cl, sizeof(*cmd), alignof(spel_gfx_bind_pipeline_cmd));

	cmd->hdr.type = SPEL_GFX_CMD_BIND_PIPELINE;
	cmd->hdr.size = (uint16_t)(cl->offset - start_offset);
	cmd->pipeline = pipeline;
}

void spel_gfx_cmd_draw(spel_gfx_cmdlist cl, uint32_t vertexCount, uint32_t firstVertex)
{
	uint64_t start_offset = cl->offset;
	spel_gfx_draw_cmd* cmd = (spel_gfx_draw_cmd*)cl->ctx->vt->cmdlist_alloc(
		cl, sizeof(*cmd), alignof(spel_gfx_draw_cmd));

	cmd->hdr.type = SPEL_GFX_CMD_DRAW;
	cmd->hdr.size = (uint16_t)(cl->offset - start_offset);
	cmd->vertex_count = vertexCount;
	cmd->first_vertex = firstVertex;
}

void spel_gfx_cmd_draw_indexed(spel_gfx_cmdlist cl, uint32_t indexCount,
							   uint32_t firstIndex, int32_t vertexOffset)
{
	uint64_t start_offset = cl->offset;
	spel_gfx_draw_indexed_cmd* cmd =
		(spel_gfx_draw_indexed_cmd*)cl->ctx->vt->cmdlist_alloc(
			cl, sizeof(*cmd), alignof(spel_gfx_draw_indexed_cmd));

	cmd->hdr.type = SPEL_GFX_CMD_DRAW_INDEXED;
	cmd->hdr.size = (uint16_t)(cl->offset - start_offset);
	cmd->index_count = indexCount;
	cmd->first_index = firstIndex;
	cmd->vertex_offset = vertexOffset;
}

spel_gfx_cmdlist spel_gfx_cmdlist_default(spel_gfx_context ctx)
{
	return ctx->cmdlist;
}

spel_gfx_pipeline spel_gfx_pipeline_create(spel_gfx_context ctx,
										   const spel_gfx_pipeline_desc* desc)
{
	return ctx->vt->pipeline_create(ctx, desc);
}

void spel_gfx_pipeline_destroy(spel_gfx_pipeline pipeline)
{
	pipeline->ctx->vt->pipeline_destroy(pipeline);
}

spel_gfx_pipeline spel_gfx_pipeline_cache_get_or_create(spel_gfx_pipeline_cache* cache,
														uint64_t hash,
														spel_gfx_pipeline pipeline)
{
	if (cache->count * 10 >= cache->capacity * 7)
	{
		uint32_t old_capacity = cache->capacity;
		uint32_t new_capacity = old_capacity ? old_capacity * 2 : 64;

		spel_gfx_pipeline_cache_entry* new_entries =
			sp_malloc(new_capacity * sizeof(*new_entries), SPEL_MEM_TAG_GFX);

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

		free(cache->entries);
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

void spel_gfx_texture_validate(const spel_gfx_texture_desc* desc)
{
	sp_assert(desc->width > 0, "width must be greater than 0");
	sp_assert(desc->height > 0, "height must be greater than 0");
	sp_assert(desc->depth > 0, "depth must be greater than 0");
	sp_assert(desc->mip_count > 0, "mipmap count must be greater than 0");

	switch (desc->type)
	{
	case SPEL_GFX_TEXTURE_2D:
		sp_assert(desc->depth == 1, "a 2d texture cannot have depth");
		break;

	case SPEL_GFX_TEXTURE_2D_ARRAY:
		sp_assert(desc->depth >= 1,
				  "you need at least 1 texture for a 2d tex array"); // layers
		break;

	case SPEL_GFX_TEXTURE_3D:
		sp_assert(desc->depth > 1, "a 3d texture must have depth");
		break;

	case SPEL_GFX_TEXTURE_CUBE:
		sp_assert(desc->depth == 6, "a cubemap must have exactly 6 faces");
		break;
	}

	uint32_t max_mips = 1 + (uint32_t)floor(log2(fmax(desc->width, desc->height)));

	sp_assert(desc->mip_count <= max_mips, "you cant have more mipmaps than the maximum");
}

spel_gfx_sampler spel_gfx_sampler_get(spel_gfx_context ctx,
									  const spel_gfx_sampler_desc* desc)
{
	spel_gfx_sampler_cache cache = ctx->sampler_cache;

	if (cache.count * 10 >= cache.capacity * 7)
	{
		uint32_t old_capacity = cache.capacity;
		uint32_t new_capacity = old_capacity ? old_capacity * 2 : 64;

		spel_gfx_sampler_cache_entry* new_entries =
			sp_malloc(new_capacity * sizeof(*new_entries), SPEL_MEM_TAG_GFX);

		for (uint32_t i = 0; i < old_capacity; ++i)
		{
			spel_gfx_sampler_cache_entry* old = &cache.entries[i];

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

		free(cache.entries);
		cache.entries = new_entries;
		cache.capacity = new_capacity;
	}

	uint32_t mask = cache.capacity - 1;
	uint64_t hash = XXH3_64bits(desc, sizeof(*desc));
	uint32_t index = (uint32_t)hash & (cache.capacity - 1);

	for (;;)
	{
		spel_gfx_sampler_cache_entry* e = &cache.entries[index];

		if (e->sampler == NULL)
		{
			e->hash = hash;
			e->sampler = ctx->vt->sampler_create(ctx, desc);
			cache.count++;
			return e->sampler;
		}

		if (e->hash == hash)
		{
			return e->sampler;
		}

		index = (index + 1) & mask;
	}
}

void spel_gfx_cmd_bind_texture(spel_gfx_cmdlist cl, uint32_t slot,
							   spel_gfx_texture texture)
{
	uint64_t start_offset = cl->offset;
	spel_gfx_bind_texture_cmd* cmd =
		(spel_gfx_bind_texture_cmd*)cl->ctx->vt->cmdlist_alloc(
			cl, sizeof(*cmd), alignof(spel_gfx_bind_texture_cmd));

	cmd->hdr.type = SPEL_GFX_CMD_BIND_TEXTURE;
	cmd->hdr.size = (uint16_t)(cl->offset - start_offset);
	cmd->texture = texture;
	cmd->slot = slot;
}

void spel_gfx_cmd_bind_sampler(spel_gfx_cmdlist cl, uint32_t slot,
							   spel_gfx_sampler sampler)
{
	uint64_t start_offset = cl->offset;
	spel_gfx_bind_sampler_cmd* cmd =
		(spel_gfx_bind_sampler_cmd*)cl->ctx->vt->cmdlist_alloc(
			cl, sizeof(*cmd), alignof(spel_gfx_bind_sampler_cmd));

	cmd->hdr.type = SPEL_GFX_CMD_BIND_SAMPLER;
	cmd->hdr.size = (uint16_t)(cl->offset - start_offset);
	cmd->sampler = sampler;
	cmd->slot = slot;
}

void spel_gfx_cmd_bind_image(spel_gfx_cmdlist cl, uint32_t slot, spel_gfx_texture texture,
							 spel_gfx_sampler sampler)
{
	uint64_t start_offset = cl->offset;
	spel_gfx_bind_image_cmd* cmd = (spel_gfx_bind_image_cmd*)cl->ctx->vt->cmdlist_alloc(
		cl, sizeof(*cmd), alignof(spel_gfx_bind_image_cmd));

	cmd->hdr.type = SPEL_GFX_CMD_BIND_IMAGE;
	cmd->hdr.size = (uint16_t)(cl->offset - start_offset);
	cmd->texture = texture;
	cmd->sampler = sampler;
	cmd->slot = slot;
}

void spel_gfx_context_default_data(spel_gfx_context ctx)
{
	ctx->cmdlist = spel_gfx_cmdlist_create(ctx);

	static const spel_gfx_sampler_desc DEFAULT_SAMPLER = {
		.min = SPEL_GFX_SAMPLER_FILTER_LINEAR,
		.mag = SPEL_GFX_SAMPLER_FILTER_LINEAR,
		.mip = SPEL_GFX_SAMPLER_MIP_LINEAR,

		.wrap_u = SPEL_GFX_SAMPLER_WRAP_REPEAT,
		.wrap_v = SPEL_GFX_SAMPLER_WRAP_REPEAT,
		.wrap_w = SPEL_GFX_SAMPLER_WRAP_REPEAT,

		.lod_bias = 0.0F,
		.max_aniso = 1.0F};

	ctx->default_sampler = spel_gfx_sampler_get(ctx, &DEFAULT_SAMPLER);

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

spel_gfx_texture spel_gfx_texture_create(spel_gfx_context ctx,
										 const spel_gfx_texture_desc* desc)
{
	return ctx->vt->texture_create(ctx, desc);
}

void spel_gfx_texture_destroy(spel_gfx_texture texture)
{
	texture->ctx->vt->texture_destroy(texture);
}

spel_gfx_texture spel_gfx_texture_white_get(spel_gfx_context ctx)
{
	return ctx->white_tex;
}

spel_gfx_texture spel_gfx_texture_checker_get(spel_gfx_context ctx)
{
	return ctx->checkerboard;
}
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
#include "utils/path.h"
#include <shaderc/shaderc.h>
#include <stdio.h>

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

	switch (ctx->backend)
	{
	case SPEL_GFX_BACKEND_OPENGL:
		spel_gfx_context_create_gl(ctx);
		break;
	}

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
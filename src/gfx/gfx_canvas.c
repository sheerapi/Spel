#include "gfx/gfx_canvas.h"
#include "core/log.h"
#include "core/memory.h"
#include "core/types.h"
#include "gfx/gfx_cmdlist.h"
#include "gfx/gfx_internal.h"
#include "gfx/gfx_types.h"
#include <stdio.h>

#define SPEL_CANVAS_VBUFFER_SIZE 16384

spel_hidden void spel_canvas_ctx_create(spel_gfx_context gfx);

spel_api spel_canvas spel_canvas_create(spel_gfx_context gfx, int width, int height,
										uint8_t flags)
{
	spel_canvas canvas = spel_memory_malloc(sizeof(*canvas), SPEL_MEM_TAG_GFX);

	if (gfx->canvas_ctx == NULL)
	{
		spel_canvas_ctx_create(gfx);
	}

	canvas->ctx = gfx->canvas_ctx;
	canvas->size.x = width;
	canvas->size.y = height;
	canvas->flags = flags;

	spel_gfx_texture_desc color_desc = {
		.type = SPEL_GFX_TEXTURE_2D,
		.depth = 1,
		.mip_count = 1,
		.width = width,
		.height = height,
		.format = SPEL_GFX_TEXTURE_FMT_RGBA8_UNORM,
		.usage = SPEL_GFX_TEXTURE_USAGE_RENDER | SPEL_GFX_TEXTURE_USAGE_SAMPLED,
	};

	canvas->color = spel_gfx_texture_create(gfx, &color_desc);
	canvas->depth = NULL;

	spel_gfx_framebuffer_desc fb_desc = {
		.color[0] = {.texture = canvas->color, .type = SPEL_GFX_ATTACHMENT_COLOR},
		.depth = {.texture = NULL},
		.color_count = 1,
		.width = width,
		.height = height,
		.auto_resize = flags & SPEL_CANVAS_AUTO_RESIZE};

	if (flags & SPEL_CANVAS_DEPTH)
	{
		spel_gfx_texture_desc depth_desc = {
			.type = SPEL_GFX_TEXTURE_2D,
			.depth = 1,
			.mip_count = 1,
			.width = width,
			.height = height,
			.format = SPEL_GFX_TEXTURE_FMT_D24S8,
			.usage = SPEL_GFX_TEXTURE_USAGE_RENDER,
		};

		canvas->depth = spel_gfx_texture_create(gfx, &depth_desc);
		fb_desc.depth = (spel_gfx_attachment){.texture = canvas->depth,
											  .type = SPEL_GFX_ATTACHMENT_DEPTH_STENCIL};
	}

	canvas->framebuffer = spel_gfx_framebuffer_create(gfx, &fb_desc);

	sprintf(canvas->name, "Canvas #%d", canvas->ctx->canvas_count);

	spel_gfx_render_pass_desc pass_desc = {
		.name = canvas->name,
		.framebuffer = canvas->framebuffer,
		.clear_colors = {spel_color_black},
		.color_load = {SPEL_GFX_LOAD_DONT_CARE},
		.color_store = {SPEL_GFX_STORE_STORE},
		.depth_load = SPEL_GFX_LOAD_DONT_CARE,
		.depth_store = flags & SPEL_CANVAS_DEPTH ?: SPEL_GFX_STORE_DONT_CARE,
	};

	canvas->pass = spel_gfx_render_pass_create(gfx, &pass_desc);
	canvas->ctx->canvas_count++;

	return canvas;
}

spel_api void spel_canvas_destroy(spel_canvas canvas)
{
	spel_gfx_texture_destroy(canvas->color);
	if (canvas->depth != NULL)
	{
		spel_gfx_texture_destroy(canvas->depth);
	}
	spel_gfx_framebuffer_destroy(canvas->framebuffer);
	spel_gfx_render_pass_destroy(canvas->pass);
	spel_memory_free(canvas);
}

spel_api void spel_canvas_begin(spel_canvas canvas)
{
	if (canvas == NULL)
	{
		canvas = spel.gfx->canvas_ctx->default_canvas;
	}

	canvas->ctx->active = canvas;
	spel_gfx_cmd_begin_pass(canvas->ctx->command_list, canvas->pass);
}

spel_api void spel_canvas_end(spel_canvas canvas)
{
	if (canvas == NULL)
	{
		canvas = spel.gfx->canvas_ctx->default_canvas;
	}

	spel_assert(canvas->ctx->active != NULL, "canvas_end called without canvas_begin");

	spel_gfx_cmd_end_pass(canvas->ctx->command_list);
	canvas->ctx->active = NULL;
}

void spel_canvas_clear(spel_color color)
{
	spel_gfx_cmd_clear(spel.gfx->canvas_ctx->command_list, color);
}

spel_hidden void spel_canvas_ctx_create(spel_gfx_context gfx)
{
	spel_canvas_context* ctx = gfx->canvas_ctx =
		spel_memory_malloc(sizeof(*gfx->canvas_ctx), SPEL_MEM_TAG_GFX);

	ctx->command_list = spel_gfx_cmdlist_create(gfx);

	ctx->default_canvas =
		spel_memory_malloc(sizeof(*ctx->default_canvas), SPEL_MEM_TAG_GFX);

	ctx->default_canvas->color = NULL;
	ctx->default_canvas->depth = NULL;

	ctx->default_canvas->is_default = true;
	ctx->default_canvas->pass = spel_gfx_render_pass_default(gfx);
	ctx->default_canvas->size = spel_vec2(spel.window.width, spel.window.height);
	ctx->default_canvas->flags = SPEL_CANVAS_AUTO_RESIZE;
	ctx->default_canvas->ctx = ctx;

	spel_gfx_buffer_desc vbuffer_desc;
	vbuffer_desc.type = SPEL_GFX_BUFFER_VERTEX;
	vbuffer_desc.usage = SPEL_GFX_USAGE_DYNAMIC;
	vbuffer_desc.access = SPEL_GFX_BUFFER_DRAW;
	vbuffer_desc.data = NULL;
	vbuffer_desc.size = SPEL_CANVAS_VBUFFER_SIZE;

	spel_gfx_buffer_desc ibuffer_desc;
	ibuffer_desc.type = SPEL_GFX_BUFFER_INDEX;
	ibuffer_desc.usage = SPEL_GFX_USAGE_DYNAMIC;
	ibuffer_desc.access = SPEL_GFX_BUFFER_DRAW;
	ibuffer_desc.data = NULL;
	ibuffer_desc.size = SPEL_CANVAS_VBUFFER_SIZE * 3 / 2;

	ctx->vbo = spel_gfx_buffer_create(gfx, &vbuffer_desc);
	ctx->ibo = spel_gfx_buffer_create(gfx, &ibuffer_desc);

	ctx->verts = spel_memory_malloc(SPEL_CANVAS_VBUFFER_SIZE, SPEL_MEM_TAG_GFX);
	ctx->indices = spel_memory_malloc(SPEL_CANVAS_VBUFFER_SIZE * 3 / 2, SPEL_MEM_TAG_GFX);

	ctx->vert_count = 0;
	ctx->index_count = 0;

	spel_gfx_pipeline_desc pipeline_desc = spel_gfx_pipeline_default_2d(gfx);
	
	ctx->pipeline = spel_gfx_pipeline_create(gfx, &pipeline_desc);
	ctx->white_texture = spel_gfx_texture_white_get(gfx);
}

spel_hidden void spel_canvas_ctx_destroy(spel_canvas_context* ctx)
{
	spel_memory_free(ctx->verts);
	spel_memory_free(ctx->indices);
	
	spel_gfx_buffer_destroy(ctx->vbo);
	spel_gfx_buffer_destroy(ctx->ibo);
	spel_memory_free(ctx->default_canvas);
	spel_gfx_cmdlist_destroy(ctx->command_list);
	spel_memory_free(ctx);
}

spel_hidden void spel_canvas_ctx_flush(spel_canvas_context* ctx)
{
	if (ctx == NULL)
	{
		return;
	}

	if (ctx->vert_count == 0)
	{
		return; // nothing to draw
	}

	// upload scratch buffers to gpu
	spel_gfx_buffer_update(ctx->vbo, ctx->verts,
						   ctx->vert_count * sizeof(spel_canvas_vertex), 0);
	spel_gfx_buffer_update(ctx->ibo, ctx->indices, ctx->index_count * sizeof(uint32_t),
						   0);

	// bind everything
	spel_gfx_cmd_bind_pipeline(ctx->command_list, ctx->pipeline);
	spel_gfx_cmd_bind_shader_buffer(ctx->command_list, ctx->ubuffer_frame);
	spel_gfx_cmd_bind_vertex(ctx->command_list, 0, ctx->vbo, 0);
	spel_gfx_cmd_bind_index(ctx->command_list, ctx->ibo, SPEL_GFX_INDEX_U32, 0);
	spel_gfx_cmd_bind_texture(ctx->command_list, 0, ctx->batch_texture);

	spel_gfx_cmd_draw_indexed(ctx->command_list, ctx->index_count, 0, 0);

	// reset scratch
	ctx->vert_count = 0;
	ctx->index_count = 0;

	spel_gfx_cmdlist_submit(ctx->command_list);
}

void spel_canvas_draw_rect(spel_rect rect)
{
	spel_canvas_context* ctx = spel.gfx->canvas_ctx;

	spel_mat3 t = ctx->transforms[ctx->transform_top];
	int base = ctx->vert_count;

	spel_vec2 tl = spel_mat3_transform_point(t, spel_vec2(rect.x, rect.y));
	spel_vec2 tr = spel_mat3_transform_point(t, spel_vec2(rect.x + rect.width, rect.y));
	spel_vec2 br = spel_mat3_transform_point(
		t, spel_vec2(rect.x + rect.width, rect.y + rect.height));
	spel_vec2 bl = spel_mat3_transform_point(t, spel_vec2(rect.x, rect.y + rect.height));

	ctx->verts[base + 0] = (spel_canvas_vertex){tl, {0, 0}, ctx->color};
	ctx->verts[base + 1] = (spel_canvas_vertex){tr, {1, 0}, ctx->color};
	ctx->verts[base + 2] = (spel_canvas_vertex){br, {1, 1}, ctx->color};
	ctx->verts[base + 3] = (spel_canvas_vertex){bl, {0, 1}, ctx->color};

	ctx->indices[ctx->index_count + 0] = base + 0;
	ctx->indices[ctx->index_count + 1] = base + 1;
	ctx->indices[ctx->index_count + 2] = base + 2;
	ctx->indices[ctx->index_count + 3] = base + 0;
	ctx->indices[ctx->index_count + 4] = base + 2;
	ctx->indices[ctx->index_count + 5] = base + 3;

	ctx->vert_count += 4;
	ctx->index_count += 6;
}
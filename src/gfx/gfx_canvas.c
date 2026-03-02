#include "gfx/gfx_canvas.h"
#include "core/log.h"
#include "core/memory.h"
#include "core/types.h"
#include "gfx/canvas/canvas_internal.h"
#include "gfx/gfx_cmdlist.h"
#include "gfx/gfx_internal.h"
#include "gfx/gfx_pipeline.h"
#include "gfx/gfx_types.h"
#include <stdio.h>

#define SPEL_CANVAS_VBUFFER_SIZE 16384

spel_api spel_canvas spel_canvas_create(spel_gfx_context gfx, int width, int height,
										uint8_t flags)
{
	spel_canvas canvas = spel_memory_malloc(sizeof(*canvas), SPEL_MEM_TAG_GFX);

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
		.color_load = {SPEL_GFX_LOAD_LOAD},
		.color_store = {SPEL_GFX_STORE_STORE},
		.depth_load = SPEL_GFX_LOAD_LOAD,
		.depth_store =
			flags & SPEL_CANVAS_DEPTH ? SPEL_GFX_STORE_STORE : SPEL_GFX_STORE_DONT_CARE,
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
	if (spel.gfx->canvas_ctx == NULL)
	{
		spel_canvas_ctx_create(spel.gfx);
	}

	if (canvas == NULL)
	{
		canvas = spel.gfx->canvas_ctx->default_canvas;
		canvas->size.x = spel.gfx->fb_width;
		canvas->size.y = spel.gfx->fb_height;
	}

	canvas->ctx->active = canvas;
	spel_gfx_cmd_begin_pass(canvas->ctx->command_list, canvas->pass);
	canvas->ctx->frame_data.proj =
		spel_mat4_ortho(0, canvas->size.x, canvas->size.y, 0, -1, 1);

	canvas->ctx->transform_top = 0;
	canvas->ctx->transforms[0] = spel_mat3_identity();

	canvas->ctx->pipeline_desc = spel_gfx_pipeline_default_2d(canvas->ctx->ctx);
	canvas->ctx->pipeline_desc.cull_mode = SPEL_GFX_CULL_NONE;
	canvas->ctx->pipeline = canvas->ctx->og_pipeline;
	canvas->ctx->pipeline_dirty = false;
	canvas->ctx->color = spel_color_white;

	canvas->ctx->fill_paint.type = SPEL_CANVAS_PAINT_COLOR;
	canvas->ctx->fill_paint.color = spel_color_white;

	canvas->ctx->stroke_paint.type = SPEL_CANVAS_PAINT_COLOR;
	canvas->ctx->stroke_paint.color = spel_color_white;
}

spel_api void spel_canvas_end()
{
	spel_assert(spel.gfx->canvas_ctx->active != NULL,
				"canvas_end called without canvas_begin");

	spel_canvas_ctx_flush(spel.gfx->canvas_ctx);
	spel_gfx_cmd_end_pass(spel.gfx->canvas_ctx->command_list);
	spel.gfx->canvas_ctx->active = NULL;
}

void spel_canvas_clear(spel_color color)
{
	spel_gfx_cmd_clear(spel.gfx->canvas_ctx->command_list, color);
}

spel_hidden void spel_canvas_ctx_create(spel_gfx_context gfx)
{
	spel_canvas_context* ctx = gfx->canvas_ctx =
		spel_memory_malloc(sizeof(*gfx->canvas_ctx), SPEL_MEM_TAG_GFX);

	ctx->ctx = gfx;
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

	ctx->transforms[0] = spel_mat3_identity();
	ctx->transform_top = 0;

	ctx->vert_cap = SPEL_CANVAS_VBUFFER_SIZE;
	ctx->index_cap = SPEL_CANVAS_VBUFFER_SIZE * 3 / 2;

	ctx->vbo = spel_gfx_buffer_create(gfx, &vbuffer_desc);
	ctx->ibo = spel_gfx_buffer_create(gfx, &ibuffer_desc);

	ctx->verts = spel_memory_malloc(SPEL_CANVAS_VBUFFER_SIZE, SPEL_MEM_TAG_GFX);
	ctx->indices = spel_memory_malloc(SPEL_CANVAS_VBUFFER_SIZE * 3 / 2, SPEL_MEM_TAG_GFX);

	ctx->vert_count = 0;
	ctx->index_count = 0;

	ctx->color = spel_color_white;

	spel_gfx_pipeline_desc pipeline_desc = spel_gfx_pipeline_default_2d(gfx);

	pipeline_desc.cull_mode = SPEL_GFX_CULL_NONE;

	ctx->og_pipeline = spel_gfx_pipeline_create(gfx, &pipeline_desc);
	ctx->pipeline = ctx->og_pipeline;
	ctx->white_texture = spel_gfx_texture_white_get(gfx);

	ctx->ubuffer_frame = spel_gfx_uniform_buffer_create(ctx->pipeline, "FrameData");
	ctx->pipeline_dirty = false;

	ctx->sampler_desc = spel_gfx_sampler_default();
	ctx->sampler = spel_gfx_sampler_get(ctx->ctx, &ctx->sampler_desc);
	ctx->sampler_dirty = false;

	ctx->line_width = 5.0F;
	ctx->fill_mode = SPEL_CANVAS_FILL;

	ctx->current_path = (spel_canvas_path){0};
	ctx->current_path.closed = false;
	ctx->current_path.cmd_capacity = 256;
	ctx->current_path.point_capacity = 256;
	ctx->miter_limit = 4;

	ctx->current_path.cmds =
		spel_memory_malloc(ctx->current_path.cmd_capacity, SPEL_MEM_TAG_GFX);

	ctx->current_path.points =
		spel_memory_malloc(ctx->current_path.point_capacity, SPEL_MEM_TAG_GFX);
}

spel_hidden void spel_canvas_ctx_destroy(spel_canvas_context* ctx)
{
	spel_memory_free(ctx->current_path.cmds);
	spel_memory_free(ctx->current_path.points);

	spel_memory_free(ctx->verts);
	spel_memory_free(ctx->indices);

	spel_gfx_uniform_buffer_destroy(ctx->ubuffer_frame);
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
	spel_gfx_cmd_buffer_update(ctx->command_list, ctx->vbo, ctx->verts,
							   ctx->vert_count * sizeof(spel_canvas_vertex), 0);
	spel_gfx_cmd_buffer_update(ctx->command_list, ctx->ibo, ctx->indices,
							   ctx->index_count * sizeof(uint32_t), 0);

	// bind everything
	spel_gfx_cmd_bind_pipeline(ctx->command_list, ctx->pipeline);

	spel_gfx_cmd_uniform_block_update(ctx->command_list, ctx->ubuffer_frame,
									  &ctx->frame_data, sizeof(ctx->frame_data), 0);
	spel_gfx_cmd_bind_shader_buffer(ctx->command_list, ctx->ubuffer_frame);

	spel_gfx_cmd_bind_vertex(ctx->command_list, 0, ctx->vbo, 0);
	spel_gfx_cmd_bind_index(ctx->command_list, ctx->ibo, SPEL_GFX_INDEX_U32, 0);
	spel_gfx_cmd_bind_texture(ctx->command_list, 0, ctx->batch_texture);
	spel_gfx_cmd_bind_sampler(ctx->command_list, 0, ctx->sampler);

	spel_gfx_cmd_draw_indexed(ctx->command_list, ctx->index_count, 0, 0);

	// reset scratch
	ctx->vert_count = 0;
	ctx->index_count = 0;
}

spel_hidden void spel_canvas_check_batch(spel_gfx_texture texture, spel_canvas_context* ctx)
{
	if (texture != ctx->batch_texture || ctx->pipeline_dirty || ctx->sampler_dirty)
	{
		spel_canvas_ctx_flush(ctx);

		ctx->batch_texture = texture;

		if (ctx->pipeline_dirty)
		{

			ctx->pipeline = spel_gfx_pipeline_create(ctx->ctx, &ctx->pipeline_desc);
			ctx->pipeline_dirty = false;
		}

		if (ctx->sampler_dirty)
		{
			ctx->sampler = spel_gfx_sampler_get(ctx->ctx, &ctx->sampler_desc);
			ctx->sampler_dirty = false;
		}
	}
}

void spel_canvas_color_set(spel_color color)
{
	spel.gfx->canvas_ctx->color = color;
	spel.gfx->canvas_ctx->stroke_color = color;
}

void spel_canvas_stroke_color_set(spel_color color)
{
	spel.gfx->canvas_ctx->stroke_color = color;
}

void spel_canvas_fill_color_set(spel_color color)
{
	spel.gfx->canvas_ctx->color = color;
}

void spel_canvas_line_width_set(float width)
{
	spel.gfx->canvas_ctx->line_width = width;
}

void spel_canvas_translate(spel_vec2 position)
{
	spel_mat3 t = spel_mat3_identity();
	// column major, translation is at [6] and [7]
	t.m[6] = position.x;
	t.m[7] = position.y;
	spel.gfx->canvas_ctx->transforms[spel.gfx->canvas_ctx->transform_top] = spel_mat3_mul(
		spel.gfx->canvas_ctx->transforms[spel.gfx->canvas_ctx->transform_top], t);
}

void spel_canvas_scale(spel_vec2 scale)
{
	spel_mat3 t = spel_mat3_identity();
	t.m[0] = scale.x;
	t.m[4] = scale.y;
	spel.gfx->canvas_ctx->transforms[spel.gfx->canvas_ctx->transform_top] = spel_mat3_mul(
		spel.gfx->canvas_ctx->transforms[spel.gfx->canvas_ctx->transform_top], t);
}

void spel_canvas_rotate(float degrees)
{
	float radians = spel_math_deg2rad(degrees);

	spel_mat3 t = spel_mat3_identity();
	t.m[0] = cosf(radians);
	t.m[1] = sinf(radians);
	t.m[3] = -sinf(radians);
	t.m[4] = cosf(radians);
	spel.gfx->canvas_ctx->transforms[spel.gfx->canvas_ctx->transform_top] = spel_mat3_mul(
		spel.gfx->canvas_ctx->transforms[spel.gfx->canvas_ctx->transform_top], t);
}

void spel_canvas_shader_set(spel_gfx_shader shader)
{
	if (shader != NULL)
	{
		spel.gfx->canvas_ctx->pipeline_desc.fragment_shader = shader;
	}
	else
	{
		spel.gfx->canvas_ctx->pipeline_desc.fragment_shader = spel.gfx->shaders[1];
	}

	spel.gfx->canvas_ctx->pipeline_dirty = true;
}

void spel_canvas_push()
{
	spel_assert(spel.gfx->canvas_ctx->transform_top < 31, "transform stack overflow");
	spel.gfx->canvas_ctx->transforms[spel.gfx->canvas_ctx->transform_top + 1] =
		spel.gfx->canvas_ctx->transforms[spel.gfx->canvas_ctx->transform_top];
	spel.gfx->canvas_ctx->transform_top++;
	// also snapshot the rest of the draw state
	spel.gfx->canvas_ctx->states[spel.gfx->canvas_ctx->state_top++] =
		spel_canvas_snapshot_state(spel.gfx->canvas_ctx);
}

void spel_canvas_pop()
{
	spel_assert(spel.gfx->canvas_ctx->transform_top > 0, "transform stack underflow");
	spel.gfx->canvas_ctx->transform_top--;
	spel.gfx->canvas_ctx->state_top--;
	spel_canvas_state_restore(
		spel.gfx->canvas_ctx,
		spel.gfx->canvas_ctx->states[spel.gfx->canvas_ctx->state_top]);
}

spel_canvas_state spel_canvas_snapshot_state(spel_canvas_context* ctx)
{
	return (spel_canvas_state){.color = ctx->color,
							   .pipeline_desc = ctx->pipeline_desc,
							   .transform = ctx->transforms[ctx->transform_top],
							   .sampler_desc = ctx->sampler_desc,
							   .line_width = ctx->line_width,
							   .fill_mode = ctx->fill_mode,
							   .stroke_color = ctx->stroke_color,
							   .miter_limit = ctx->miter_limit};
}

void spel_canvas_state_restore(spel_canvas_context* ctx, spel_canvas_state s)
{
	ctx->color = s.color;
	ctx->pipeline_desc = s.pipeline_desc;
	ctx->transforms[ctx->transform_top] = s.transform;
	ctx->sampler_desc = s.sampler_desc;
	ctx->line_width = s.line_width;
	ctx->fill_mode = s.fill_mode;
	ctx->stroke_color = s.stroke_color;
	ctx->miter_limit = s.miter_limit;

	ctx->pipeline_dirty = true;
	ctx->sampler_dirty = true;
}

void spel_canvas_ensure_capacity(int vertsNeeded, int indicesNeeded)
{
	spel_assert(spel.gfx->canvas_ctx->vert_count + vertsNeeded <=
					spel.gfx->canvas_ctx->vert_cap,
				"canvas vertex buffer overflow");
	spel_assert(spel.gfx->canvas_ctx->index_count + indicesNeeded <=
					spel.gfx->canvas_ctx->index_cap,
				"canvas index buffer overflow");
}

void spel_canvas_sampling_set(spel_gfx_sampler_filter filter)
{
	spel.gfx->canvas_ctx->sampler_desc.min = filter;
	spel.gfx->canvas_ctx->sampler_desc.mag = filter;
}

void spel_canvas_draw_line(spel_vec2 start, spel_vec2 end)
{
	spel_canvas_context* ctx = spel.gfx->canvas_ctx;

	spel_canvas_check_batch(ctx->white_texture, ctx);
	spel_canvas_ensure_capacity(4, 6);

	float dx = end.x - start.x;
	float dy = end.y - start.y;
	float len = sqrtf((dx * dx) + (dy * dy));
	if (len < 0.0001F)
	{
		return; // degenerate line
	}

	// perpendicular normal
	float nx = (-dy / len) * (ctx->line_width * 0.5F);
	float ny = (dx / len) * (ctx->line_width * 0.5F);

	spel_mat3 t = ctx->transforms[ctx->transform_top];
	int base = ctx->vert_count;

	ctx->verts[base + 0] = (spel_canvas_vertex){
		spel_mat3_transform_point(t, spel_vec2(start.x + nx, start.y + ny)),
		{0, 0},
		ctx->color};
	ctx->verts[base + 1] = (spel_canvas_vertex){
		spel_mat3_transform_point(t, spel_vec2(end.x + nx, end.y + ny)),
		{1, 0},
		ctx->color};
	ctx->verts[base + 2] = (spel_canvas_vertex){
		spel_mat3_transform_point(t, spel_vec2(end.x - nx, end.y - ny)),
		{1, 1},
		ctx->color};
	ctx->verts[base + 3] = (spel_canvas_vertex){
		spel_mat3_transform_point(t, spel_vec2(start.x - nx, start.y - ny)),
		{0, 1},
		ctx->color};

	ctx->indices[ctx->index_count + 0] = base + 0;
	ctx->indices[ctx->index_count + 1] = base + 1;
	ctx->indices[ctx->index_count + 2] = base + 2;
	ctx->indices[ctx->index_count + 3] = base + 0;
	ctx->indices[ctx->index_count + 4] = base + 2;
	ctx->indices[ctx->index_count + 5] = base + 3;

	ctx->vert_count += 4;
	ctx->index_count += 6;
}

void spel_canvas_draw_rect(spel_rect rect)
{
	spel_canvas_context* ctx = spel.gfx->canvas_ctx;

	spel_canvas_check_batch(ctx->white_texture, ctx);
	spel_canvas_ensure_capacity(4, 6);

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

void spel_canvas_draw_image(spel_gfx_texture tex, spel_rect dst)
{
	spel_canvas_context* ctx = spel.gfx->canvas_ctx;

	spel_canvas_check_batch(tex, ctx);
	spel_canvas_ensure_capacity(4, 6);

	spel_mat3 t = ctx->transforms[ctx->transform_top];
	int base = ctx->vert_count;

	spel_vec2 tl = spel_mat3_transform_point(t, spel_vec2(dst.x, dst.y));
	spel_vec2 tr = spel_mat3_transform_point(t, spel_vec2(dst.x + dst.width, dst.y));
	spel_vec2 br =
		spel_mat3_transform_point(t, spel_vec2(dst.x + dst.width, dst.y + dst.height));
	spel_vec2 bl = spel_mat3_transform_point(t, spel_vec2(dst.x, dst.y + dst.height));

	ctx->verts[base + 0] = (spel_canvas_vertex){tl, {0, 0}, ctx->color};
	ctx->verts[base + 1] = (spel_canvas_vertex){tr, {1, 0}, ctx->color};
	ctx->verts[base + 2] = (spel_canvas_vertex){br, {1, 1}, ctx->color};
	ctx->verts[base + 3] = (spel_canvas_vertex){bl, {0, 1}, ctx->color};

	// same indices as draw_rect
	ctx->indices[ctx->index_count + 0] = base + 0;
	ctx->indices[ctx->index_count + 1] = base + 1;
	ctx->indices[ctx->index_count + 2] = base + 2;
	ctx->indices[ctx->index_count + 3] = base + 0;
	ctx->indices[ctx->index_count + 4] = base + 2;
	ctx->indices[ctx->index_count + 5] = base + 3;

	ctx->vert_count += 4;
	ctx->index_count += 6;
}

void spel_canvas_draw_image_region(spel_gfx_texture tex, spel_rect src, spel_rect dst)
{
	spel_canvas_context* ctx = spel.gfx->canvas_ctx;

	spel_canvas_check_batch(tex, ctx);
	spel_canvas_ensure_capacity(4, 6);

	float tex_w = spel_gfx_texture_size(tex).x;
	float tex_h = spel_gfx_texture_size(tex).y;

	float u0 = src.x / tex_w;
	float v0 = src.y / tex_h;
	float u1 = (src.x + src.width) / tex_w;
	float v1 = (src.y + src.height) / tex_h;

	spel_mat3 t = ctx->transforms[ctx->transform_top];
	int base = ctx->vert_count;

	spel_vec2 tl = spel_mat3_transform_point(t, spel_vec2(dst.x, dst.y));
	spel_vec2 tr = spel_mat3_transform_point(t, spel_vec2(dst.x + dst.width, dst.y));
	spel_vec2 br =
		spel_mat3_transform_point(t, spel_vec2(dst.x + dst.width, dst.y + dst.height));
	spel_vec2 bl = spel_mat3_transform_point(t, spel_vec2(dst.x, dst.y + dst.height));

	ctx->verts[base + 0] = (spel_canvas_vertex){tl, {u0, v0}, ctx->color};
	ctx->verts[base + 1] = (spel_canvas_vertex){tr, {u1, v0}, ctx->color};
	ctx->verts[base + 2] = (spel_canvas_vertex){br, {u1, v1}, ctx->color};
	ctx->verts[base + 3] = (spel_canvas_vertex){bl, {u0, v1}, ctx->color};

	ctx->indices[ctx->index_count + 0] = base + 0;
	ctx->indices[ctx->index_count + 1] = base + 1;
	ctx->indices[ctx->index_count + 2] = base + 2;
	ctx->indices[ctx->index_count + 3] = base + 0;
	ctx->indices[ctx->index_count + 4] = base + 2;
	ctx->indices[ctx->index_count + 5] = base + 3;

	ctx->vert_count += 4;
	ctx->index_count += 6;
}

void spel_canvas_draw_circle(spel_vec2 center, float radius)
{
	spel_canvas_context* ctx = spel.gfx->canvas_ctx;

	int segments = (int)(radius);
	if (segments < 8)
	{
		segments = 8;
	}
	if (segments > 64)
	{
		segments = 64;
	}
	spel_canvas_check_batch(ctx->white_texture, ctx);
	spel_canvas_ensure_capacity(segments + 1, segments * 3);

	spel_mat3 t = ctx->transforms[ctx->transform_top];
	int base = ctx->vert_count;

	ctx->verts[base] = (spel_canvas_vertex){
		spel_mat3_transform_point(t, center), {0.5F, 0.5F}, ctx->color};

	for (int i = 0; i < segments; i++)
	{
		float angle = (float)i / (float)segments * 2.0F * spel_pi;
		float x = center.x + (cosf(angle) * radius);
		float y = center.y + (sinf(angle) * radius);

		ctx->verts[base + 1 + i] = (spel_canvas_vertex){
			spel_mat3_transform_point(t, spel_vec2(x, y)),
			{0.5F + (cosf(angle) * 0.5F), 0.5F + (sinf(angle) * 0.5F)},
			ctx->color};
	}

	for (int i = 0; i < segments; i++)
	{
		ctx->indices[ctx->index_count + (i * 3) + 0] = base;		 // center
		ctx->indices[ctx->index_count + (i * 3) + 1] = base + 1 + i; // current
		ctx->indices[ctx->index_count + (i * 3) + 2] =
			base + 1 + ((i + 1) % segments); // next
	}

	ctx->vert_count += segments + 1;
	ctx->index_count += segments * 3;
}
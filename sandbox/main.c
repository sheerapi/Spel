#include "core/entry.h"
#include "core/log.h"
#include "core/memory.h"
#include "core/panic.h"
#include "gfx/gfx.h"
#include "utils/display.h"
#include <stdio.h>
#include <string.h>
#include <time.h>

spel_gfx_buffer vbuffer;
spel_gfx_buffer ibuffer;
spel_gfx_uniform_buffer ubuffer;
spel_gfx_pipeline pipeline;
spel_gfx_pipeline fullscreen_pipeline;

spel_gfx_uniform position_handle;
spel_gfx_uniform color_handle;

spel_gfx_texture offscreen_color;
spel_gfx_texture offscreen_depth;
spel_gfx_framebuffer offscreen_fb;
spel_gfx_render_pass offscreen_pass;
spel_gfx_render_pass backbuffer_pass;

void spel_conf()
{
	spel.window.resizable = false;
}

void spel_load()
{
	spel_gfx_pipeline_desc pipeline_desc = spel_gfx_pipeline_default_2d(spel.gfx);
	pipeline = spel_gfx_pipeline_create(spel.gfx, &pipeline_desc);

	spel_gfx_pipeline_desc fullscreen_pipeline_desc = spel_gfx_pipeline_fullscreen(
		spel.gfx, spel_gfx_shader_load(spel.gfx, "spel_internal_blit.frag.spv"));
	fullscreen_pipeline = spel_gfx_pipeline_create(spel.gfx, &fullscreen_pipeline_desc);

	typedef struct
	{
		float pos[2];
		float uv[2];
		spel_color color;
	} Vertex;

	Vertex vertices[] = {
		{{0.5F, 0.5F}, {1.0F, 1.0F}, spel_color_white()},
		{{0.5F, -0.5F}, {1.0F, 0.0F}, spel_color_white()},
		{{-0.5F, -0.5F}, {0.0F, 0.0F}, spel_color_white()},
		{{-0.5F, 0.5F}, {0.0F, 1.0F}, spel_color_white()},
	};

	unsigned int indices[] = {3, 2, 1, 3, 1, 0};

	spel_gfx_buffer_desc vbuffer_desc;
	vbuffer_desc.type = SPEL_GFX_BUFFER_VERTEX;
	vbuffer_desc.usage = SPEL_GFX_USAGE_STATIC;
	vbuffer_desc.access = SPEL_GFX_BUFFER_DRAW;
	vbuffer_desc.data = vertices;
	vbuffer_desc.size = sizeof(vertices);

	spel_gfx_buffer_desc ibuffer_desc;
	ibuffer_desc.type = SPEL_GFX_BUFFER_INDEX;
	ibuffer_desc.usage = SPEL_GFX_USAGE_STATIC;
	ibuffer_desc.access = SPEL_GFX_BUFFER_DRAW;
	ibuffer_desc.data = indices;
	ibuffer_desc.size = sizeof(indices);

	vbuffer = spel_gfx_buffer_create(spel.gfx, &vbuffer_desc);
	ibuffer = spel_gfx_buffer_create(spel.gfx, &ibuffer_desc);

	position_handle = spel_gfx_uniform_get(pipeline, "position");
	color_handle = spel_gfx_uniform_get(pipeline, "color");

	ubuffer = spel_gfx_uniform_buffer_create(pipeline, "FrameData");

	spel_gfx_texture_desc color_desc = {
		.type = SPEL_GFX_TEXTURE_2D,
		.depth = 1,
		.mip_count = 1,
		.width = spel.window.width,
		.height = spel.window.height,
		.format = SPEL_GFX_TEXTURE_FMT_RGBA8_UNORM,
		.usage = SPEL_GFX_TEXTURE_USAGE_RENDER | SPEL_GFX_TEXTURE_USAGE_SAMPLED,
	};
	spel_gfx_texture_desc depth_desc = {
		.type = SPEL_GFX_TEXTURE_2D,
		.depth = 1,
		.mip_count = 1,
		.width = spel.window.width,
		.height = spel.window.height,
		.format = SPEL_GFX_TEXTURE_FMT_D16,
		.usage = SPEL_GFX_TEXTURE_USAGE_RENDER,
	};

	offscreen_color = spel_gfx_texture_create(spel.gfx, &color_desc);
	offscreen_depth = spel_gfx_texture_create(spel.gfx, &depth_desc);

	spel_gfx_framebuffer_desc fb_desc = {
		.color[0] = {.texture = offscreen_color, .type = SPEL_GFX_ATTACHMENT_COLOR},
		.depth = {.texture = offscreen_depth, .type = SPEL_GFX_ATTACHMENT_DEPTH},
		.color_count = 1,
		.width = spel.window.width,
		.height = spel.window.height,
	};
	offscreen_fb = spel_gfx_framebuffer_create(spel.gfx, &fb_desc);

	spel_gfx_render_pass_desc offscreen_pass_desc = {
		.name = "G-Buffer",
		.framebuffer = offscreen_fb,
		.color_load = {SPEL_GFX_LOAD_CLEAR},
		.color_store = {SPEL_GFX_STORE_STORE},
		.clear_colors = {spel_color_cyan()},
		.depth_load = SPEL_GFX_LOAD_CLEAR,
		.depth_store = SPEL_GFX_STORE_DONT_CARE,
		.clear_depth = 1.0F,
	};

	spel_gfx_render_pass_desc backbuffer_pass_desc = {
		.name = "Back Buffer",
		.framebuffer = NULL,
		.color_load = {SPEL_GFX_LOAD_DONT_CARE},
		.color_store = {SPEL_GFX_STORE_STORE},
		.depth_load = SPEL_GFX_LOAD_DONT_CARE,
		.depth_store = SPEL_GFX_STORE_DONT_CARE,
	};

	offscreen_pass = spel_gfx_render_pass_create(spel.gfx, &offscreen_pass_desc);
	backbuffer_pass = spel_gfx_render_pass_create(spel.gfx, &backbuffer_pass_desc);

	spel_memory_dump_terminal();
}

float color_data[8] = {1, 0, 0, 1, 0, 1, 0, 1};

spel_vec2 position;

void spel_draw()
{
	spel_gfx_cmdlist cl = spel_gfx_cmdlist_default(spel.gfx);

	// first pass: rendering
	spel_gfx_cmd_begin_pass(cl, offscreen_pass);

	spel_gfx_cmd_bind_pipeline(cl, pipeline);
	position.y += spel.time.delta;

	spel_gfx_cmd_uniform_update(cl, ubuffer, position_handle, &position,
								sizeof(position));
	spel_gfx_cmd_uniform_update(cl, ubuffer, color_handle, &color_data,
								sizeof(color_data));

	spel_gfx_cmd_bind_shader_buffer(cl, ubuffer);
	spel_gfx_cmd_bind_vertex(cl, 0, vbuffer, 0);
	spel_gfx_cmd_bind_index(cl, ibuffer, SPEL_GFX_INDEX_U32, 0);
	spel_gfx_cmd_bind_texture(cl, 0, spel_gfx_texture_checker_get(spel.gfx));
	spel_gfx_cmd_draw_indexed(cl, 6, 0, 0);

	spel_gfx_cmd_end_pass(cl);

	// second pass: offscreen -> backbuffer
	spel_gfx_cmd_begin_pass(cl, backbuffer_pass);

	spel_gfx_cmd_bind_pipeline(cl, fullscreen_pipeline);
	spel_gfx_cmd_bind_texture(cl, 0, offscreen_color);
	spel_gfx_cmd_draw(cl, 3, 0);

	spel_gfx_cmd_end_pass(cl);

	// submit it all
	spel_gfx_cmdlist_submit(cl);
}

void spel_quit()
{
	spel_gfx_pipeline_destroy(pipeline);
	spel_gfx_pipeline_destroy(fullscreen_pipeline);
	spel_gfx_uniform_buffer_destroy(ubuffer);
	spel_gfx_buffer_destroy(vbuffer);
	spel_gfx_buffer_destroy(ibuffer);
}
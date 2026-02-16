#include "core/entry.h"
#include "core/log.h"
#include "core/memory.h"
#include "core/panic.h"
#include "gfx/gfx.h"
#include "gfx/gfx_uniform.h"
#include "utils/display.h"
#include <stdio.h>
#include <string.h>
#include <time.h>

spel_gfx_buffer vbuffer;
spel_gfx_buffer ibuffer;
spel_gfx_uniform_buffer ubuffer;
spel_gfx_pipeline pipeline;

spel_gfx_uniform position_handle;
spel_gfx_uniform color_handle;

void spel_conf()
{
	spel.window.resizable = false;
}

void spel_load()
{
	spel_gfx_pipeline_desc pipeline_desc = spel_gfx_pipeline_default_2d(spel.gfx);
	pipeline = spel_gfx_pipeline_create(spel.gfx, &pipeline_desc);

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
}

float color_data[8] = {
	1, 0, 0, 1,
	0, 1, 0, 1
};

spel_vec2 position;

void spel_draw()
{
	spel_gfx_cmdlist cl = spel_gfx_cmdlist_default(spel.gfx);
	spel_gfx_cmd_bind_pipeline(cl, pipeline);

	spel_gfx_cmd_clear(cl, spel_color_cyan());

	position.y += spel.time.delta;

	spel_gfx_cmd_uniform_update(cl, ubuffer, position_handle, &position, sizeof(position));
	spel_gfx_cmd_uniform_update(cl, ubuffer, color_handle, &color_data, sizeof(color_data));

	spel_gfx_cmd_bind_shader_buffer(cl, position_handle, ubuffer);

	spel_gfx_cmd_bind_vertex(cl, 0, vbuffer, 0);
	spel_gfx_cmd_bind_index(cl, ibuffer, SPEL_GFX_INDEX_U32, 0);
	spel_gfx_cmd_bind_texture(cl, 0, spel_gfx_texture_checker_get(spel.gfx));

	spel_gfx_cmd_draw_indexed(cl, 6, 0, 0);

	spel_gfx_cmdlist_submit(cl);
}

void spel_quit()
{
	spel_gfx_pipeline_destroy(pipeline);
	spel_gfx_uniform_buffer_destroy(ubuffer);
	spel_gfx_buffer_destroy(vbuffer);
	spel_gfx_buffer_destroy(ibuffer);
}
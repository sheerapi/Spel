#include "core/entry.h"
#include "core/log.h"
#include "core/memory.h"
#include "core/window.h"
#include "gfx/gfx.h"
#include "input/input.h"
#include <math.h>

spel_gfx_pipeline pipeline;
spel_gfx_buffer vbuffer;
spel_gfx_buffer ibuffer;
spel_gfx_uniform_buffer ubuffer;

spel_gfx_texture shadow_map;
spel_gfx_pipeline shadow_pipeline;

typedef struct
{
	float pos[3];
	float normal[3];
} Vertex3D;

typedef struct
{
	spel_mat4 model;
	spel_mat4 view;
	spel_mat4 proj;
} FrameData;

FrameData frame_data;

Vertex3D cube_vertices[] = {
	// +X
	{{0.5F, -0.5F, -0.5F}, {1, 0, 0}},
	{{0.5F, 0.5F, -0.5F}, {1, 0, 0}},
	{{0.5F, 0.5F, 0.5F}, {1, 0, 0}},
	{{0.5F, -0.5F, 0.5F}, {1, 0, 0}},

	// -X
	{{-0.5F, -0.5F, 0.5F}, {-1, 0, 0}},
	{{-0.5F, 0.5F, 0.5F}, {-1, 0, 0}},
	{{-0.5F, 0.5F, -0.5F}, {-1, 0, 0}},
	{{-0.5F, -0.5F, -0.5F}, {-1, 0, 0}},

	// +Y
	{{-0.5F, 0.5F, -0.5F}, {0, 1, 0}},
	{{-0.5F, 0.5F, 0.5F}, {0, 1, 0}},
	{{0.5F, 0.5F, 0.5F}, {0, 1, 0}},
	{{0.5F, 0.5F, -0.5F}, {0, 1, 0}},

	// -Y
	{{-0.5F, -0.5F, 0.5F}, {0, -1, 0}},
	{{-0.5F, -0.5F, -0.5F}, {0, -1, 0}},
	{{0.5F, -0.5F, -0.5F}, {0, -1, 0}},
	{{0.5F, -0.5F, 0.5F}, {0, -1, 0}},

	// +Z
	{{-0.5F, -0.5F, 0.5F}, {0, 0, 1}},
	{{0.5F, -0.5F, 0.5F}, {0, 0, 1}},
	{{0.5F, 0.5F, 0.5F}, {0, 0, 1}},
	{{-0.5F, 0.5F, 0.5F}, {0, 0, 1}},

	// -Z
	{{0.5F, -0.5F, -0.5F}, {0, 0, -1}},
	{{-0.5F, -0.5F, -0.5F}, {0, 0, -1}},
	{{-0.5F, 0.5F, -0.5F}, {0, 0, -1}},
	{{0.5F, 0.5F, -0.5F}, {0, 0, -1}},
};

uint32_t cube_indices[] = {0,  1,  2,  0,  2,  3,  4,  5,  6,  4,  6,  7,
						   8,  9,  10, 8,  10, 11, 12, 13, 14, 12, 14, 15,
						   16, 17, 18, 16, 18, 19, 20, 21, 22, 20, 22, 23};

void spel_conf()
{
	spel.window.resizable = false;
	spel.log.severity = SPEL_SEV_DEBUG;
}

void spel_load()
{
	spel_gfx_pipeline_desc pipeline_desc = spel_gfx_pipeline_default();

	static const spel_gfx_vertex_attrib ATTRIBS[] = {
		{0, spel_gfx_vertex_format_float3, 0, 0},  // position
		{1, spel_gfx_vertex_format_float3, 12, 0}}; // normal

	static const spel_gfx_vertex_stream STREAMS[] = {
		{.stride = ((sizeof(float) * 3) + (sizeof(float) * 3)), .rate = SPEL_GFX_VERTEX_RATE_VERTEX}};

	pipeline_desc.vertex_layout.attribs = ATTRIBS;
	pipeline_desc.vertex_layout.attrib_count = 2;
	pipeline_desc.vertex_layout.streams = STREAMS;
	pipeline_desc.vertex_layout.stream_count = 1;

	pipeline_desc.vertex_shader = spel_gfx_shader_load(spel.gfx, "3d_test.vert.spv");
	pipeline_desc.fragment_shader = spel_gfx_shader_load(spel.gfx, "3d_test.frag.spv");

	pipeline_desc.depth_state.depth_test = true;
	pipeline_desc.depth_state.depth_write = true;
	pipeline_desc.depth_state.depth_compare = SPEL_GFX_COMPARE_LESS;
	pipeline_desc.cull_mode = SPEL_GFX_CULL_BACK;
	pipeline_desc.winding = SPEL_GFX_WINDING_COUNTER_CLOCKWISE;

	pipeline = spel_gfx_pipeline_create(spel.gfx, &pipeline_desc);

	spel_gfx_buffer_desc vbuffer_desc;
	vbuffer_desc.type = SPEL_GFX_BUFFER_VERTEX;
	vbuffer_desc.usage = SPEL_GFX_USAGE_STATIC;
	vbuffer_desc.access = SPEL_GFX_BUFFER_DRAW;
	vbuffer_desc.data = cube_vertices;
	vbuffer_desc.size = sizeof(cube_vertices);

	spel_gfx_buffer_desc ibuffer_desc;
	ibuffer_desc.type = SPEL_GFX_BUFFER_INDEX;
	ibuffer_desc.usage = SPEL_GFX_USAGE_STATIC;
	ibuffer_desc.access = SPEL_GFX_BUFFER_DRAW;
	ibuffer_desc.data = cube_indices;
	ibuffer_desc.size = sizeof(cube_indices);

	vbuffer = spel_gfx_buffer_create(spel.gfx, &vbuffer_desc);
	ibuffer = spel_gfx_buffer_create(spel.gfx, &ibuffer_desc);

	ubuffer = spel_gfx_uniform_buffer_create(pipeline, "FrameData");

	spel_memory_dump_terminal();
}

void spel_update(double delta)
{
	if (spel_input_key_pressed(SPEL_KEY_Q))
	{
		spel_window_close();
	}

	spel_vec3 eye = {0.0F, 0.0F, 3.0F};
	spel_vec3 center = {0.0F, 0.0F, 0.0F};
	spel_vec3 up = {0.0F, 1.0F, 0.0F};
}

void spel_draw()
{
	spel_gfx_cmdlist cl = spel_gfx_cmdlist_default(spel.gfx);

	spel_gfx_cmd_bind_pipeline(cl, pipeline);
	spel_gfx_cmd_clear(cl, spel_color_cyan);

	spel_gfx_cmd_uniform_block_update(cl, ubuffer, &frame_data, sizeof(FrameData));
	
	spel_gfx_cmd_bind_shader_buffer(cl, ubuffer);
	spel_gfx_cmd_bind_vertex(cl, 0, vbuffer, 0);
	spel_gfx_cmd_bind_index(cl, ibuffer, SPEL_GFX_INDEX_U32, 0);
	spel_gfx_cmd_bind_texture(cl, 0, spel_gfx_texture_checker_get(spel.gfx));
	spel_gfx_cmd_draw_indexed(cl, 36, 0, 0);

	// submit it all
	spel_gfx_cmdlist_submit(cl);
}

void spel_quit()
{
	spel_gfx_pipeline_destroy(pipeline);
	spel_gfx_uniform_buffer_destroy(ubuffer);
	spel_gfx_buffer_destroy(vbuffer);
	spel_gfx_buffer_destroy(ibuffer);
}
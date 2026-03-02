#include "core/entry.h"
#include "core/log.h"
#include "core/memory.h"
#include "core/window.h"
#include "gfx/canvas/canvas.h"
#include "gfx/canvas/canvas_internal.h"
#include "gfx/gfx.h"
#include "gfx/gfx_internal.h"
#include "input/input.h"
#include <float.h>
#include <math.h>
#include <stdio.h>

spel_gfx_pipeline pipeline;
spel_gfx_buffer vbuffer;
spel_gfx_buffer ibuffer;
spel_gfx_uniform_buffer ubuffer_frame;
spel_gfx_uniform_buffer ubuffer_obj;

spel_gfx_shader vert_shader;
spel_gfx_shader frag_shader;
spel_gfx_shader frag_2d_shader;

// we technically *could* make it one big buffer, but i dont know
// how to do that yet
spel_gfx_buffer vbuffer_plane;
spel_gfx_buffer ibuffer_plane;

spel_gfx_texture ground_texture;
spel_gfx_sampler ground_sampler;

spel_gfx_texture shadow_map;
spel_gfx_pipeline shadow_pipeline;

typedef struct
{
	spel_vec3 pos;
	spel_vec3 normal;
	spel_vec2 uv;
} Vertex3D;

typedef struct
{
	spel_mat4 view;
	spel_mat4 proj;
} FrameData;

typedef struct
{
	spel_mat4 model;
} ObjectData;

FrameData frame_data;
ObjectData cube_data;
ObjectData plane_data;

typedef struct
{
	float yaw;
	float pitch;
	float radius;
} OrbitCamera;

OrbitCamera cam = {0};

Vertex3D cube_vertices[] = {
	// +X
	{{0.5F, -0.5F, -0.5F}, {1, 0, 0}, {0, 0}},
	{{0.5F, 0.5F, -0.5F}, {1, 0, 0}, {1, 0}},
	{{0.5F, 0.5F, 0.5F}, {1, 0, 0}, {1, 1}},
	{{0.5F, -0.5F, 0.5F}, {1, 0, 0}, {0, 1}},

	// -X
	{{-0.5F, -0.5F, 0.5F}, {-1, 0, 0}, {0, 0}},
	{{-0.5F, 0.5F, 0.5F}, {-1, 0, 0}, {1, 0}},
	{{-0.5F, 0.5F, -0.5F}, {-1, 0, 0}, {1, 1}},
	{{-0.5F, -0.5F, -0.5F}, {-1, 0, 0}, {0, 1}},

	// +Y
	{{-0.5F, 0.5F, -0.5F}, {0, 1, 0}, {0, 0}},
	{{-0.5F, 0.5F, 0.5F}, {0, 1, 0}, {0, 1}},
	{{0.5F, 0.5F, 0.5F}, {0, 1, 0}, {1, 1}},
	{{0.5F, 0.5F, -0.5F}, {0, 1, 0}, {1, 0}},

	// -Y
	{{-0.5F, -0.5F, 0.5F}, {0, -1, 0}, {0, 0}},
	{{-0.5F, -0.5F, -0.5F}, {0, -1, 0}, {0, 1}},
	{{0.5F, -0.5F, -0.5F}, {0, -1, 0}, {1, 1}},
	{{0.5F, -0.5F, 0.5F}, {0, -1, 0}, {1, 0}},

	// +Z
	{{-0.5F, -0.5F, 0.5F}, {0, 0, 1}, {0, 0}},
	{{0.5F, -0.5F, 0.5F}, {0, 0, 1}, {1, 0}},
	{{0.5F, 0.5F, 0.5F}, {0, 0, 1}, {1, 1}},
	{{-0.5F, 0.5F, 0.5F}, {0, 0, 1}, {0, 1}},

	// -Z
	{{0.5F, -0.5F, -0.5F}, {0, 0, -1}, {0, 0}},
	{{-0.5F, -0.5F, -0.5F}, {0, 0, -1}, {1, 0}},
	{{-0.5F, 0.5F, -0.5F}, {0, 0, -1}, {1, 1}},
	{{0.5F, 0.5F, -0.5F}, {0, 0, -1}, {0, 1}},
};

Vertex3D plane_vertices[] = {
	{{-10, -0.5F, -10}, {0, 1, 0}, {0, 0}},
	{{10, -0.5F, -10}, {0, 1, 0}, {10, 0}},
	{{10, -0.5F, 10}, {0, 1, 0}, {10, 10}},
	{{-10, -0.5F, 10}, {0, 1, 0}, {0, 10}},
};

uint32_t cube_indices[] = {0,  1,  2,  0,  2,  3,  4,  5,  6,  4,  6,  7,
						   8,  9,  10, 8,  10, 11, 12, 13, 14, 12, 14, 15,
						   16, 17, 18, 16, 18, 19, 20, 21, 22, 20, 22, 23};

uint32_t plane_indices[] = {0, 2, 1, 0, 3, 2};

void spel_conf()
{
	spel.window.resizable = false;
	spel.window.swapchain.msaa = 2;
	spel.log.severity = SPEL_SEV_DEBUG;
}

void spel_load()
{
	spel_gfx_pipeline_desc pipeline_desc = spel_gfx_pipeline_default();

	static const spel_gfx_vertex_attrib ATTRIBS[] = {
		{0, spel_gfx_vertex_format_float3, 0, 0},	// position
		{1, spel_gfx_vertex_format_float3, 12, 0},	// normal
		{2, spel_gfx_vertex_format_float2, 24, 0}}; // iv

	static const spel_gfx_vertex_stream STREAMS[] = {
		{.stride = ((sizeof(float) * 3) + (sizeof(float) * 3) + (sizeof(float) * 2)),
		 .rate = SPEL_GFX_VERTEX_RATE_VERTEX}};

	pipeline_desc.vertex_layout.attribs = ATTRIBS;
	pipeline_desc.vertex_layout.attrib_count = 3;
	pipeline_desc.vertex_layout.streams = STREAMS;
	pipeline_desc.vertex_layout.stream_count = 1;

	vert_shader = spel_gfx_shader_load(spel.gfx, "3d_test.vert.spv");
	frag_shader = spel_gfx_shader_load(spel.gfx, "3d_test.frag.spv");
	frag_2d_shader = spel_gfx_shader_load(spel.gfx, "2d_test.frag.spv");

	pipeline_desc.vertex_shader = vert_shader;
	pipeline_desc.fragment_shader = frag_shader;

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

	vbuffer_desc.data = plane_vertices;
	vbuffer_desc.size = sizeof(plane_vertices);

	ibuffer_desc.data = plane_indices;
	ibuffer_desc.size = sizeof(plane_indices);

	vbuffer_plane = spel_gfx_buffer_create(spel.gfx, &vbuffer_desc);
	ibuffer_plane = spel_gfx_buffer_create(spel.gfx, &ibuffer_desc);

	ubuffer_frame = spel_gfx_uniform_buffer_create(pipeline, "FrameData");
	ubuffer_obj = spel_gfx_uniform_buffer_create(pipeline, "ObjectData");

	ground_texture = spel_gfx_texture_checker_create(spel.gfx, spel_color_hex(0xcccccc),
													 spel_color_hex(0x777777), 64);

	spel_gfx_sampler_desc ground_sampler_desc = spel_gfx_sampler_default();

	ground_sampler = spel_gfx_sampler_get(spel.gfx, &ground_sampler_desc);

	cam.yaw = 0.0F;
	cam.pitch = 0.3F;
	cam.radius = 5.0F;

	plane_data.model = spel_mat4_identity();
}

void spel_update(double delta)
{
	if (spel_input_key_pressed(SPEL_KEY_Q))
	{
		spel_window_close();
	}

	if (spel_input_mouse_button(SPEL_MOUSE_LEFT))
	{
		cam.yaw += -spel_input_mouse_delta().x * 0.05F;
		cam.pitch += spel_input_mouse_delta().y * 0.05F;
	}

	if (cam.pitch > 1.5F)
	{
		cam.pitch = 1.5F;
	}
	if (cam.pitch < -1.5F)
	{
		cam.pitch = -1.5F;
	}

	cam.radius -= spel_input_mouse_wheel().y * 0.5F;

	if (cam.radius < 1.0F)
	{
		cam.radius = 1.0F;
	}
	if (cam.radius > 20.0F)
	{
		cam.radius = 20.0F;
	}

	spel_vec3 eye;

	eye.x = cam.radius * cosf(cam.pitch) * sinf(cam.yaw);
	eye.y = cam.radius * sinf(cam.pitch);
	eye.z = cam.radius * cosf(cam.pitch) * cosf(cam.yaw);

	spel_vec3 center = {0.0F, 0.0F, 0.0F};
	spel_vec3 up = {0.0F, 1.0F, 0.0F};

	frame_data.view = spel_mat4_look_at(eye, center, up);
	frame_data.proj = spel_mat4_perspective(70.0F * (3.14159F / 180.0F),
											(float)spel.window.width / spel.window.height,
											0.1F, 100.0F);

	cube_data.model = spel_mat4_rotate_y(spel_math_deg2rad(spel.time.time * 25.0F));
}

void spel_draw()
{
	spel_gfx_cmdlist cl = spel_gfx_cmdlist_default(spel.gfx);

	spel_gfx_cmd_bind_pipeline(cl, pipeline);
	spel_gfx_cmd_clear(cl, spel_color_sky);

	spel_gfx_cmd_uniform_block_update(cl, ubuffer_frame, &frame_data, sizeof(FrameData),
									  0);
	spel_gfx_cmd_bind_shader_buffer(cl, ubuffer_frame);

	// plane
	spel_gfx_cmd_uniform_block_update(cl, ubuffer_obj, &plane_data, sizeof(ObjectData),
									  0);
	spel_gfx_cmd_bind_shader_buffer(cl, ubuffer_obj);

	spel_gfx_cmd_bind_vertex(cl, 0, vbuffer_plane, 0);
	spel_gfx_cmd_bind_index(cl, ibuffer_plane, SPEL_GFX_INDEX_U32, 0);
	spel_gfx_cmd_bind_image(cl, 0, ground_texture, ground_sampler);
	spel_gfx_cmd_draw_indexed(cl, 6, 0, 0);

	// cube
	spel_gfx_cmd_uniform_block_update(cl, ubuffer_obj, &cube_data, sizeof(ObjectData), 0);
	spel_gfx_cmd_bind_shader_buffer(cl, ubuffer_obj);

	spel_gfx_cmd_bind_vertex(cl, 0, vbuffer, 0);
	spel_gfx_cmd_bind_index(cl, ibuffer, SPEL_GFX_INDEX_U32, 0);
	spel_gfx_cmd_bind_texture(cl, 0, spel_gfx_texture_white_get(spel.gfx));
	spel_gfx_cmd_draw_indexed(cl, 36, 0, 0);

	// submit it all
	spel_gfx_cmdlist_submit(cl);

	spel_canvas_begin(NULL);

	spel_canvas_path_begin();
	// 5-pointed star via inner/outer radius points
	float cx = 400, cy = 300;
	float outer_r = 100, inner_r = 40;

	for (int i = 0; i < 10; i++)
	{
		float angle = (float)i / 10.0f * 2.0f * 3.14159f - 3.14159f * 0.5f;
		float r = (i % 2 == 0) ? outer_r : inner_r;
		float x = cx + cosf(angle) * r;
		float y = cy + sinf(angle) * r;

		if (i == 0)
			spel_canvas_path_moveto(spel_vec2(x, y));
		else
			spel_canvas_path_lineto(spel_vec2(x, y));
	}
	spel_canvas_path_close();

	spel_canvas_path_stroke();

	spel_canvas_path_begin();
	spel_canvas_path_moveto(spel_vec2(100, 100));
	spel_canvas_path_lineto(spel_vec2(200, 100));
	spel_canvas_path_lineto(spel_vec2(200, 150));
	spel_canvas_path_lineto(spel_vec2(150, 150));
	spel_canvas_path_lineto(spel_vec2(150, 300));
	spel_canvas_path_lineto(spel_vec2(100, 300));
	spel_canvas_path_close();
	spel_canvas_path_stroke();

	spel_canvas_path_begin();
	spel_canvas_path_moveto(spel_vec2(400, 400));
	spel_canvas_path_lineto(spel_vec2(500, 500));
	spel_canvas_path_stroke();

	spel_canvas_path_begin();
	spel_canvas_path_moveto(spel_vec2(500, 400));
	spel_canvas_path_bezierto(spel_vec2(600, 385), spel_vec2(600, 385),
							  spel_vec2(700, 400));
	spel_canvas_path_stroke();

	spel_canvas_end();
}

void spel_quit()
{
	spel_memory_dump_terminal();
	
	spel_gfx_shader_destroy(frag_2d_shader);
	spel_gfx_shader_destroy(vert_shader);
	spel_gfx_shader_destroy(frag_shader);
	spel_gfx_pipeline_destroy(pipeline);
	spel_gfx_texture_destroy(ground_texture);
	spel_gfx_uniform_buffer_destroy(ubuffer_obj);
	spel_gfx_uniform_buffer_destroy(ubuffer_frame);
	spel_gfx_buffer_destroy(vbuffer_plane);
	spel_gfx_buffer_destroy(ibuffer_plane);
	spel_gfx_buffer_destroy(vbuffer);
	spel_gfx_buffer_destroy(ibuffer);
}

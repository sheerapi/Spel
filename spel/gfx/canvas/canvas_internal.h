#ifndef SPEL_GFX_CANVAS_INTERNAL
#define SPEL_GFX_CANVAS_INTERNAL
#include "../gfx_pipeline.h"
#include "../gfx_texture.h"
#include "../gfx_uniform.h"
#include "canvas_types.h"
#include "utils/math.h"

typedef struct
{
	spel_mat4 proj;
} spel_canvas_frame_data;

typedef struct
{
	spel_color color;
	spel_color stroke_color;
	spel_gfx_pipeline_desc pipeline_desc;
	spel_mat3 transform;
	spel_gfx_sampler_desc sampler_desc;
	float line_width;
	spel_canvas_fill_mode fill_mode;
} spel_canvas_state;

typedef struct
{
	spel_gfx_context ctx;

	uint8_t canvas_count;
	spel_canvas active;
	spel_gfx_cmdlist command_list;

	spel_canvas default_canvas;

	// canvas state
	spel_color color;
	spel_color stroke_color;
	float line_width;
	spel_gfx_pipeline_desc pipeline_desc;
	spel_gfx_sampler_desc sampler_desc;
	bool pipeline_dirty;
	bool sampler_dirty;
	spel_canvas_fill_mode fill_mode;

	// transform stack
	spel_mat3 transforms[32];
	int transform_top;

	spel_canvas_state states[32];
	int state_top;

	// gfx resources
	spel_gfx_buffer vbo;
	spel_gfx_buffer ibo;
	spel_gfx_pipeline pipeline;
	spel_gfx_sampler sampler;
	spel_gfx_uniform_buffer ubuffer_frame;
	spel_gfx_texture white_texture;

	spel_gfx_pipeline og_pipeline;

	// cpu-side scratch
	spel_canvas_vertex* verts;
	uint32_t* indices;
	int vert_count;
	int index_count;

	int vert_cap;
	int index_cap;

	// current batch state
	spel_gfx_texture batch_texture;

	// frame data
	spel_canvas_frame_data frame_data;
} spel_canvas_context;

typedef struct spel_canvas_t
{
	spel_canvas_context* ctx;

	char name[16];
	bool is_default;

	spel_vec2 size;
	uint8_t flags;

	spel_gfx_texture color;
	spel_gfx_texture depth;

	spel_gfx_framebuffer framebuffer;
	spel_gfx_render_pass pass;
} spel_canvas_t;

#endif
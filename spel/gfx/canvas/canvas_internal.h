#ifndef SPEL_GFX_CANVAS_INTERNAL
#define SPEL_GFX_CANVAS_INTERNAL
#include "../gfx_pipeline.h"
#include "../gfx_texture.h"
#include "../gfx_uniform.h"
#include "canvas_types.h"
#include "utils/math.h"

// paths
typedef enum
{
	SPEL_PATH_MOVE_TO,
	SPEL_PATH_LINE_TO,
	SPEL_PATH_BEZIER_TO,
	SPEL_PATH_CLOSE,
} spel_path_cmd_type;

typedef struct
{
	spel_path_cmd_type type;
	uint8_t size;
	union
	{
		spel_vec2 move;

		struct
		{
			spel_vec2 control;
			spel_vec2 pos;
		} quad;

		struct
		{
			spel_vec2 control1;
			spel_vec2 control2;
			spel_vec2 position;
		} bezier;
	};
} spel_path_cmd;

typedef struct
{
	uint16_t flags;
	uint8_t size;
	spel_vec2 position;
	spel_vec2 direction;	   // direction to next point, computed after tessellation
	float len;				   // length to next point
	spel_vec2 miter_direction; // miter direction, computed during stroke expansion
} spel_path_point;

#define spel_point_corner 0x01		// sharp corner, affects join type
#define spel_point_left 0x02		// left turn
#define spel_point_bevel 0x04		// needs bevel join
#define spel_point_inner_bevel 0x08 // needs inner bevel

#define spel_canvas_tess_tol 0.25F
#define spel_canvas_dist_tol 0.01F

typedef struct
{
	uint8_t* cmds;
	uint32_t cmd_offset;
	uint32_t cmd_capacity;
	uint32_t cmd_count;

	uint8_t* points;
	uint32_t point_offset;
	uint32_t point_capacity;
	uint32_t point_count;

	spel_rect bounds;
	spel_vec2 cursor;
	spel_vec2 start;

	bool closed;
} spel_canvas_path;

// paint
typedef enum
{
	SPEL_CANVAS_PAINT_COLOR,
	SPEL_CANVAS_PAINT_GRADIENT,
	SPEL_CANVAS_PAINT_IMAGE,
	SPEL_CANVAS_PAINT_CUSTOM
} spel_canvas_paint_type;

// composite image + gradient / custom shader
typedef struct
{
	spel_canvas_paint_type type;

	union
	{
		struct
		{
			spel_gfx_texture texture;
			spel_vec2 offset;
			spel_vec2 size; // scale
			float angle;	// rotation
		} image;

		struct
		{
			bool linear;
			spel_vec2 start;
			spel_vec2 end;
			float inner_radius; // radial only
			float outer_radius; // radial only
			spel_color inner_color;
			spel_color outer_color;
		} gradient;

		spel_color color;
		spel_canvas_shader shader;
	};
} spel_canvas_paint;

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
	float miter_limit;
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
	float miter_limit;
	spel_gfx_pipeline_desc pipeline_desc;
	spel_gfx_sampler_desc sampler_desc;
	bool pipeline_dirty;
	bool sampler_dirty;
	spel_canvas_fill_mode fill_mode;

	spel_canvas_paint fill_paint;
	spel_canvas_paint stroke_paint;

	spel_canvas_path current_path;

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

// path related stuff
spel_path_cmd* spel_canvas_path_alloc();
void spel_canvas_path_point_add(spel_vec2 position, uint16_t flags);
spel_path_point* spel_canvas_path_point_get(uint32_t index);

void spel_canvas_path_tessellate();
void spel_canvas_path_compute_directions();
void spel_canvas_path_compute_normals();

void spel_canvas_path_tessellate_bezier(spel_vec2 start, spel_vec2 control1,
										spel_vec2 control2, spel_vec2 end, int depth);

void spel_canvas_fill_path(spel_canvas_paint paint);
void spel_canvas_stroke_path(spel_canvas_paint paint, float width);
bool spel_canvas_path_convex();

void spel_canvas_fill_path_convex(spel_canvas_paint paint);
void spel_canvas_fill_path_concave(spel_canvas_paint paint);

spel_hidden void spel_canvas_ctx_create(spel_gfx_context gfx);
spel_hidden void spel_canvas_check_batch(spel_gfx_texture texture,
										 spel_canvas_context* ctx);
spel_hidden spel_canvas_state spel_canvas_snapshot_state(spel_canvas_context* ctx);
spel_hidden void spel_canvas_state_restore(spel_canvas_context* ctx, spel_canvas_state s);
spel_hidden void spel_canvas_ensure_capacity(int vertsNeeded, int indicesNeeded);

#endif
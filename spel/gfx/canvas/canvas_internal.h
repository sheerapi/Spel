#ifndef SPEL_GFX_CANVAS_INTERNAL
#define SPEL_GFX_CANVAS_INTERNAL
#include "../gfx_pipeline.h"
#include "../gfx_texture.h"
#include "../gfx_uniform.h"
#include "canvas_types.h"
#include "utils/math.h"

typedef enum
{
	SPEL_CANVAS_SIMPLE,
	SPEL_CANVAS_PATH,
	SPEL_CANVAS_TEXT
} spel_canvas_mode;

// fonts
#define SPFN_MAGIC 0x4E465053
#define SPFN_VERSION 1

#define SPFN_TYPE_SDF 0
#define SPFN_TYPE_MSDF 1
#define SPFN_TYPE_BITMAP 2
#define SPFN_TYPE_MTSDF 3

#pragma pack(push, 1)
typedef struct
{
	uint32_t magic;
	uint8_t version;
	uint8_t channels;
	uint8_t font_type;
	uint16_t atlas_width;
	uint16_t atlas_height;
	uint16_t glyph_count;
	uint32_t kerning_count;
	uint32_t image_size;
	float em_size;
	float ascender;
	float descender;
	float line_height;
	float sdf_range;
	uint8_t padding[7];
} spel_font_header;

typedef struct
{
	uint32_t codepoint;
	float uv_x, uv_y;
	float uv_w, uv_h;
	float plane_x, plane_y;
	float plane_w, plane_h;
	float advance;
	uint16_t padding[4];
} spel_font_glyph;

typedef struct
{
	uint32_t codepoint_a;
	uint32_t codepoint_b;
	float advance;
	uint8_t padding[4];
} spel_font_kerning;

#pragma pack(pop)

typedef struct spel_font_t
{
	spel_font_header header;
	spel_font_glyph* glyphs;
	spel_font_kerning* kerning;
	spel_gfx_texture atlas;
	bool internal;
	spel_gfx_context ctx;

	int ascii_index[128];
	uint32_t* ext_codepoints;
	int* ext_indices;
	int ext_count;
} spel_font_t;

typedef struct
{
	int mode;

	float sdf_threshold;
} spel_canvas_font_data;

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

	spel_path_point* points;
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
typedef struct spel_canvas_paint_t
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
} spel_canvas_paint_t;

typedef struct
{
	int32_t paint_type;
	float pad0, pad1, pad2;
	spel_vec4 inner_color;
	spel_vec4 outer_color;
	spel_vec2 paint_start;
	spel_vec2 paint_end;
	float radius_inner;
	float radius_outer;
	float pad3, pad4;
} spel_canvas_paint_data;

typedef struct
{
	spel_mat4 proj;
} spel_canvas_frame_data;

typedef struct
{
	spel_color start;
	spel_color end;
	bool vertical;
} spel_canvas_gradient_simple;

typedef struct
{
	spel_canvas_paint_type simple_paint;
	union
	{
		spel_color color;
		spel_canvas_gradient_simple gradient;
	};
	spel_gfx_pipeline_desc pipeline_desc;
	spel_mat3 transform;
	spel_gfx_sampler_desc sampler_desc;
	float line_width;
	float miter_limit;
	spel_canvas_fill_mode fill_mode;
	spel_canvas_join_type join_type;
	spel_canvas_cap_type cap_type;

	spel_canvas_paint fill_paint;
	spel_canvas_paint stroke_paint;

	spel_font font;
	float font_size;
	uint8_t text_align;
} spel_canvas_state;

typedef struct
{
	spel_gfx_context ctx;

	uint8_t canvas_count;
	spel_canvas active;
	spel_gfx_cmdlist command_list;

	spel_canvas_mode mode;
	bool default_shader; // whether the user is using a custom shader or the 2d one

	// default fonts
	spel_font geist;
	spel_font vga; // IBM VGA 8x16

	// scratch buffers
	int* ear_indices;
	bool* ear_active;
	uint32_t ear_cap;

	int* stroke_point_bases;
	bool* stroke_is_double;
	int stroke_scratch_capacity;

	spel_canvas default_canvas;

	// canvas state
	spel_canvas_paint_type simple_paint;
	union
	{
		spel_color color;
		spel_canvas_gradient_simple gradient;
	};

	float line_width;
	float miter_limit;
	spel_gfx_pipeline_desc pipeline_desc;
	spel_gfx_sampler_desc sampler_desc;
	bool pipeline_dirty;
	bool sampler_dirty;
	bool path_mode;
	spel_canvas_fill_mode fill_mode;
	spel_canvas_join_type join_type;
	spel_canvas_cap_type cap_type;
	spel_font font;
	float font_size;
	uint8_t text_align;

	spel_canvas_paint fill_paint;
	spel_canvas_paint stroke_paint;

	spel_canvas_path current_path;

	// transform stack
	spel_mat3 transforms[16];
	int transform_top;

	spel_canvas_state states[16];
	int state_top;

	// gfx resources
	spel_gfx_buffer vbo;
	spel_gfx_buffer ibo;
	spel_gfx_pipeline pipeline;
	spel_gfx_sampler sampler;
	spel_gfx_uniform_buffer ubuffer_frame;
	spel_gfx_texture white_texture;

	spel_canvas_paint_data paint_data;
	spel_gfx_uniform_buffer paint_ubuffer;
	spel_gfx_pipeline paint_pipeline;

	spel_gfx_pipeline og_pipeline;
	spel_gfx_pipeline og_paint_pipeline;

	spel_canvas_font_data font_data;
	spel_gfx_uniform_buffer font_ubuffer;

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

void spel_canvas_fill_path(spel_canvas_paint* paint);
void spel_canvas_stroke_path(spel_canvas_paint* paint, float width);
bool spel_canvas_path_convex();

void spel_canvas_fill_path_convex(spel_canvas_paint* paint);
void spel_canvas_fill_path_concave(spel_canvas_paint* paint);

// path stroking
void spel_canvas_cap_round(spel_path_point* p, float w, spel_color color, bool start);
void spel_canvas_cap_square(spel_path_point* p, float w, spel_color color, bool start);
void spel_canvas_stroke_push_cap_verts(spel_path_point* p, float w, float ox, float oy,
									   spel_color color);
void spel_canvas_stroke_basic(spel_canvas_path* path, float w, spel_color color,
							  int* outPointBases, bool* outIsDouble);
void spel_canvas_join_bevel(spel_path_point* p0, spel_path_point* p1, float w,
							spel_color color);
void spel_canvas_join_miter(spel_path_point* p0, spel_path_point* p1, float w,
							float miterLimit, spel_color color);
void spel_canvas_join_round(spel_path_point* p0, spel_path_point* p1, float w,
							spel_color color);
void spel_canvas_cap_round_connected(spel_path_point* p, float w, spel_color color,
									 bool start, int stripEndpointBase);
void spel_canvas_join_bevel_connected(spel_path_point* p0, spel_path_point* p1, float w,
									  spel_color color, int vbase);
void spel_canvas_join_round_connected(spel_path_point* p0, spel_path_point* p1, float w,
									  spel_color color, int vbase);

spel_hidden void spel_canvas_ctx_create(spel_gfx_context gfx);

// we do whatever work we have to do (e.g: binding a pipeline) for specific modes
// here
spel_hidden void spel_canvas_mode_flush(spel_canvas_mode mode, spel_canvas_context* ctx);

spel_hidden bool spel_canvas_check_batch(spel_gfx_texture texture, spel_canvas_mode mode,
										 spel_canvas_context* ctx);
spel_hidden spel_canvas_state spel_canvas_snapshot_state(spel_canvas_context* ctx);
spel_hidden void spel_canvas_state_restore(spel_canvas_context* ctx, spel_canvas_state s);
spel_hidden void spel_canvas_ensure_capacity(int vertsNeeded, int indicesNeeded);

// fonts
float spel_canvas_font_kerning(spel_font font, uint32_t cpA, uint32_t cpB);
const spel_font_glyph* spel_canvas_font_find_glyph(spel_font font, uint32_t codepoint);
uint32_t spel_canvas_font_utf8_next(const char** str);

#endif

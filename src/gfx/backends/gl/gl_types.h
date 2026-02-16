#ifndef SPEL_GFX_GL_TYPES
#define SPEL_GFX_GL_TYPES
#include "gfx/gfx_framebuffer.h"
#include "gfx/gfx_types.h"
#include "gl.h"

typedef struct
{
	GLuint shader;
} spel_gfx_shader_gl;

typedef struct
{
	GLuint program;
	GLuint vao;

	GLsizei* strides;

	struct
	{
		bool test;
		bool write;
		bool clamp;
		GLenum func;
	} depth_state;

	struct
	{
		bool test;
		GLbitfield write_mask;
		GLbitfield read_mask;
		GLbitfield reference;
		GLenum func;
		GLenum fail_op;
		GLenum depth_op;
		GLenum pass_op;
	} stencil_state;

	struct
	{
		bool enabled;
		GLenum src_rgb;
		GLenum dst_rgb;

		GLenum src_a;
		GLenum dst_a;

		GLenum op_rgb;
		GLenum op_a;
		GLbitfield write_mask;
	} blend_state;

	struct
	{
		GLenum primitives;
		GLenum cull_mode;
		GLenum winding;
	} topology;

	bool scissor_test;
} spel_gfx_pipeline_gl;

typedef struct
{
	GLenum internal_format;
	GLenum external_format;
	GLenum type;

	uint8_t bytes_per_pixel;
	uint8_t is_depth;
	uint8_t is_srgb;

	uint8_t renderable;
	uint8_t storage;
} spel_gfx_gl_format_info;

typedef struct
{
	GLuint buffer;
	void* mirror;
	uint32_t size;
	uint32_t dirty_min;
	uint32_t dirty_max;
} spel_gfx_gl_buffer;

typedef struct
{
	GLenum draw_buffers[SPEL_GFX_MAX_COLOR_ATTACHMENTS];
	uint32_t draw_buffer_count;
} spel_gfx_gl_framebuffer;

static const spel_gfx_gl_format_info GL_FORMATS[SPEL_GFX_TEXTURE_FORMAT_COUNT] = {
	[SPEL_GFX_TEXTURE_FMT_R8_UNORM] = {GL_R8, GL_RED, GL_UNSIGNED_BYTE, 1, 0, 0, 0, 1},

	[SPEL_GFX_TEXTURE_FMT_RG8_UNORM] = {GL_RG8, GL_RG, GL_UNSIGNED_BYTE, 2, 0, 0, 0, 1},

	[SPEL_GFX_TEXTURE_FMT_RGBA8_UNORM] = {GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE, 4, 0, 0, 1,
										  1},

	[SPEL_GFX_TEXTURE_FMT_RGBA8_SRGB] = {GL_SRGB8_ALPHA8, GL_RGBA, GL_UNSIGNED_BYTE, 4, 0,
										 1, 1, 0},

	[SPEL_GFX_TEXTURE_FMT_R16F] = {GL_R16F, GL_RED, GL_HALF_FLOAT, 2, 0, 0, 0, 1},

	[SPEL_GFX_TEXTURE_FMT_RG16F] = {GL_RG16F, GL_RG, GL_HALF_FLOAT, 4, 0, 0, 0, 1},

	[SPEL_GFX_TEXTURE_FMT_RGBA16F] = {GL_RGBA16F, GL_RGBA, GL_HALF_FLOAT, 8, 0, 0, 1, 1},

	[SPEL_GFX_TEXTURE_FMT_R32F] = {GL_R32F, GL_RED, GL_FLOAT, 4, 0, 0, 0, 1},

	[SPEL_GFX_TEXTURE_FMT_RG32F] = {GL_RG32F, GL_RG, GL_FLOAT, 8, 0, 0, 0, 1},

	[SPEL_GFX_TEXTURE_FMT_RGBA32F] = {GL_RGBA32F, GL_RGBA, GL_FLOAT, 16, 0, 0, 1, 1},

	[SPEL_GFX_TEXTURE_FMT_D16] = {GL_DEPTH_COMPONENT16, GL_DEPTH_COMPONENT,
								  GL_UNSIGNED_SHORT, 2, 1, 0, 1, 0},

	[SPEL_GFX_TEXTURE_FMT_D24S8] = {GL_DEPTH24_STENCIL8, GL_DEPTH_STENCIL,
									GL_UNSIGNED_INT_24_8, 4, 1, 0, 1, 0},

	[SPEL_GFX_TEXTURE_FMT_D32F] = {GL_DEPTH_COMPONENT32F, GL_DEPTH_COMPONENT, GL_FLOAT, 4,
								   1, 0, 1, 0},
};

#endif
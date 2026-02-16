#ifndef SPEL_GFX_COMMANDS
#define SPEL_GFX_COMMANDS
#include "gfx/gfx_types.h"
#include "gfx/gfx_uniform.h"
#include "utils/math.h"
#include <stdint.h>

typedef enum
{
	SPEL_GFX_CMD_BIND_VERTEX,
	SPEL_GFX_CMD_BIND_INDEX,
	SPEL_GFX_CMD_BIND_PIPELINE,
	SPEL_GFX_CMD_BIND_TEXTURE,
	SPEL_GFX_CMD_BIND_SAMPLER,
	SPEL_GFX_CMD_BIND_IMAGE,
	SPEL_GFX_CMD_BIND_SHADER_BUFFER,
	SPEL_GFX_CMD_CLEAR,
	SPEL_GFX_CMD_DRAW,
	SPEL_GFX_CMD_DRAW_INDEXED,
	SPEL_GFX_CMD_VIEWPORT,
	SPEL_GFX_CMD_SCISSOR,
	SPEL_GFX_CMD_UNIFORM_UPDATE,
} spel_gfx_cmd_type;

typedef struct spel_gfx_cmd_header
{
	spel_gfx_cmd_type type;
	uint16_t size;
} spel_gfx_cmd_header;

typedef struct spel_gfx_clear_cmd
{
	spel_gfx_cmd_header hdr;
	spel_color color;
} spel_gfx_clear_cmd;

typedef struct spel_gfx_bind_vertex_cmd
{
	spel_gfx_cmd_header hdr;
	spel_gfx_buffer buf;
	size_t offset;
	uint32_t stream;
} spel_gfx_bind_vertex_cmd;

typedef struct spel_gfx_bind_index_cmd
{
	spel_gfx_cmd_header hdr;
	spel_gfx_buffer buf;
	size_t offset;
	spel_gfx_index_type type;
} spel_gfx_bind_index_cmd;

typedef struct spel_gfx_bind_pipeline_cmd
{
	spel_gfx_cmd_header hdr;
	spel_gfx_pipeline pipeline;
} spel_gfx_bind_pipeline_cmd;

typedef struct spel_gfx_draw_cmd
{
	spel_gfx_cmd_header hdr;
	uint32_t vertex_count;
	uint32_t first_vertex;
} spel_gfx_draw_cmd;

typedef struct spel_gfx_draw_indexed_cmd
{
	spel_gfx_cmd_header hdr;
	uint32_t index_count;
	uint32_t first_index;
	int32_t vertex_offset;
} spel_gfx_draw_indexed_cmd;

typedef struct spel_gfx_bind_texture_cmd
{
	spel_gfx_cmd_header hdr;
	uint32_t slot;
	spel_gfx_texture texture;
} spel_gfx_bind_texture_cmd;

typedef struct spel_gfx_bind_sampler_cmd
{
	spel_gfx_cmd_header hdr;
	uint32_t slot;
	spel_gfx_sampler sampler;
} spel_gfx_bind_sampler_cmd;

typedef struct spel_gfx_bind_image_cmd
{
	spel_gfx_cmd_header hdr;
	uint32_t slot;
	spel_gfx_texture texture;
	spel_gfx_sampler sampler;
} spel_gfx_bind_image_cmd;

typedef struct spel_gfx_viewport_cmd
{
	spel_gfx_cmd_header hdr;
	int x;
	int y;
	int width;
	int height;
} spel_gfx_viewport_cmd;

typedef struct spel_gfx_scissor_cmd
{
	spel_gfx_cmd_header hdr;
	int x;
	int y;
	int width;
	int height;
} spel_gfx_scissor_cmd;

typedef struct spel_gfx_bind_shader_buffer_cmd
{
	spel_gfx_cmd_header hdr;
	spel_gfx_buffer buf;
	uint32_t location;
} spel_gfx_bind_shader_buffer_cmd;

typedef struct spel_gfx_uniform_update_cmd
{
	spel_gfx_cmd_header hdr;
	spel_gfx_uniform_buffer buffer;
	spel_gfx_uniform handle;
	const void* data;
	size_t size;
} spel_gfx_uniform_update_cmd;

#endif
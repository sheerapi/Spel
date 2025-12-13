#ifndef SPEL_GFX_COMMANDS
#define SPEL_GFX_COMMANDS
#include "gfx/gfx_types.h"
#include "utils/math.h"
#include <stdint.h>

typedef enum
{
	SPEL_GFX_CMD_BIND_VERTEX,
	SPEL_GFX_CMD_BIND_INDEX,
	SPEL_GFX_CMD_CLEAR
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
} spel_gfx_bind_vertex_cmd;

typedef struct spel_gfx_bind_index_cmd
{
	spel_gfx_cmd_header hdr;
	spel_gfx_buffer buf;
	size_t offset;
	spel_gfx_index_type type;
} spel_gfx_bind_index_cmd;

#endif
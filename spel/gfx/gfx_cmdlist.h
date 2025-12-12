#ifndef SPEL_GFX_CMDLIST
#define SPEL_GFX_CMDLIST
#include "core/macros.h"
#include "gfx/gfx_types.h"
#include <stdint.h>

typedef struct spel_gfx_cmdlist_t
{
	uint8_t* buffer;
	uint64_t offset;
	uint64_t capacity;

	spel_gfx_context ctx;
	void* data;
} spel_gfx_cmdlist_t;

typedef struct spel_gfx_cmd_header
{
	uint16_t type;
	uint16_t size;
} spel_gfx_cmd_header;

sp_api spel_gfx_cmdlist spel_gfx_cmdlist_create(spel_gfx_context ctx);
sp_api void spel_gfx_cmdlist_destroy(spel_gfx_cmdlist cmdlist);

sp_api void spel_gfx_cmdlist_submit(spel_gfx_cmdlist cmdlist);

#define sp_cmdlist_default_size (4 * 1024) // 4 KB

#endif
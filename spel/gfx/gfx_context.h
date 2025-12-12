#ifndef SPEL_GFX_CONTEXT
#define SPEL_GFX_CONTEXT
#include "core/macros.h"
#	include "gfx/gfx_types.h"

typedef struct
{
	spel_gfx_backend backend;
	bool debug;
	int vsync;
} spel_gfx_context_desc;

sp_api spel_gfx_context spel_gfx_context_create(spel_gfx_context_desc* desc);
sp_api void spel_gfx_context_destroy(spel_gfx_context ctx);
sp_api void spel_gfx_context_conf(spel_gfx_backend backend);

sp_api void spel_gfx_frame_begin(spel_gfx_context ctx);
sp_api void spel_gfx_frame_present(spel_gfx_context ctx);

#endif
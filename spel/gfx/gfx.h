#ifndef SPEL_GFX
#define SPEL_GFX
#include "core/macros.h"
#include <stdint.h>

typedef struct spel_gfx_context spel_gfx_context;

typedef enum
{
	GFX_BACKEND_OPENGL,
} gfx_backend_type;

sp_hidden void spel_gfx_context_conf(gfx_backend_type backend);

sp_api spel_gfx_context* spel_gfx_context_create(gfx_backend_type backend);
sp_api void spel_gfx_context_destroy(spel_gfx_context* ctx);

sp_api void spel_gfx_frame_begin(spel_gfx_context* ctx);
sp_api void spel_gfx_frame_end(spel_gfx_context* ctx);

#endif
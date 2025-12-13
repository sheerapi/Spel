#ifndef SPEL_GFX
#define SPEL_GFX

#include "gfx_types.h"
#include "gfx_context.h"

// gfx_cmdlist.h can't be included due to structural concerns
sp_api spel_gfx_cmdlist spel_gfx_cmdlist_create(spel_gfx_context ctx);
sp_api void spel_gfx_cmdlist_destroy(spel_gfx_cmdlist cmdlist);

sp_api void spel_gfx_cmdlist_submit(spel_gfx_cmdlist cmdlist);

#include "gfx_buffer.h"

#endif
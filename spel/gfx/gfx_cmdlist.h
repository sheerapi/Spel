#ifndef SPEL_GFX_CMDLIST
#define SPEL_GFX_CMDLIST
#include "core/macros.h"
#include "gfx/gfx_types.h"
#include "utils/math.h"

sp_api spel_gfx_cmdlist spel_gfx_cmdlist_create(spel_gfx_context ctx);
sp_api void spel_gfx_cmdlist_destroy(spel_gfx_cmdlist cmdlist);

sp_api void spel_gfx_cmdlist_submit(spel_gfx_cmdlist cmdlist);

sp_api void spel_gfx_cmd_bind_vertex(spel_gfx_cmdlist cl, spel_gfx_buffer buf);
sp_api void spel_gfx_cmd_bind_index(spel_gfx_cmdlist cl, spel_gfx_buffer buf);

sp_api void spel_gfx_cmd_clear(spel_gfx_cmdlist cl, spel_color color);

#endif
#include "core/entry.h"
#include "gfx/gfx.h"
#include "gfx/gfx_types.h"

spel_gfx_cmdlist cmdlist;
spel_gfx_buffer vbuffer;
spel_gfx_buffer ibuffer;

void spel_load()
{
	cmdlist = spel_gfx_cmdlist_create(spel.gfx);

	spel_gfx_buffer_desc vbuffer_desc;
	vbuffer_desc.type = SPEL_GFX_BUFFER_VERTEX;
	vbuffer_desc.access = SPEL_GFX_BUFFER_DRAW;
	vbuffer_desc.usage = SPEL_GFX_USAGE_STATIC;
	vbuffer_desc.size = 64;
	vbuffer_desc.data = nullptr;

	vbuffer = spel_gfx_buffer_create(spel.gfx, &vbuffer_desc);
}

void spel_draw()
{
	spel_gfx_cmd_clear(cmdlist, spel_color_cyan());
	spel_gfx_cmdlist_submit(cmdlist);
}

void spel_quit()
{
	spel_gfx_cmdlist_destroy(cmdlist);
	spel_gfx_buffer_destroy(vbuffer);
}
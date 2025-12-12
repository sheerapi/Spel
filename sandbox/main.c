#include "core/entry.h"
#include "gfx/gfx.h"

spel_gfx_cmdlist cmdlist;

void spel_load()
{
	cmdlist = spel_gfx_cmdlist_create(spel.gfx);
}

void spel_draw()
{
	spel_gfx_cmdlist_submit(cmdlist);
}

void spel_quit()
{
	spel_gfx_cmdlist_destroy(cmdlist);
}
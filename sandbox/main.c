#include "core/entry.h"
#include "core/log.h"
#include "core/memory.h"
#include "gfx/gfx.h"
#include "gfx/gfx_types.h"
#include <string.h>

spel_gfx_cmdlist cmdlist;
spel_gfx_buffer vbuffer;
spel_gfx_shader vertex_shader;

double dumpTime = 0;

void spel_load()
{
	cmdlist = spel_gfx_cmdlist_create(spel.gfx);

	vertex_shader =
		spel_gfx_shader_load(spel.gfx, "test.vert.spv", "main", SPEL_GFX_SHADER_VERTEX);
	spel_memory_dump();
}

void spel_draw()
{
	dumpTime += spel.time.delta;

	if (dumpTime >= 5)
	{
		log_info("%f", spel.time.delta);
		spel_memory_dump();
		dumpTime = 0;
	}
	spel_gfx_cmd_clear(cmdlist, spel_color_magenta());

	spel_gfx_cmdlist_submit(cmdlist);
}

void spel_quit()
{
	spel_gfx_shader_destroy(vertex_shader);
	spel_gfx_cmdlist_destroy(cmdlist);
	// spel_gfx_buffer_destroy(vbuffer);
}
#include "core/entry.h"
#include "core/log.h"
#include "core/memory.h"
#include "gfx/gfx.h"
#include "gfx/gfx_types.h"
#include <string.h>

spel_gfx_buffer vbuffer;
spel_gfx_shader vertex_shader;

void spel_conf()
{
	spel.window.resizable = false;
}

void spel_load()
{
	vertex_shader =
		spel_gfx_shader_load(spel.gfx, "test.vert.spv", "main", SPEL_GFX_SHADER_VERTEX);

	
	spel_memory_dump();
}

void spel_draw()
{
	spel_gfx_cmdlist cl = spel_gfx_cmdlist_default(spel.gfx);

	spel_gfx_cmd_clear(cl, spel_color_magenta());

	spel_gfx_cmdlist_submit(cl);
}

void spel_quit()
{
	spel_gfx_shader_destroy(vertex_shader);
	// spel_gfx_buffer_destroy(vbuffer);
}
#include "core/entry.h"
#include "core/log.h"
#include "core/memory.h"
#include "gfx/gfx.h"
#include "gfx/gfx_types.h"
#include <string.h>

spel_gfx_cmdlist cmdlist;
spel_gfx_buffer vbuffer;
spel_gfx_shader vertex_shader;

void spel_load()
{
	cmdlist = spel_gfx_cmdlist_create(spel.gfx);

	char* buffer = 0;
	long length;
	FILE* f = fopen("vert.spv", "rb");

	if (f)
	{
		fseek(f, 0, SEEK_END);
		length = ftell(f);
		fseek(f, 0, SEEK_SET);
		buffer = malloc(length);
		if (buffer)
		{
			fread(buffer, 1, length, f);
		}
		fclose(f);
	}

	spel_gfx_shader_desc shader_desc;
	shader_desc.stage = SPEL_GFX_SHADER_VERTEX;
	shader_desc.entry = "main";
	shader_desc.debug_name = "test_vertex";
	shader_desc.source = buffer;
	shader_desc.source_size = length;
	
	vertex_shader = spel_gfx_shader_create(spel.gfx, &shader_desc);
	spel_memory_dump();
}

void spel_draw()
{
	spel_gfx_cmd_clear(cmdlist, spel_color_magenta());

	spel_gfx_cmdlist_submit(cmdlist);
}

void spel_quit()
{
	spel_gfx_shader_destroy(vertex_shader);
	spel_gfx_cmdlist_destroy(cmdlist);
	// spel_gfx_buffer_destroy(vbuffer);
}
#include "core/entry.h"
#include "core/log.h"
#include "core/memory.h"
#include "gfx/gfx.h"
#include "gfx/gfx_buffer.h"
#include "gfx/gfx_types.h"
#include <string.h>

spel_gfx_buffer vbuffer;
spel_gfx_buffer ibuffer;
spel_gfx_shader vertex_shader;
spel_gfx_shader fragment_shader;
spel_gfx_pipeline pipeline;

void spel_conf()
{
	spel.window.resizable = false;
}

void spel_load()
{
	vertex_shader =
		spel_gfx_shader_load(spel.gfx, "test.vert.spv", "main", SPEL_GFX_SHADER_VERTEX);

	fragment_shader =
		spel_gfx_shader_load(spel.gfx, "test.frag.spv", "main", SPEL_GFX_SHADER_FRAGMENT);

	spel_gfx_pipeline_desc pipeline_desc =
		spel_gfx_pipeline_minimal(vertex_shader, fragment_shader);
	pipeline = spel_gfx_pipeline_create(spel.gfx, &pipeline_desc);

	float vertices[] = {
		0.5f,  0.5f,  0.0f, // top right
		0.5f,  -0.5f, 0.0f, // bottom right
		-0.5f, -0.5f, 0.0f, // bottom left
		-0.5f, 0.5f,
		0.0f // top left
	};

	unsigned int indices[] = {// note that we start from 0!
							  3, 2, 1, 3, 1, 0};

	spel_gfx_buffer_desc vbuffer_desc;
	vbuffer_desc.type = SPEL_GFX_BUFFER_VERTEX;
	vbuffer_desc.usage = SPEL_GFX_USAGE_STATIC;
	vbuffer_desc.access = SPEL_GFX_BUFFER_DRAW;
	vbuffer_desc.data = vertices;
	vbuffer_desc.size = sizeof(vertices);

	spel_gfx_buffer_desc ibuffer_desc;
	ibuffer_desc.type = SPEL_GFX_BUFFER_INDEX;
	ibuffer_desc.usage = SPEL_GFX_USAGE_STATIC;
	ibuffer_desc.access = SPEL_GFX_BUFFER_DRAW;
	ibuffer_desc.data = indices;
	ibuffer_desc.size = sizeof(indices);

	vbuffer = spel_gfx_buffer_create(spel.gfx, &vbuffer_desc);
	ibuffer = spel_gfx_buffer_create(spel.gfx, &ibuffer_desc);

	spel_memory_dump();
}

void spel_draw()
{
	spel_gfx_cmdlist cl = spel_gfx_cmdlist_default(spel.gfx);
	spel_gfx_cmd_clear(cl, spel_color_black());

	spel_gfx_cmd_bind_pipeline(cl, pipeline);
	spel_gfx_cmd_bind_vertex(cl, 0, vbuffer, 0);
	spel_gfx_cmd_bind_index(cl, ibuffer, SPEL_GFX_INDEX_U32, 0);

	spel_gfx_cmd_draw_indexed(cl, 6, 0, 0);

	spel_gfx_cmdlist_submit(cl);
}

void spel_quit()
{
	spel_gfx_pipeline_destroy(pipeline);
	spel_gfx_shader_destroy(fragment_shader);
	spel_gfx_shader_destroy(vertex_shader);
	spel_gfx_buffer_destroy(vbuffer);
}
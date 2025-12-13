#include "core/entry.h"
#include "gfx/gfx_internal.h"
#include "gfx/gfx_types.h"
#include "gl.h"

void exec_cmd_clear(spel_gfx_clear_cmd* cmd);

spel_gfx_cmdlist spel_gfx_cmdlist_create_gl(spel_gfx_context ctx)
{
	spel_gfx_cmdlist cl = (spel_gfx_cmdlist)sp_malloc(sizeof(*cl), SPEL_MEM_TAG_GFX);
	cl->capacity = sp_cmdlist_default_size;
	cl->offset = 0;
	cl->ctx = ctx;
	cl->buffer = sp_malloc(cl->capacity, SPEL_MEM_TAG_GFX);

	return cl;
}

void spel_gfx_cmdlist_destroy_gl(spel_gfx_cmdlist cl)
{
	sp_free(cl->buffer);
	sp_free(cl);
}

void* spel_gfx_cmdlist_alloc_gl(spel_gfx_cmdlist cl, size_t size, size_t align)
{
	uint64_t aligned = (cl->offset + (align - 1)) & ~(align - 1);

	if (aligned + size > cl->capacity)
	{
		cl->capacity *= 2;
		void* new_buffer = sp_realloc(cl->buffer, cl->capacity, SPEL_MEM_TAG_GFX);
		if (new_buffer == NULL)
		{
			spel_error("Out of memory?");
			return NULL;
		}
		cl->buffer = new_buffer;
	}

	void* ptr = cl->buffer + aligned;
	cl->offset = aligned + size;
	return ptr;
}

void spel_gfx_cmdlist_submit_gl(spel_gfx_cmdlist cl)
{
	uint8_t* ptr = cl->buffer;
	while (ptr < cl->buffer + cl->offset)
	{
		spel_gfx_cmd_header* hdr = (spel_gfx_cmd_header*)ptr;

		switch (hdr->type)
		{
		case SPEL_GFX_CMD_CLEAR:
			exec_cmd_clear((spel_gfx_clear_cmd*)ptr);
			break;
		default:
			break;
		}

		ptr += hdr->size;
	}

	cl->offset = 0;
}

void exec_cmd_clear(spel_gfx_clear_cmd* cmd)
{
	glClearColor((float)cmd->color.r / 255, (float)cmd->color.g / 255,
				 (float)cmd->color.b / 255, (float)cmd->color.a / 255);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
}

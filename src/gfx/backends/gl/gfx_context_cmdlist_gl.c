#include "core/entry.h"
#include "gfx/gfx_cmdlist.h"
#include "gfx/gfx_types.h"

spel_gfx_cmdlist spel_gfx_cmdlist_create_gl(spel_gfx_context ctx)
{
	spel_gfx_cmdlist cl = (spel_gfx_cmdlist)malloc(sizeof(*cl));
	cl->capacity = sp_cmdlist_default_size;
	cl->offset = 0;
	cl->ctx = ctx;
	cl->buffer = malloc(cl->capacity);

	return cl;
}

void spel_gfx_cmdlist_destroy_gl(spel_gfx_cmdlist cl)
{
	free(cl->buffer);
	free(cl);
}

void* spel_gfx_cmdlist_alloc_gl(spel_gfx_cmdlist cl, size_t size, size_t align)
{
	uint64_t aligned = (cl->offset + (align - 1)) & ~(align - 1);

	if (aligned + size > cl->capacity)
	{
		cl->capacity *= 2;
		void* new_buffer = realloc(cl->buffer, cl->capacity);
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
}
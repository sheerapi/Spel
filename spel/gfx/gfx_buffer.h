#ifndef SPEL_GFX_BUFFER
#define SPEL_GFX_BUFFER
#include "core/macros.h"
#include "gfx/gfx_types.h"
#include <stddef.h>

typedef struct spel_gfx_buffer_desc
{
	spel_gfx_buffer_type type;
	spel_gfx_buffer_usage usage;
	spel_gfx_buffer_access access;
	size_t size;
	const void* data;
} spel_gfx_buffer_desc;

sp_api spel_gfx_buffer spel_gfx_buffer_create(spel_gfx_context ctx,
											  const spel_gfx_buffer_desc* desc);

sp_api void spel_gfx_buffer_destroy(spel_gfx_buffer buf);
sp_api void spel_gfx_buffer_update(spel_gfx_buffer buf, const void* data, size_t size,
								   size_t offset);

sp_api void* spel_gfx_buffer_map(spel_gfx_buffer buf, size_t offset, size_t size,
								 spel_gfx_access access);
sp_api void spel_gfx_buffer_unmap(spel_gfx_buffer buf);

sp_api void spel_gfx_buffer_flush(spel_gfx_buffer buf, size_t offset, size_t size);

#endif
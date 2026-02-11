#ifndef SPEL_GFX_SHADER
#define SPEL_GFX_SHADER
#include "core/macros.h"
#include "gfx/gfx_types.h"
#include <stddef.h>

typedef struct spel_gfx_shader_desc
{
	void* source;
	size_t source_size;

	const char* debug_name;
} spel_gfx_shader_desc;

sp_api spel_gfx_shader spel_gfx_shader_create(spel_gfx_context ctx,
											spel_gfx_shader_desc* desc);

sp_api void spel_gfx_shader_destroy(spel_gfx_shader shader);

sp_api spel_gfx_shader spel_gfx_shader_load(spel_gfx_context ctx, const char* path);

#endif
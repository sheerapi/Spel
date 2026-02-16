#ifndef SPEL_GFX_UNIFORM
#define SPEL_GFX_UNIFORM
#include "core/macros.h"
#include "gfx/gfx_types.h"
#include <stdint.h>

typedef struct
{
	union
	{
		uint32_t location;
		struct
		{
			uint16_t set;
			uint16_t binding;
		};
	};

	uint32_t offset;
	uint32_t size;
	uint32_t count;
} spel_gfx_uniform;

typedef struct
{
	union
	{
		uint32_t location;
		struct
		{
			uint16_t set;
			uint16_t binding;
		};
	};
	
	spel_gfx_buffer buffer;
	uint32_t size;
} spel_gfx_uniform_buffer;

sp_api spel_gfx_uniform* spel_gfx_uniform_list(spel_gfx_pipeline pipeline,
											   uint32_t* count);

sp_api spel_gfx_uniform spel_gfx_uniform_get(spel_gfx_pipeline pipeline,
											 const char* name);

const sp_api char* spel_gfx_uniform_name(spel_gfx_pipeline pipeline,
										 spel_gfx_uniform handle);

sp_api char** spel_gfx_uniform_block_names(spel_gfx_pipeline pipeline,
												 uint32_t* count);

sp_api spel_gfx_uniform_buffer spel_gfx_uniform_buffer_create(spel_gfx_pipeline pipeline,
															  const char* blockName);

sp_api void spel_gfx_uniform_buffer_destroy(spel_gfx_uniform_buffer buf);

sp_api uint32_t spel_gfx_uniform_block_size(spel_gfx_pipeline pipeline,
											spel_gfx_uniform handle);

#endif
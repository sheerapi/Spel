#ifndef SPEL_GFX_TEXTURE
#define SPEL_GFX_TEXTURE
#include "core/macros.h"
#include "gfx/gfx_types.h"
#include "utils/math.h"

typedef struct
{
	spel_gfx_texture_type type;
	spel_gfx_texture_format format;
	spel_gfx_texture_usage usage;

	uint32_t width;
	uint32_t height;
	uint32_t depth; // 1 for 2D
	uint32_t mip_count;

	void* data;
	size_t data_size;
} spel_gfx_texture_desc;

typedef struct
{
	spel_gfx_sampler_filter min;
	spel_gfx_sampler_filter mag;
	spel_gfx_sampler_mip_filter mip;

	spel_gfx_sampler_wrap wrap_u;
	spel_gfx_sampler_wrap wrap_v;
	spel_gfx_sampler_wrap wrap_w;

	float lod_bias;
	float max_aniso;
} spel_gfx_sampler_desc;

typedef struct
{
	spel_gfx_texture_format format;
	spel_gfx_texture_usage usage;
	uint32_t mip_count;
	bool srgb;
} spel_gfx_texture_load_desc;

sp_api spel_gfx_texture spel_gfx_texture_create(spel_gfx_context ctx,
												const spel_gfx_texture_desc* desc);

sp_api spel_gfx_texture spel_gfx_texture_color_create(spel_gfx_context ctx, spel_color color);

sp_api void spel_gfx_texture_destroy(spel_gfx_texture texture);
sp_api spel_gfx_texture spel_gfx_texture_white_get(spel_gfx_context ctx);
sp_api spel_gfx_texture spel_gfx_texture_checker_get(spel_gfx_context ctx);

sp_api spel_gfx_texture spel_gfx_texture_load(spel_gfx_context ctx, const char* path,
											  const spel_gfx_texture_load_desc* desc);

sp_api spel_gfx_texture spel_gfx_texture_load_color(spel_gfx_context ctx,
													const char* path);
sp_api spel_gfx_texture spel_gfx_texture_load_linear(spel_gfx_context ctx,
													 const char* path);

sp_api spel_gfx_sampler_desc spel_gfx_sampler_default();
sp_api spel_gfx_sampler spel_gfx_sampler_get(spel_gfx_context ctx,
											 const spel_gfx_sampler_desc* desc);
#endif
#ifndef SPEL_GFX_FRAMEBUFFER
#define SPEL_GFX_FRAMEBUFFER
#include "gfx/gfx_types.h"
#include "utils/math.h"

#define SPEL_GFX_MAX_COLOR_ATTACHMENTS 8

typedef struct
{
	spel_gfx_texture texture;
	spel_gfx_attachment_type type;
	uint32_t mip;
	uint32_t layer;
} spel_gfx_attachment;

typedef struct
{
	spel_gfx_attachment color[SPEL_GFX_MAX_COLOR_ATTACHMENTS];
	spel_gfx_attachment depth;
	uint32_t color_count;
	uint32_t width;
	uint32_t height;
} spel_gfx_framebuffer_desc;

typedef struct
{
	const char* name;

	spel_gfx_framebuffer framebuffer;

	spel_gfx_load_op color_load[SPEL_GFX_MAX_COLOR_ATTACHMENTS];
	spel_gfx_store_op color_store[SPEL_GFX_MAX_COLOR_ATTACHMENTS];
	spel_color clear_colors[SPEL_GFX_MAX_COLOR_ATTACHMENTS];

	spel_gfx_load_op depth_load;
	spel_gfx_store_op depth_store;
	float clear_depth;
} spel_gfx_render_pass_desc;

sp_api spel_gfx_framebuffer
spel_gfx_framebuffer_create(spel_gfx_context ctx, const spel_gfx_framebuffer_desc* desc);

sp_api spel_vec2 spel_gfx_framebuffer_size(spel_gfx_framebuffer fb);
sp_api spel_gfx_texture spel_gfx_framebuffer_color(spel_gfx_framebuffer fb,
												   uint32_t index);
sp_api spel_gfx_texture spel_gfx_framebuffer_depth(spel_gfx_framebuffer fb);

sp_api void spel_gfx_framebuffer_blit(spel_gfx_framebuffer src, spel_rect srcRegion,
									  spel_gfx_framebuffer dst, spel_rect dstRegion);

sp_api void spel_gfx_framebuffer_blit_simple(spel_gfx_framebuffer src,
										spel_gfx_framebuffer dst);

sp_api void spel_gfx_framebuffer_blit_mask(spel_gfx_framebuffer src, spel_rect srcRegion,
										   spel_gfx_framebuffer dst, spel_rect dstRegion,
										   uint8_t attachment,
										   spel_gfx_sampler_filter filter);

sp_api void spel_gfx_framebuffer_destroy(spel_gfx_framebuffer fb);

sp_api spel_gfx_render_pass
spel_gfx_render_pass_create(spel_gfx_context ctx, const spel_gfx_render_pass_desc* desc);

sp_api void spel_gfx_render_pass_destroy(spel_gfx_render_pass pass);
sp_api spel_gfx_render_pass spel_gfx_render_pass_default(spel_gfx_context ctx);

#endif
#ifndef SPEL_GFX_CANVAS
#define SPEL_GFX_CANVAS
#include "core/macros.h"
#include "gfx/gfx_types.h"
#include "utils/math.h"

typedef struct spel_gfx_canvas_t* spel_gfx_canvas;
typedef struct spel_gfx_canvas_shader_t* spel_gfx_canvas_shader;

typedef enum
{
	SPEL_GFX_BLEND_ALPHA,	 // standard alpha blending (default)
	SPEL_GFX_BLEND_ADDITIVE, // src + dst (particles, glows)
	SPEL_GFX_BLEND_MULTIPLY, // src * dst
	SPEL_GFX_BLEND_NONE,	 // no blending, overwrite
} spel_gfx_blend_mode;

typedef enum
{
	SPEL_GFX_CANVAS_COLOR = 1 << 0,		 // color attachment (almost always want this)
	SPEL_GFX_CANVAS_DEPTH = 1 << 1,		 // depth attachment (for 3d draws into canvas)
	SPEL_GFX_CANVAS_MSAA = 1 << 2,		 // multisample (resolves on canvas_end)
	SPEL_GFX_CANVAS_AUTO_RESIZE = 1 << 3 // the canvas should match the window size
} spel_gfx_canvas_flags;

spel_api spel_gfx_canvas spel_gfx_canvas_create(int width, int height, uint8_t flags);
spel_api void spel_gfx_canvas_destroy(spel_gfx_canvas canvas);

spel_api spel_gfx_texture spel_gfx_canvas_texture(spel_gfx_canvas canvas);
spel_gfx_texture spel_gfx_canvas_depth_texture(spel_gfx_canvas canvas);

int spel_gfx_canvas_width(spel_gfx_canvas canvas);
int spel_gfx_canvas_height(spel_gfx_canvas canvas);

void spel_gfx_canvas_begin(spel_gfx_canvas canvas);
void spel_gfx_canvas_end(spel_gfx_canvas canvas);

void spel_gfx_canvas_clear(spel_color color);

#endif
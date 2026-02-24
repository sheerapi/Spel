#ifndef SPEL_CANVAS
#define SPEL_CANVAS
#include "core/macros.h"
#include "gfx/gfx_types.h"
#include "utils/math.h"

typedef struct spel_canvas_t* spel_canvas;
typedef struct spel_canvas_shader_t* spel_canvas_shader;

typedef struct
{
	spel_vec2 position;
	spel_vec2 uv;
	spel_color color;
} spel_canvas_vertex;

typedef enum
{
	SPEL_GFX_BLEND_ALPHA,	 // standard alpha blending (default)
	SPEL_GFX_BLEND_ADDITIVE, // src + dst (particles, glows)
	SPEL_GFX_BLEND_MULTIPLY, // src * dst
	SPEL_GFX_BLEND_NONE,	 // no blending, overwrite
} spel_gfx_blend_mode;

typedef enum
{
	SPEL_CANVAS_COLOR = 1 << 0,		 // color attachment (almost always want this)
	SPEL_CANVAS_DEPTH = 1 << 1,		 // depth attachment (for 3d draws into canvas)
	SPEL_CANVAS_MSAA = 1 << 2,		 // multisample (resolves on canvas_end)
	SPEL_CANVAS_AUTO_RESIZE = 1 << 3 // the canvas should match the window size
} spel_canvas_flags;

spel_api spel_canvas spel_canvas_create(spel_gfx_context gfx, int width, int height,
										uint8_t flags);
spel_api void spel_canvas_destroy(spel_canvas canvas);

spel_api spel_gfx_texture spel_canvas_texture(spel_canvas canvas);
spel_gfx_texture spel_canvas_depth_texture(spel_canvas canvas);

int spel_canvas_width(spel_canvas canvas);
int spel_canvas_height(spel_canvas canvas);

void spel_canvas_begin(spel_canvas canvas);
void spel_canvas_end();

void spel_canvas_clear(spel_color color);
void spel_canvas_draw_rect(spel_rect rect);
void spel_canvas_color_set(spel_color color);

#endif
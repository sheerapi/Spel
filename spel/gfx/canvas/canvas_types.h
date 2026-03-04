#ifndef SPEL_GFX_CANVAS_TYPES
#define SPEL_GFX_CANVAS_TYPES
#include "utils/math.h"

typedef struct spel_canvas_t* spel_canvas;
typedef struct spel_font_t* spel_font;
typedef struct spel_canvas_shader_t* spel_canvas_shader;
typedef struct spel_canvas_paint_t spel_canvas_paint;

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
	SPEL_CANVAS_FILL,
	SPEL_CANVAS_STROKE,
	SPEL_CANVAS_FILL_AND_STROKE,
} spel_canvas_fill_mode;

typedef enum
{
	SPEL_CANVAS_COLOR = 1 << 0,		 // color attachment (almost always want this)
	SPEL_CANVAS_DEPTH = 1 << 1,		 // depth attachment (for 3d draws into canvas)
	SPEL_CANVAS_MSAA = 1 << 2,		 // multisample (resolves on canvas_end)
	SPEL_CANVAS_AUTO_RESIZE = 1 << 3 // the canvas should match the window size
} spel_canvas_flags;

typedef enum
{
	SPEL_CANVAS_JOIN_MITER,
	SPEL_CANVAS_JOIN_BEVEL,
	SPEL_CANVAS_JOIN_ROUND
} spel_canvas_join_type;

typedef enum
{
	SPEL_CANVAS_CAP_SQUARE,
	SPEL_CANVAS_CAP_BUTT,
	SPEL_CANVAS_CAP_ROUND
} spel_canvas_cap_type;

typedef enum
{
	SPEL_CANVAS_ALIGN_LEFT,
	SPEL_CANVAS_ALIGN_CENTER,
	SPEL_CANVAS_ALIGN_RIGHT,

	SPEL_CANVAS_ALIGN_TOP = 0,
	SPEL_CANVAS_ALIGN_MIDDLE,
	SPEL_CANVAS_ALIGN_BOTTOM
} spel_canvas_text_align;

#endif
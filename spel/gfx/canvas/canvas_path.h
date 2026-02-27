#ifndef SPEL_GFX_CANVAS_PATH
#define SPEL_GFX_CANVAS_PATH
#include "gfx/canvas/canvas_types.h"
#include "utils/math.h"

typedef enum
{
	SPEL_FILL_NONZERO,	// standard, fills everything enclosed (default)
	SPEL_FILL_EVEN_ODD, // alternates fill on overlapping regions
} spel_canvas_fill_rule;

void spel_canvas_path_begin(spel_canvas_path path);
void spel_canvas_path_moveto(spel_vec2 position);
void spel_canvas_path_lineto(spel_vec2 position);
void spel_canvas_path_bezierto(spel_vec2 control1, spel_vec2 control2,
							   spel_vec2 position);
void spel_canvas_path_cubicto(spel_vec2 control, spel_vec2 position);
void spel_canvas_path_arc(spel_vec2 center, float radius, float angleStart,
						  float angleEnd, bool ccw);
void spel_canvas_path_arcto(spel_vec2 start, spel_vec2 end, float radius);
void spel_canvas_path_close();

void spel_canvas_path_rect(spel_rect rect);
void spel_canvas_path_rrect(spel_rect rect, float radius);
void spel_canvas_path_circle(spel_vec2 center, float radius);
void spel_canvas_path_ellipse(spel_vec2 center, spel_vec2 radius);

void spel_canvas_path_fill();
void spel_canvas_path_stroke();
void spel_canvas_path_fill_stroke();

void spel_canvas_fill_rule_set(spel_canvas_fill_rule rule);

#endif
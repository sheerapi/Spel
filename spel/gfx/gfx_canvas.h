#ifndef SPEL_CANVAS
#define SPEL_CANVAS
#include "core/macros.h"
#include "gfx/canvas/canvas_types.h"
#include "gfx/gfx_types.h"
#include "utils/math.h"

// we really should move this to the canvas folder later

// object creation
spel_api spel_canvas spel_canvas_create(spel_gfx_context gfx, int width, int height,
										uint8_t flags);
spel_api void spel_canvas_destroy(spel_canvas canvas);

spel_api spel_font spel_font_create(spel_gfx_context gfx, const uint8_t* data,
									size_t dataSize);
spel_api void spel_font_destroy(spel_font font);

spel_api spel_gfx_texture spel_canvas_texture(spel_canvas canvas);
spel_gfx_texture spel_canvas_depth_texture(spel_canvas canvas);

int spel_canvas_width(spel_canvas canvas);
int spel_canvas_height(spel_canvas canvas);

void spel_canvas_begin(spel_canvas canvas);
void spel_canvas_end();

void spel_canvas_clear(spel_color color);

/// sets both fill and stroke color
/// affects simple and path rendering variants
void spel_canvas_color_set(spel_color color);

/// sets both fill and stroke gradients
/// affects simple and path rendering variants
void spel_canvas_gradient_set(spel_color start, spel_color end, bool vertical);

/// affects simple and path rendering variants
void spel_canvas_fill_color_set(spel_color color);

/// affects simple and path rendering variants
void spel_canvas_stroke_color_set(spel_color color);
void spel_canvas_shader_set(spel_gfx_shader shader);
void spel_canvas_sampling_set(spel_gfx_sampler_filter filter);
void spel_canvas_fill_mode_set(spel_canvas_fill_mode mode);
void spel_canvas_line_width_set(float width);

void spel_canvas_translate(spel_vec2 position);
void spel_canvas_scale(spel_vec2 scale);
void spel_canvas_rotate(float degrees);

void spel_canvas_push();
void spel_canvas_pop();

void spel_canvas_draw_rect(spel_rect rect);
void spel_canvas_draw_image(spel_gfx_texture tex, spel_rect dst);
void spel_canvas_draw_image_region(spel_gfx_texture tex, spel_rect src, spel_rect dst);
void spel_canvas_draw_circle(spel_vec2 center, float radius);
void spel_canvas_draw_line(spel_vec2 start, spel_vec2 end);
void spel_canvas_draw_text(const char* text, spel_vec2 position);
void spel_canvas_draw_text_wrapped(const char* text, spel_vec2 position, float maxWidth);

#endif
// lets make pong
#include "core/log.h"
#include "core/types.h"
#include "gfx/gfx.h"
#include "input/input.h"

static float player_y;
static float enemy_y;
static spel_vec2 ball_pos;
static spel_action axis_vertical;

void spel_load()
{
	// why an axis instead of spel_input_key?
	// axes provide a way to gather input in more than a simple "is key pressed"
	// way, by aggregating input from other sources (e.g: keyboard + gamepad).
	// this way the paddle can move with the w/s keys, the arrow keys, or a gamepad
	// if we desire
	axis_vertical = spel_input_action_create("vertical", SPEL_ACTION_ANALOG);
	spel_input_action_bind_axis(axis_vertical, SPEL_KEY_S, SPEL_KEY_W);

	// middle of the screen
	ball_pos = spel_vec2((float)spel.window.width / 2, (float)spel.window.height / 2);
	player_y = (spel.window.height / 2) - 50;
	enemy_y = (spel.window.height / 2) - 50;
}

void spel_update(double dt)
{
	// 300 px/s basically
	// we subtract instead of adding because, in screen space, y increases
	// as we move down. confusing, i know
	player_y -= (float)(300 * spel_input_action_value(axis_vertical) * dt);
}

void spel_draw()
{
	spel_canvas_begin(NULL);

	// court line
	// white but kinda transparent
	spel_canvas_color_set(spel_color_hexa(0xFFFFFF55));
	spel_canvas_draw_rect(spel_rect((spel.window.width / 2) - 2.5F, 0, 5, spel.window.height));

	spel_canvas_color_set(spel_color_white);
	spel_canvas_draw_rect(spel_rect(50, enemy_y, 25, 100));

	// why "spel.window.width - 75"?
	// so we can assure it always renders in the same spot (right side).
	// we subtract 75 because we want it to be 50px off the right side,
	// and we have to account for the paddle being 25px wide
	spel_canvas_draw_rect(spel_rect(spel.window.width - 75, player_y, 25, 100));

	spel_canvas_draw_circle(ball_pos, 10);

	spel_canvas_draw_text_wrapped("Hello World! This is Spël, a game framework made in C inspired by LÖVE2D", spel_vec2(100, 200), 100);

	spel_canvas_end();
}
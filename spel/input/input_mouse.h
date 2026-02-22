#ifndef SPEL_INPUT_MOUSE
#define SPEL_INPUT_MOUSE
#include "utils/math.h"

typedef enum
{
	SPEL_MOUSE_LEFT = 0,
	SPEL_MOUSE_MIDDLE,
	SPEL_MOUSE_RIGHT,
	SPEL_MOUSE_X1,
	SPEL_MOUSE_X2,
	SPEL_MOUSE_BUTTON_COUNT
} spel_mouse_button;

spel_api spel_vec2 spel_input_mouse_pos();
spel_api spel_vec2 spel_input_mouse_delta();
spel_api spel_vec2 spel_input_mouse_wheel();

spel_api bool spel_input_mouse_button(spel_mouse_button btn);
spel_api bool spel_input_mouse_pressed(spel_mouse_button btn);
spel_api bool spel_input_mouse_released(spel_mouse_button btn);

spel_api void spel_input_mouse_set_visible(bool visible);
spel_api void spel_input_mouse_set_locked(bool locked);

#endif
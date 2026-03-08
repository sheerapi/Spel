#ifndef SPEL_INPUT_INTERNAL
#define SPEL_INPUT_INTERNAL
#include "input_action.h"
#include "input_gamepad.h"
#include "input_keyboard.h"
#include "input_mouse.h"
#include "utils/math.h"
#include <stdbool.h>

#define SPEL_MAX_CONTROLLERS 16
#define SPEL_MAX_CONTROLLER_FINGERS 2
#define SPEL_MAX_BINDINGS 8

#define SPEL_KEY_WORDS ((SPEL_KEY_COUNT + 63) / 64)

#define SPEL_KEY_WORD(k) ((k) >> 6)
#define SPEL_KEY_MASK(k) (1ull << ((k) & 63))

typedef struct SDL_Gamepad SDL_Gamepad;
typedef union SDL_Event SDL_Event;

typedef struct
{
	bool active;
	spel_vec2 pos;
	float pressure;
} spel_touchpad_finger;

typedef struct spel_gamepad_t
{
	bool connected;
	SDL_Gamepad* sdl_gamepad;
	uint32_t buttons;
	uint32_t buttons_prev;
	float axes[SPEL_GAMEPAD_AXIS_COUNT];

	bool gyro_enabled;
	bool accel_enabled;
	spel_vec3 gyro;
	spel_vec3 accel;

	bool touchpad_enabled;
	spel_touchpad_finger touchpad[SPEL_MAX_CONTROLLER_FINGERS];
} spel_gamepad_t;

typedef enum
{
	SPEL_BINDING_NONE = 0,
	SPEL_BINDING_KEY,
	SPEL_BINDING_KEY_AXIS,
	SPEL_BINDING_MOUSE_BUTTON,
	SPEL_BINDING_GAMEPAD_BUTTON,
	SPEL_BINDING_GAMEPAD_AXIS,
} spel_binding_type;

typedef struct
{
	spel_binding_type type;
	union
	{
		spel_key key;

		struct
		{
			spel_key negative;
			spel_key positive;
		} key_axis;

		spel_mouse_button mouse_button;

		struct
		{
			spel_gamepad_id gamepad;
			spel_gamepad_button button;
		} gamepad_button;

		struct
		{
			spel_gamepad_id gamepad;
			spel_gamepad_axis axis;
		} gamepad_axis;
	};
} spel_action_binding;

typedef struct spel_action_t
{
	char name[48];
	bool type; // SPEL_ACTION_DIGITAL or SPEL_ACTION_ANALOG
	uint8_t binding_count;
	spel_action_binding bindings[SPEL_MAX_BINDINGS];
} spel_action_t;

typedef struct spel_input_t
{
	uint64_t keys[SPEL_KEY_WORDS];
	uint64_t keys_prev[SPEL_KEY_WORDS];

	spel_vec2 mouse_pos;
	spel_vec2 mouse_delta;
	spel_vec2 mouse_wheel;

	bool mouse_buttons[SPEL_MOUSE_BUTTON_COUNT];
	bool mouse_buttons_prev[SPEL_MOUSE_BUTTON_COUNT];

	spel_gamepad gamepads[SPEL_MAX_CONTROLLERS];

	spel_action actions;
	uint32_t action_count;
	uint32_t action_capacity;

	char text_input[256];
	uint32_t text_input_len;
	bool text_input_enabled;

	bool gamepad_initialized;
} spel_input_t;

spel_hidden void spel_input_init();
spel_hidden void spel_input_shutdown();

spel_hidden void spel_input_process_event(SDL_Event* event);
spel_hidden void spel_input_ensure_gamepad();

#endif
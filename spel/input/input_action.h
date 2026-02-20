#ifndef SPEL_INPUT_ACTION
#define SPEL_INPUT_ACTION
#include "input_gamepad.h"
#include "input_keyboard.h"
#include "input_mouse.h"

typedef struct spel_action_t* spel_action;

typedef enum
{
	SPEL_ACTION_DIGITAL, // button-like, on/off
	SPEL_ACTION_ANALOG,	 // axis-like, -1 to 1
} spel_action_type;

spel_action spel_input_action_create(const char* name, spel_action_type type);
void spel_input_action_destroy(spel_action action);

void spel_input_action_bind_key(spel_action action, spel_key key);
void spel_input_action_bind_axis(spel_action action, spel_key neg, spel_key pos);
void spel_input_action_bind_gamepad_axis(spel_action action, spel_gamepad_id pad,
										 spel_gamepad_axis axis);

void spel_input_action_bind_mouse_button(spel_action action, spel_mouse_button btn);
void spel_input_action_bind_gamepad_button(spel_action action, spel_gamepad_id pad,
										   spel_gamepad_button btn);

void spel_input_action_unbind_all(spel_action action);

bool spel_input_action(spel_action action);			 // digital: pressed, analog: non-zero
bool spel_input_action_pressed(spel_action action);	 // digital only
bool spel_input_action_released(spel_action action); // digital only
float spel_input_action_value(spel_action action);	 // analog: -1 to 1, digital: 0 or 1

#endif
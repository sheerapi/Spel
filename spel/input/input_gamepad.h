#ifndef SPEL_GAMEPAD
#define SPEL_GAMEPAD
#include "utils/math.h"
#include <stdbool.h>
#include <stdint.h>

typedef struct spel_gamepad_t* spel_gamepad;
typedef uint8_t spel_gamepad_id;

typedef enum
{
	SPEL_GAMEPAD_BUTTON_SOUTH, // A / Cross
	SPEL_GAMEPAD_BUTTON_EAST,  // B / Circle
	SPEL_GAMEPAD_BUTTON_WEST,  // X / Square
	SPEL_GAMEPAD_BUTTON_NORTH, // Y / Triangle
	SPEL_GAMEPAD_BUTTON_BACK,  // Select / Share
	SPEL_GAMEPAD_BUTTON_GUIDE, // Xbox / PS button
	SPEL_GAMEPAD_BUTTON_START,
	SPEL_GAMEPAD_BUTTON_L_STICK,
	SPEL_GAMEPAD_BUTTON_R_STICK,
	SPEL_GAMEPAD_BUTTON_L_SHOULDER,
	SPEL_GAMEPAD_BUTTON_R_SHOULDER,
	SPEL_GAMEPAD_BUTTON_DPAD_UP,
	SPEL_GAMEPAD_BUTTON_DPAD_DOWN,
	SPEL_GAMEPAD_BUTTON_DPAD_LEFT,
	SPEL_GAMEPAD_BUTTON_DPAD_RIGHT,
	SPEL_GAMEPAD_BUTTON_MISC1, /**< Additional button (e.g. Xbox Series X share button,
								 PS5 microphone button, Nintendo Switch Pro capture
								 button, Amazon Luna microphone button, Google Stadia
								 capture button) */
	SPEL_GAMEPAD_BUTTON_RIGHT_PADDLE1, /**< Upper or primary paddle, under your right hand
										 (e.g. Xbox Elite paddle P1, DualSense Edge RB
										 button, Right Joy-Con SR button) */
	SPEL_GAMEPAD_BUTTON_LEFT_PADDLE1,  /**< Upper or primary paddle, under your left hand
										 (e.g. Xbox Elite paddle P3, DualSense Edge LB
										 button, Left Joy-Con SL button) */
	SPEL_GAMEPAD_BUTTON_RIGHT_PADDLE2, /**< Lower or secondary paddle, under your right
										 hand (e.g. Xbox Elite paddle P2, DualSense Edge
										 right Fn button, Right Joy-Con SL button) */
	SPEL_GAMEPAD_BUTTON_LEFT_PADDLE2, /**< Lower or secondary paddle, under your left hand
										(e.g. Xbox Elite paddle P4, DualSense Edge left Fn
										button, Left Joy-Con SR button) */
	SPEL_GAMEPAD_BUTTON_TOUCHPAD,	  /**< PS4/PS5 touchpad button */
	SPEL_GAMEPAD_BUTTON_MISC2,		  /**< Additional button */
	SPEL_GAMEPAD_BUTTON_MISC3, /**< Additional button (e.g. Nintendo GameCube left trigger
								 click) */
	SPEL_GAMEPAD_BUTTON_MISC4, /**< Additional button (e.g. Nintendo GameCube right
								 trigger click) */
	SPEL_GAMEPAD_BUTTON_MISC5, /**< Additional button */
	SPEL_GAMEPAD_BUTTON_MISC6,
	SPEL_GAMEPAD_BUTTON_COUNT
} spel_gamepad_button;

typedef enum
{
	SPEL_GAMEPAD_AXIS_LEFT_X,
	SPEL_GAMEPAD_AXIS_LEFT_Y,
	SPEL_GAMEPAD_AXIS_RIGHT_X,
	SPEL_GAMEPAD_AXIS_RIGHT_Y,
	SPEL_GAMEPAD_AXIS_LEFT_TRIGGER,
	SPEL_GAMEPAD_AXIS_RIGHT_TRIGGER,
	SPEL_GAMEPAD_AXIS_COUNT
} spel_gamepad_axis;

spel_api bool spel_input_gamepad_connected(spel_gamepad_id id);
spel_api bool spel_input_gamepad_button(spel_gamepad_id id, spel_gamepad_button btn);
spel_api bool spel_input_gamepad_button_pressed(spel_gamepad_id id, spel_gamepad_button btn);
spel_api float spel_input_gamepad_axis(spel_gamepad_id id, spel_gamepad_axis axis);

spel_api spel_vec2 spel_input_gamepad_left_stick(spel_gamepad_id id);
spel_api spel_vec2 spel_input_gamepad_right_stick(spel_gamepad_id id);

spel_api float spel_input_gamepad_axis_deadzone(spel_gamepad_id id, spel_gamepad_axis axis,
											   float deadzone);

spel_api spel_vec2 spel_input_gamepad_left_stick_deadzone(spel_gamepad_id id, float deadzone);
spel_api spel_vec2 spel_input_gamepad_right_stick_deadzone(spel_gamepad_id id,
														  float deadzone);

spel_api void spel_input_gamepad_rumble(spel_gamepad_id id, float lowFreq, float highFreq,
									   uint32_t durationMs);
spel_api void spel_input_gamepad_rumble_triggers(spel_gamepad_id id, float left,
												float right, uint32_t durationMs);
spel_api void spel_input_gamepad_led(spel_gamepad_id id, spel_color color);

spel_api bool spel_input_gamepad_has_gyro(spel_gamepad_id id);
spel_api bool spel_input_gamepad_has_accel(spel_gamepad_id id);
spel_api spel_vec3 spel_input_gamepad_gyro(spel_gamepad_id id); // rad/s
spel_api spel_vec3 spel_input_gamepad_accel(spel_gamepad_id id); // m/s²

spel_api bool spel_input_gamepad_has_touchpad(spel_gamepad_id id);
spel_api spel_vec2 spel_input_gamepad_touchpad(spel_gamepad_id id, uint8_t finger);
spel_api float spel_input_gamepad_touchpad_pressure(spel_gamepad_id id, uint8_t finger);
spel_api bool spel_input_gamepad_touchpad_active(spel_gamepad_id id, uint8_t finger);

#endif
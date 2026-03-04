#include "input/input.h"
#include "SDL3/SDL_init.h"
#include "SDL3/SDL_keyboard.h"
#include "core/log.h"
#include "core/types.h"
#include "input/input_gamepad.h"
#include "input/input_internal.h"
#include <math.h>
#include <string.h>

spel_gamepad_id spel_find_gamepad_by_id(uint32_t sdlId);

spel_hidden void spel_input_shutdown()
{
	for (size_t i = 0; i < SPEL_MAX_CONTROLLERS; i++)
	{
		spel_memory_free(spel.input->gamepads[i]);
	}

	spel_memory_free(spel.input->actions);
	spel_memory_free(spel.input);
}

spel_api void spel_input_update()
{
	memcpy(spel.input->keys_prev, spel.input->keys, sizeof(spel.input->keys));
	memcpy(spel.input->mouse_buttons_prev, spel.input->mouse_buttons,
		   sizeof(spel.input->mouse_buttons));

	spel.input->mouse_delta = (spel_vec2){0, 0};
	spel.input->mouse_wheel = (spel_vec2){0, 0};
	spel.input->text_input_len = 0;

	for (int i = 0; i < SPEL_MAX_CONTROLLERS; i++)
	{
		spel_gamepad pad = spel.input->gamepads[i];
		if (!pad->connected || !pad->sdl_gamepad)
		{
			continue;
		}

		pad->buttons_prev = pad->buttons;

		pad->axes[SPEL_GAMEPAD_AXIS_LEFT_X] =
			(float)SDL_GetGamepadAxis(pad->sdl_gamepad, SDL_GAMEPAD_AXIS_LEFTX) /
			32767.0F;
		pad->axes[SPEL_GAMEPAD_AXIS_LEFT_Y] =
			-((float)SDL_GetGamepadAxis(pad->sdl_gamepad, SDL_GAMEPAD_AXIS_LEFTY) /
			  32767.0F);
		pad->axes[SPEL_GAMEPAD_AXIS_RIGHT_X] =
			(float)SDL_GetGamepadAxis(pad->sdl_gamepad, SDL_GAMEPAD_AXIS_RIGHTX) /
			32767.0F;
		pad->axes[SPEL_GAMEPAD_AXIS_RIGHT_Y] =
			-((float)SDL_GetGamepadAxis(pad->sdl_gamepad, SDL_GAMEPAD_AXIS_RIGHTY) /
			  32767.0F);
		pad->axes[SPEL_GAMEPAD_AXIS_LEFT_TRIGGER] =
			(float)SDL_GetGamepadAxis(pad->sdl_gamepad, SDL_GAMEPAD_AXIS_LEFT_TRIGGER) /
			32767.0F;
		pad->axes[SPEL_GAMEPAD_AXIS_RIGHT_TRIGGER] =
			(float)SDL_GetGamepadAxis(pad->sdl_gamepad, SDL_GAMEPAD_AXIS_RIGHT_TRIGGER) /
			32767.0F;

		if (pad->gyro_enabled)
		{
			float data[3];
			SDL_GetGamepadSensorData(pad->sdl_gamepad, SDL_SENSOR_GYRO, data, 3);
			pad->gyro = (spel_vec3){data[0], data[1], data[2]};
		}

		if (pad->accel_enabled)
		{
			float data[3];
			SDL_GetGamepadSensorData(pad->sdl_gamepad, SDL_SENSOR_ACCEL, data, 3);
			pad->accel = (spel_vec3){data[0], data[1], data[2]};
		}
	}
}

spel_hidden void spel_input_process_event(SDL_Event* event)
{
	// someone else wants our input, controller detection should still work
	// but no input should be forwarded

	bool captured = false;
	if (!spel_event_emit(SPEL_EVENT_INTERNAL_INPUT_SDL_EVENT, event))
	{
		captured = true;
	}

	switch ((SDL_EventType)event->type)
	{
	case SDL_EVENT_MOUSE_MOTION:
		if (captured)
		{
			break;
		}
		spel.input->mouse_pos = (spel_vec2){.x = event->motion.x, .y = event->motion.y};
		spel.input->mouse_delta =
			(spel_vec2){.x = event->motion.xrel, .y = event->motion.yrel};
		spel_event_emit(SPEL_EVENT_MOUSE_MOVE, &spel.input->mouse_delta);
		break;

	case SDL_EVENT_MOUSE_BUTTON_DOWN:
	{
		if (captured)
		{
			break;
		}
		spel.input->mouse_buttons[(spel_mouse_button)(event->button.button - 1)] = true;
		spel_mouse_button btn = (spel_mouse_button)(event->button.button - 1);
		spel_event_emit(SPEL_EVENT_MOUSE_BUTTON_DOWN, &btn);
		break;
	}

	case SDL_EVENT_MOUSE_BUTTON_UP:
	{
		if (captured)
		{
			break;
		}
		spel.input->mouse_buttons[(spel_mouse_button)(event->button.button - 1)] = false;
		spel_mouse_button btn = (spel_mouse_button)(event->button.button - 1);
		spel_event_emit(SPEL_EVENT_MOUSE_BUTTON_UP, &btn);
		break;
	}

	case SDL_EVENT_MOUSE_WHEEL:
		if (captured)
		{
			break;
		}
		spel.input->mouse_wheel = (spel_vec2){.x = event->wheel.x, .y = event->wheel.y};
		spel_event_emit(SPEL_EVENT_MOUSE_SCROLL, &spel.input->mouse_wheel);
		break;

	case SDL_EVENT_KEY_DOWN:
		if (captured)
		{
			break;
		}
		spel.input->keys[SPEL_KEY_WORD(event->key.scancode)] |=
			SPEL_KEY_MASK(event->key.scancode);
		spel_event_emit(SPEL_EVENT_KEY_DOWN, &event->key.scancode);
		break;

	case SDL_EVENT_KEY_UP:
		if (captured)
		{
			break;
		}
		spel.input->keys[SPEL_KEY_WORD(event->key.scancode)] &=
			~SPEL_KEY_MASK(event->key.scancode);
		spel_event_emit(SPEL_EVENT_KEY_UP, &event->key.scancode);
		break;

	case SDL_EVENT_TEXT_INPUT:
		if (captured)
		{
			break;
		}
		if (spel.input->text_input_enabled)
		{
			size_t event_len = strlen(event->text.text);
			size_t available =
				sizeof(spel.input->text_input) - spel.input->text_input_len - 1;

			if (event_len <= available)
			{
				strcat(spel.input->text_input, event->text.text);
				spel.input->text_input_len += event_len;

				spel_event_emit(SPEL_EVENT_TEXT_INPUT, &event_len);
			}
			else
			{
				spel_event_emit(SPEL_EVENT_TEXT_INPUT_TRUNCATED, spel.input->text_input);
			}
		}
		break;

	case SDL_EVENT_GAMEPAD_AXIS_MOTION:
		if (captured)
		{
			break;
		}
		spel_event_emit(SPEL_EVENT_GAMEPAD_AXIS_MOTION, &event->gaxis.axis);
		break;

	case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
	case SDL_EVENT_GAMEPAD_BUTTON_UP:
	{
		if (captured)
		{
			break;
		}

		int pad_id = -1;
		for (int i = 0; i < SPEL_MAX_CONTROLLERS; i++)
		{
			if (spel.input->gamepads[i]->sdl_gamepad &&
				SDL_GetGamepadID(spel.input->gamepads[i]->sdl_gamepad) ==
					event->gbutton.which)
			{
				pad_id = i;
				break;
			}
		}

		if (pad_id >= 0)
		{
			spel_gamepad_button btn = (uint32_t)event->gbutton.button;
			if (event->type == SDL_EVENT_GAMEPAD_BUTTON_DOWN)
			{
				spel.input->gamepads[pad_id]->buttons |= (1U << btn);
				spel_event_emit(SPEL_EVENT_GAMEPAD_BUTTON_DOWN, &btn);
			}
			else
			{
				spel.input->gamepads[pad_id]->buttons &= ~(1U << btn);
				spel_event_emit(SPEL_EVENT_GAMEPAD_BUTTON_UP, &btn);
			}
		}
		break;
	}

	case SDL_EVENT_GAMEPAD_ADDED:
		for (int i = 0; i < SPEL_MAX_CONTROLLERS; i++)
		{
			if (!spel.input->gamepads[i]->connected)
			{
				spel.input->gamepads[i]->sdl_gamepad =
					SDL_OpenGamepad(event->gdevice.which);
				spel.input->gamepads[i]->connected = true;

				spel.input->gamepads[i]->gyro_enabled = (SDL_GamepadHasSensor(
					spel.input->gamepads[i]->sdl_gamepad, SDL_SENSOR_GYRO));

				spel.input->gamepads[i]->accel_enabled = (SDL_GamepadHasSensor(
					spel.input->gamepads[i]->sdl_gamepad, SDL_SENSOR_ACCEL));

				spel.input->gamepads[i]->touchpad_enabled =
					SDL_GetNumGamepadTouchpads(spel.input->gamepads[i]->sdl_gamepad) != 0;

				spel_event_emit(SPEL_EVENT_GAMEPAD_ADDED, &i);
				break;
			}
		}
		break;

	case SDL_EVENT_GAMEPAD_REMOVED:
		for (int i = 0; i < SPEL_MAX_CONTROLLERS; i++)
		{
			if (spel.input->gamepads[i]->sdl_gamepad &&
				SDL_GetGamepadID(spel.input->gamepads[i]->sdl_gamepad) ==
					event->gdevice.which)
			{
				SDL_CloseGamepad(spel.input->gamepads[i]->sdl_gamepad);
				memset(spel.input->gamepads[i], 0, sizeof(spel_gamepad_t));
				spel_event_emit(SPEL_EVENT_GAMEPAD_REMOVED, &i);
				break;
			}
		}
		break;

	case SDL_EVENT_GAMEPAD_TOUCHPAD_DOWN:
	case SDL_EVENT_GAMEPAD_TOUCHPAD_MOTION:
	case SDL_EVENT_GAMEPAD_TOUCHPAD_UP:
	{
		if (captured)
		{
			break;
		}
		
		uint8_t pad_id = spel_find_gamepad_by_id(event->gtouchpad.which);
		if (pad_id == 255 || event->gtouchpad.finger >= SPEL_MAX_CONTROLLER_FINGERS)
		{
			break;
		}

		spel_touchpad_finger* finger =
			&spel.input->gamepads[pad_id]->touchpad[event->gtouchpad.finger];

		uint8_t finger_id = event->gtouchpad.finger;

		if (event->type == SDL_EVENT_GAMEPAD_TOUCHPAD_UP)
		{
			finger->active = false;
			finger->pos = (spel_vec2){event->gtouchpad.x, -event->gtouchpad.y};
			finger->pressure = 0.0F;
			spel_event_emit(SPEL_EVENT_GAMEPAD_TOUCHPAD_UP, &finger_id);
		}
		else
		{
			finger->active = true;
			finger->pos = (spel_vec2){event->gtouchpad.x, -event->gtouchpad.y};
			finger->pressure = event->gtouchpad.pressure;

			spel_event_emit(SPEL_EVENT_GAMEPAD_TOUCHPAD_MOTION, &finger_id);

			if (finger->pressure == 1)
			{
				spel_event_emit(SPEL_EVENT_GAMEPAD_TOUCHPAD_DOWN, &finger_id);
			}
		}
		break;
	}
	default:
		break;
	}
}

spel_hidden void spel_input_init()
{
	spel.input = spel_memory_malloc(sizeof(*spel.input), SPEL_MEM_TAG_CORE);

	spel.input->action_capacity = 4;
	spel.input->actions = spel_memory_malloc(
		sizeof(spel_action_t) * spel.input->action_capacity, SPEL_MEM_TAG_CORE);

	for (size_t i = 0; i < SPEL_MAX_CONTROLLERS; i++)
	{
		spel.input->gamepads[i] =
			spel_memory_malloc(sizeof(spel_gamepad_t), SPEL_MEM_TAG_CORE);
	}
}

spel_api bool spel_input_key(spel_key key)
{
	return (spel.input->keys[SPEL_KEY_WORD(key)] & SPEL_KEY_MASK(key)) != 0;
}

spel_api bool spel_input_key_pressed(spel_key key)
{
	return (spel.input->keys[SPEL_KEY_WORD(key)] & SPEL_KEY_MASK(key)) != 0 &&
		   (spel.input->keys_prev[SPEL_KEY_WORD(key)] & SPEL_KEY_MASK(key)) == 0;
}

spel_api bool spel_input_key_released(spel_key key)
{
	return (spel.input->keys[SPEL_KEY_WORD(key)] & SPEL_KEY_MASK(key)) == 0 &&
		   (spel.input->keys_prev[SPEL_KEY_WORD(key)] & SPEL_KEY_MASK(key)) != 0;
}

spel_api bool spel_input_key_shift()
{
	return spel_input_key(SPEL_KEY_LSHIFT) || spel_input_key(SPEL_KEY_RSHIFT);
}

spel_api bool spel_input_key_ctrl()
{
	return spel_input_key(SPEL_KEY_LCTRL) || spel_input_key(SPEL_KEY_RCTRL);
}

spel_api bool spel_input_key_alt()
{
	return spel_input_key(SPEL_KEY_LALT) || spel_input_key(SPEL_KEY_RALT);
}

spel_api void spel_input_text_start()
{
	SDL_StartTextInput(spel.window.handle);
	spel.input->text_input_enabled = true;
	spel_event_emit(SPEL_EVENT_TEXT_INPUT_START, NULL);
}

spel_api void spel_input_text_stop()
{
	SDL_StopTextInput(spel.window.handle);
	spel_input_text_clear();
	spel_event_emit(SPEL_EVENT_TEXT_INPUT_STOP, NULL);
}

spel_api bool spel_input_text_active()
{
	return spel.input->text_input_enabled;
}

spel_api const char* spel_input_text(uint8_t* size)
{
	if (size != NULL)
	{
		*size = spel.input->text_input_len;
	}
	return spel.input->text_input;
}

spel_api void spel_input_text_clear()
{
	spel.input->text_input_enabled = false;
	memset(spel.input->text_input, 0, 256);
}

spel_api spel_vec2 spel_input_mouse_pos()
{
	return spel.input->mouse_pos;
}

spel_api spel_vec2 spel_input_mouse_delta()
{
	return spel.input->mouse_delta;
}

spel_api spel_vec2 spel_input_mouse_wheel()
{
	return spel.input->mouse_wheel;
}

spel_api bool spel_input_mouse_button(spel_mouse_button btn)
{
	return spel.input->mouse_buttons[btn];
}

spel_api bool spel_input_mouse_pressed(spel_mouse_button btn)
{
	return spel.input->mouse_buttons[btn] && !spel.input->mouse_buttons_prev[btn];
}

spel_api bool spel_input_mouse_released(spel_mouse_button btn)
{
	return !spel.input->mouse_buttons[btn] && spel.input->mouse_buttons_prev[btn];
}

spel_api void spel_input_mouse_set_visible(bool visible)
{
	visible ? SDL_ShowCursor() : SDL_HideCursor();
}

spel_api void spel_input_mouse_set_locked(bool locked)
{
	SDL_SetWindowMouseGrab(spel.window.handle, locked);
}

spel_hidden spel_gamepad_id spel_find_gamepad_by_id(uint32_t sdlId)
{
	for (int i = 0; i < SPEL_MAX_CONTROLLERS; i++)
	{
		if (spel.input->gamepads[i]->sdl_gamepad &&
			SDL_GetGamepadID(spel.input->gamepads[i]->sdl_gamepad) == sdlId)
		{
			return i;
		}
	}

	return -1;
}

spel_api bool spel_input_gamepad_connected(spel_gamepad_id id)
{
	spel_input_ensure_gamepad();
	return spel.input->gamepads[id]->connected;
}

spel_api bool spel_input_gamepad_button(spel_gamepad_id id, spel_gamepad_button btn)
{
	if (id >= SPEL_MAX_CONTROLLERS || btn >= SPEL_GAMEPAD_BUTTON_COUNT)
	{
		return false;
	}
	spel_input_ensure_gamepad();
	return (spel.input->gamepads[id]->buttons & (1 << btn)) != 0;
}

spel_api bool spel_input_gamepad_button_pressed(spel_gamepad_id id,
											   spel_gamepad_button btn)
{
	if (id >= SPEL_MAX_CONTROLLERS || btn >= SPEL_GAMEPAD_BUTTON_COUNT)
	{
		return false;
	}
	spel_input_ensure_gamepad();
	uint32_t mask = 1U << btn;
	return (spel.input->gamepads[id]->buttons & mask) &&
		   !(spel.input->gamepads[id]->buttons_prev & mask);
}

spel_api bool spel_input_gamepad_button_released(spel_gamepad_id id,
												spel_gamepad_button btn)
{
	if (id >= SPEL_MAX_CONTROLLERS || btn >= SPEL_GAMEPAD_BUTTON_COUNT)
	{
		return false;
	}
	spel_input_ensure_gamepad();
	uint32_t mask = 1U << btn;
	return !(spel.input->gamepads[id]->buttons & mask) &&
		   (spel.input->gamepads[id]->buttons_prev & mask);
}

spel_api float spel_input_gamepad_axis(spel_gamepad_id id, spel_gamepad_axis axis)
{
	spel_input_ensure_gamepad();
	return spel.input->gamepads[id]->axes[axis];
}

spel_api spel_vec2 spel_input_gamepad_left_stick(spel_gamepad_id id)
{
	spel_input_ensure_gamepad();
	return (spel_vec2){.x = spel.input->gamepads[id]->axes[SPEL_GAMEPAD_AXIS_LEFT_X],
					   .y = spel.input->gamepads[id]->axes[SPEL_GAMEPAD_AXIS_LEFT_Y]};
}

spel_api spel_vec2 spel_input_gamepad_right_stick(spel_gamepad_id id)
{
	spel_input_ensure_gamepad();
	return (spel_vec2){.x = spel.input->gamepads[id]->axes[SPEL_GAMEPAD_AXIS_RIGHT_X],
					   .y = spel.input->gamepads[id]->axes[SPEL_GAMEPAD_AXIS_RIGHT_Y]};
}

spel_api float spel_input_gamepad_axis_deadzone(spel_gamepad_id id, spel_gamepad_axis axis,
											   float deadzone)
{
	spel_input_ensure_gamepad();
	float value = spel_input_gamepad_axis(id, axis);

	if (fabsf(value) < deadzone)
	{
		return 0.0F;
	}

	float sign = value < 0 ? -1.0F : 1.0F;
	return sign * (fabsf(value) - deadzone) / (1.0F - deadzone);
}

spel_api spel_vec2 spel_input_gamepad_left_stick_deadzone(spel_gamepad_id id,
														 float deadzone)
{
	spel_input_ensure_gamepad();
	spel_vec2 stick = spel_input_gamepad_left_stick(id);
	float magnitude = sqrtf((stick.x * stick.x) + (stick.y * stick.y));

	if (magnitude < deadzone)
	{
		return (spel_vec2){0, 0};
	}

	float scale = (magnitude - deadzone) / (1.0F - deadzone) / magnitude;
	return (spel_vec2){stick.x * scale, stick.y * scale};
}

spel_api spel_vec2 spel_input_gamepad_right_stick_deadzone(spel_gamepad_id id,
														  float deadzone)
{
	spel_input_ensure_gamepad();
	spel_vec2 stick = spel_input_gamepad_right_stick(id);
	float magnitude = sqrtf((stick.x * stick.x) + (stick.y * stick.y));

	if (magnitude < deadzone)
	{
		return (spel_vec2){0, 0};
	}

	float scale = (magnitude - deadzone) / (1.0F - deadzone) / magnitude;
	return (spel_vec2){stick.x * scale, stick.y * scale};
}

spel_api void spel_input_gamepad_rumble(spel_gamepad_id id, float lowFreq, float highFreq,
									   uint32_t durationMs)
{
	spel_input_ensure_gamepad();

	if (!spel_input_gamepad_connected(id))
	{
		return;
	}

	SDL_RumbleGamepad(spel.input->gamepads[id]->sdl_gamepad,
					  (uint16_t)(lowFreq * UINT16_MAX), (uint16_t)(highFreq * UINT16_MAX),
					  durationMs);
}

spel_api void spel_input_gamepad_rumble_triggers(spel_gamepad_id id, float left,
												float right, uint32_t durationMs)
{
	spel_input_ensure_gamepad();

	if (!spel_input_gamepad_connected(id))
	{
		return;
	}

	SDL_RumbleGamepadTriggers(spel.input->gamepads[id]->sdl_gamepad,
							  (uint16_t)(left * UINT16_MAX),
							  (uint16_t)(right * UINT16_MAX), durationMs);
}

spel_api void spel_input_gamepad_led(spel_gamepad_id id, spel_color color)
{
	spel_input_ensure_gamepad();

	if (!spel_input_gamepad_connected(id))
	{
		return;
	}

	float alpha = color.a / 255.0f;
	uint8_t final_r = (uint8_t)(color.r * alpha);
	uint8_t final_g = (uint8_t)(color.g * alpha);
	uint8_t final_b = (uint8_t)(color.b * alpha);

	SDL_SetGamepadLED(spel.input->gamepads[id]->sdl_gamepad, final_r, final_g, final_b);
}

spel_api bool spel_input_gamepad_has_gyro(spel_gamepad_id id)
{
	spel_input_ensure_gamepad();
	return spel.input->gamepads[id]->gyro_enabled;
}

spel_api bool spel_input_gamepad_has_accel(spel_gamepad_id id)
{
	spel_input_ensure_gamepad();
	return spel.input->gamepads[id]->accel_enabled;
}

spel_api spel_vec3 spel_input_gamepad_gyro(spel_gamepad_id id)
{
	spel_input_ensure_gamepad();
	return spel.input->gamepads[id]->gyro;
}

spel_api spel_vec3 spel_input_gamepad_accel(spel_gamepad_id id)
{
	spel_input_ensure_gamepad();
	return spel.input->gamepads[id]->accel;
}

spel_api bool spel_input_gamepad_has_touchpad(spel_gamepad_id id)
{
	spel_input_ensure_gamepad();
	return spel.input->gamepads[id]->touchpad_enabled;
}

spel_api spel_vec2 spel_input_gamepad_touchpad(spel_gamepad_id id, uint8_t finger)
{
	if (finger >= SPEL_MAX_CONTROLLER_FINGERS)
	{
		return (spel_vec2){.x = 0, .y = 0};
	}
	spel_input_ensure_gamepad();
	return spel.input->gamepads[id]->touchpad[finger].pos;
}

spel_api float spel_input_gamepad_touchpad_pressure(spel_gamepad_id id, uint8_t finger)
{
	if (finger >= SPEL_MAX_CONTROLLER_FINGERS)
	{
		return 0.0F;
	}
	spel_input_ensure_gamepad();
	return spel.input->gamepads[id]->touchpad[finger].pressure;
}

spel_api bool spel_input_gamepad_touchpad_active(spel_gamepad_id id, uint8_t finger)
{
	if (finger >= SPEL_MAX_CONTROLLER_FINGERS)
	{
		return false;
	}
	spel_input_ensure_gamepad();
	return spel.input->gamepads[id]->touchpad[finger].active;
}

spel_api spel_action spel_input_action_create(const char* name, spel_action_type type)
{
	if (spel.input->action_count >= spel.input->action_capacity)
	{
		spel.input->action_capacity *= 2;
		spel.input->actions = spel_memory_realloc(
			spel.input->actions, sizeof(spel_action_t) * spel.input->action_capacity,
			SPEL_MEM_TAG_CORE);
	}

	spel_action action = &spel.input->actions[spel.input->action_count++];
	action->type = type;
	strncpy(action->name, name, 48);
	return action;
}

spel_api void spel_input_action_bind_key(spel_action action, spel_key key)
{
	spel_action_binding* binding = &action->bindings[action->binding_count++];
	binding->type = SPEL_BINDING_KEY;
	binding->key = key;
}

spel_api void spel_input_action_bind_axis(spel_action action, spel_key neg, spel_key pos)
{
	spel_action_binding* binding = &action->bindings[action->binding_count++];
	binding->type = SPEL_BINDING_KEY_AXIS;
	binding->key_axis.negative = neg;
	binding->key_axis.positive = pos;
}

spel_api void spel_input_action_bind_gamepad_axis(spel_action action, spel_gamepad_id pad,
												 spel_gamepad_axis axis)
{
	spel_input_ensure_gamepad();
	spel_action_binding* binding = &action->bindings[action->binding_count++];
	binding->type = SPEL_BINDING_GAMEPAD_AXIS;
	binding->gamepad_axis.gamepad = pad;
	binding->gamepad_axis.axis = axis;
}

spel_api void spel_input_action_bind_mouse_button(spel_action action,
												 spel_mouse_button btn)
{
	spel_action_binding* binding = &action->bindings[action->binding_count++];
	binding->type = SPEL_BINDING_MOUSE_BUTTON;
	binding->mouse_button = btn;
}

spel_api void spel_input_action_bind_gamepad_button(spel_action action,
												   spel_gamepad_id pad,
												   spel_gamepad_button btn)
{
	spel_input_ensure_gamepad();
	spel_action_binding* binding = &action->bindings[action->binding_count++];
	binding->type = SPEL_BINDING_GAMEPAD_BUTTON;
	binding->gamepad_button.button = btn;
	binding->gamepad_button.gamepad = pad;
}

spel_api void spel_input_action_unbind_all(spel_action action)
{
	for (size_t i = 0; i < action->binding_count; i++)
	{
		action->bindings[i].type = SPEL_BINDING_NONE;
	}

	action->binding_count = 0;
}

spel_api bool spel_input_action(spel_action action)
{
	bool result = false;
	for (size_t i = 0; i < action->binding_count; i++)
	{
		bool binding_result = false;
		spel_action_binding* binding = &action->bindings[i];
		switch (binding->type)
		{
		case SPEL_BINDING_KEY:
			binding_result = spel_input_key(binding->key);
			break;

		case SPEL_BINDING_KEY_AXIS:
			binding_result = spel_input_key(binding->key_axis.negative) ||
							 spel_input_key(binding->key_axis.positive);
			break;

		case SPEL_BINDING_MOUSE_BUTTON:
			binding_result = spel_input_mouse_button(binding->mouse_button);
			break;

		case SPEL_BINDING_GAMEPAD_BUTTON:
			binding_result = spel_input_gamepad_button(binding->gamepad_button.gamepad,
													   binding->gamepad_button.button);
			break;

		case SPEL_BINDING_GAMEPAD_AXIS:
			binding_result = spel_input_gamepad_axis(binding->gamepad_axis.gamepad,
													 binding->gamepad_axis.axis) != 0;
			break;

		default:
			break;
		}

		if (binding_result && result == false)
		{
			result = binding_result;
		}
	}

	return result;
}

spel_api bool spel_input_action_pressed(spel_action action)
{
	bool result = false;

	for (size_t i = 0; i < action->binding_count; i++)
	{
		bool binding_result = false;
		spel_action_binding* binding = &action->bindings[i];
		switch (binding->type)
		{
		case SPEL_BINDING_KEY:
			binding_result = spel_input_key_pressed(binding->key);
			break;

		case SPEL_BINDING_MOUSE_BUTTON:
			binding_result = spel_input_mouse_pressed(binding->mouse_button);
			break;

		case SPEL_BINDING_GAMEPAD_BUTTON:
			binding_result = spel_input_gamepad_button_pressed(
				binding->gamepad_button.gamepad, binding->gamepad_button.button);
			break;

		default:
			break;
		}

		if (binding_result && result == false)
		{
			result = binding_result;
		}
	}

	return result;
}

spel_api bool spel_input_action_released(spel_action action)
{
	bool result = false;
	for (size_t i = 0; i < action->binding_count; i++)
	{
		bool binding_result = false;
		spel_action_binding* binding = &action->bindings[i];
		switch (binding->type)
		{
		case SPEL_BINDING_KEY:
			binding_result = spel_input_key_released(binding->key);
			break;

		case SPEL_BINDING_MOUSE_BUTTON:
			binding_result = spel_input_mouse_released(binding->mouse_button);
			break;

		case SPEL_BINDING_GAMEPAD_BUTTON:
			binding_result = spel_input_gamepad_button_released(
				binding->gamepad_button.gamepad, binding->gamepad_button.button);
			break;

		default:
			break;
		}

		if (binding_result && result == false)
		{
			result = binding_result;
		}
	}

	return result;
}

spel_api float spel_input_action_value(spel_action action)
{
	float result = 0;

	for (size_t i = 0; i < action->binding_count; i++)
	{
		float binding_result = 0;
		spel_action_binding* binding = &action->bindings[i];
		switch (binding->type)
		{
		case SPEL_BINDING_KEY:
			binding_result = spel_input_key(binding->key);
			break;

		case SPEL_BINDING_KEY_AXIS:
		{
			if (spel_input_key(binding->key_axis.negative))
			{
				binding_result = -1;
			}
			else if (spel_input_key(binding->key_axis.positive))
			{
				binding_result = 1;
			}
			else
			{
				binding_result = 0;
			}
			break;
		}

		case SPEL_BINDING_MOUSE_BUTTON:
			binding_result = spel_input_mouse_button(binding->mouse_button);
			break;

		case SPEL_BINDING_GAMEPAD_BUTTON:
			binding_result = spel_input_gamepad_button(binding->gamepad_button.gamepad,
													   binding->gamepad_button.button);
			break;

		case SPEL_BINDING_GAMEPAD_AXIS:
			binding_result = spel_input_gamepad_axis(binding->gamepad_axis.gamepad,
													 binding->gamepad_axis.axis);
			break;
		default:
			break;
		}

		if (result == 0 && binding_result != 0)
		{
			result = binding_result;
		}
	}

	return result;
}

spel_hidden void spel_input_ensure_gamepad()
{
	if (spel.input->gamepad_initialized)
	{
		return;
	}

	SDL_InitSubSystem(SDL_INIT_GAMEPAD | SDL_INIT_SENSOR);
	spel.input->gamepad_initialized = true;

	int num_gamepads = 0;
	SDL_JoystickID* gamepads = SDL_GetGamepads(&num_gamepads);
	if (gamepads)
	{
		for (int i = 0; i < num_gamepads && i < SPEL_MAX_CONTROLLERS; i++)
		{
			spel.input->gamepads[i]->sdl_gamepad = SDL_OpenGamepad(gamepads[i]);
			spel.input->gamepads[i]->connected = true;
		}
		SDL_free(gamepads);
	}
}
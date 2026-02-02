#include "core/event.h"
#include "SDL3/SDL_events.h"
#include "core/entry.h"
#include "core/log.h"
#include "core/macros.h"
#include "core/types.h"
#include "core/window.h"
#include <assert.h>
#include <string.h>

SDL_Event event;

sp_hidden spel_event_id spel_event_intern(const char* name)
{
	spel_string_intern* found = NULL;
	HASH_FIND_STR(spel.events.interns, name, found);

	if (found)
	{
		return found->id;
	}

	found = sp_malloc(sizeof(*found), SPEL_MEM_TAG_CORE);
	if (!found)
	{
		sp_error(SPEL_ERR_OOM, "failed to allocate intern entry");
		return (spel_event_id)-1;
	}

	size_t len = strlen(name) + 1;
	found->key = sp_malloc(len, SPEL_MEM_TAG_CORE);
	if (!found->key)
	{
		sp_error(SPEL_ERR_OOM, "failed to allocate interned string");
		sp_free(found);
		return (spel_event_id)-1;
	}

	memcpy((char*)found->key, name, len);
	found->id = spel.events.counter++;

	HASH_ADD_KEYPTR(hh, spel.events.interns, found->key, strlen(found->key), found);

	return found->id;
}

sp_hidden void spel_event_ensure_capacity(spel_event_id id)
{
	if (id < (spel_event_id)spel.events.capacity)
	{
		return;
	}

	int new_cap = (spel.events.capacity == 0) ? 32 : spel.events.capacity;
	while (new_cap <= id)
	{
		new_cap *= 2;
	}

	spel_event_list* temp =
		(spel_event_list*)realloc(spel.events.events, sizeof(spel_event_list) * new_cap);
	if (!temp)
	{
		sp_error(SPEL_ERR_OOM, "failed to grow events array");
		return;
	}
	spel.events.events = temp;

	for (int i = spel.events.capacity; i < new_cap; i++)
	{
		spel.events.events[i] = (spel_event_list){NULL};
	}

	spel.events.capacity = new_cap;
}

sp_api void spel_event_register(const char* name, spel_event_callback cb)
{
	spel_event_id id = spel_event_intern(name);
	if (id == (spel_event_id)-1)
	{
		return;
	}
	spel_event_ensure_capacity(id);

	spel_event_list* list = &spel.events.events[id];

	if (list->count == list->cap)
	{
		list->cap = (list->cap == 0) ? 4 : list->cap * 2;
		spel_event_callback* temp = (spel_event_callback*)realloc(
			(void*)list->callbacks, list->cap * sizeof(spel_event_callback));
		if (!temp)
		{
			sp_error(SPEL_ERR_OOM, "failed to grow callback list");
			return;
		}
		list->callbacks = temp;
	}

	list->callbacks[list->count] = cb;
	list->count++;
}

sp_api void spel_event_emit(const char* name, void* data)
{
	spel_event_id id = spel_event_intern(name);
	if (id == (spel_event_id)-1)
	{
		return;
	}

	if (id >= (spel_event_id)spel.events.capacity)
	{
		return;
	}

	spel_event_list* list = &spel.events.events[id];

	for (int i = 0; i < list->count; i++)
	{
		list->callbacks[i](data);
	}
}

sp_api void spel_event_poll()
{
	while ((int)SDL_PollEvent(&event) == 1)
	{
		spel_event_handle(&event);
	}
}

sp_hidden void spel_event_handle(void* event)
{
	SDL_Event* ev = (SDL_Event*)event;

	switch ((SDL_EventType)ev->type)
	{
	case SDL_EVENT_QUIT:
	case SDL_EVENT_TERMINATING:
	case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
		spel_window_close();
		break;

	case SDL_EVENT_LOW_MEMORY:
		sp_callback(spel_low_memory);
		break;

	case SDL_EVENT_WINDOW_RESIZED:
		spel.window.width = ev->window.data1;
		spel.window.height = ev->window.data2;
		break;

	case SDL_EVENT_WINDOW_MOVED:
		spel.window.x = ev->window.data1;
		spel.window.y = ev->window.data2;
		break;

	case SDL_EVENT_WINDOW_DISPLAY_CHANGED:
		spel.window.display = ev->window.data1;
		break;

	case SDL_EVENT_WILL_ENTER_BACKGROUND:
	case SDL_EVENT_DID_ENTER_BACKGROUND:
	case SDL_EVENT_WILL_ENTER_FOREGROUND:
	case SDL_EVENT_DID_ENTER_FOREGROUND:
	case SDL_EVENT_LOCALE_CHANGED:
	case SDL_EVENT_SYSTEM_THEME_CHANGED:
	case SDL_EVENT_DISPLAY_ORIENTATION:
	case SDL_EVENT_DISPLAY_ADDED:
	case SDL_EVENT_DISPLAY_REMOVED:
	case SDL_EVENT_DISPLAY_MOVED:
	case SDL_EVENT_DISPLAY_DESKTOP_MODE_CHANGED:
	case SDL_EVENT_DISPLAY_CURRENT_MODE_CHANGED:
	case SDL_EVENT_DISPLAY_CONTENT_SCALE_CHANGED:
	case SDL_EVENT_WINDOW_SHOWN:
	case SDL_EVENT_WINDOW_HIDDEN:
	case SDL_EVENT_WINDOW_EXPOSED:
	case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
	case SDL_EVENT_WINDOW_METAL_VIEW_RESIZED:
	case SDL_EVENT_WINDOW_MINIMIZED:
	case SDL_EVENT_WINDOW_MAXIMIZED:
	case SDL_EVENT_WINDOW_RESTORED:
	case SDL_EVENT_WINDOW_MOUSE_ENTER:
	case SDL_EVENT_WINDOW_MOUSE_LEAVE:
	case SDL_EVENT_WINDOW_FOCUS_GAINED:
	case SDL_EVENT_WINDOW_FOCUS_LOST:
	case SDL_EVENT_WINDOW_HIT_TEST:
	case SDL_EVENT_WINDOW_ICCPROF_CHANGED:
	case SDL_EVENT_WINDOW_DISPLAY_SCALE_CHANGED:
	case SDL_EVENT_WINDOW_SAFE_AREA_CHANGED:
	case SDL_EVENT_WINDOW_OCCLUDED:
	case SDL_EVENT_WINDOW_ENTER_FULLSCREEN:
	case SDL_EVENT_WINDOW_LEAVE_FULLSCREEN:
	case SDL_EVENT_WINDOW_DESTROYED:
	case SDL_EVENT_WINDOW_HDR_STATE_CHANGED:
	case SDL_EVENT_KEY_DOWN:
	case SDL_EVENT_KEY_UP:
	case SDL_EVENT_TEXT_EDITING:
	case SDL_EVENT_TEXT_INPUT:
	case SDL_EVENT_KEYMAP_CHANGED:
	case SDL_EVENT_KEYBOARD_ADDED:
	case SDL_EVENT_KEYBOARD_REMOVED:
	case SDL_EVENT_TEXT_EDITING_CANDIDATES:
	case SDL_EVENT_MOUSE_MOTION:
	case SDL_EVENT_MOUSE_BUTTON_DOWN:
	case SDL_EVENT_MOUSE_BUTTON_UP:
	case SDL_EVENT_MOUSE_WHEEL:
	case SDL_EVENT_MOUSE_ADDED:
	case SDL_EVENT_MOUSE_REMOVED:
	case SDL_EVENT_JOYSTICK_AXIS_MOTION:
	case SDL_EVENT_JOYSTICK_BALL_MOTION:
	case SDL_EVENT_JOYSTICK_HAT_MOTION:
	case SDL_EVENT_JOYSTICK_BUTTON_DOWN:
	case SDL_EVENT_JOYSTICK_BUTTON_UP:
	case SDL_EVENT_JOYSTICK_ADDED:
	case SDL_EVENT_JOYSTICK_REMOVED:
	case SDL_EVENT_JOYSTICK_BATTERY_UPDATED:
	case SDL_EVENT_JOYSTICK_UPDATE_COMPLETE:
	case SDL_EVENT_GAMEPAD_AXIS_MOTION:
	case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
	case SDL_EVENT_GAMEPAD_BUTTON_UP:
	case SDL_EVENT_GAMEPAD_ADDED:
	case SDL_EVENT_GAMEPAD_REMOVED:
	case SDL_EVENT_GAMEPAD_REMAPPED:
	case SDL_EVENT_GAMEPAD_TOUCHPAD_DOWN:
	case SDL_EVENT_GAMEPAD_TOUCHPAD_MOTION:
	case SDL_EVENT_GAMEPAD_TOUCHPAD_UP:
	case SDL_EVENT_GAMEPAD_SENSOR_UPDATE:
	case SDL_EVENT_GAMEPAD_UPDATE_COMPLETE:
	case SDL_EVENT_GAMEPAD_STEAM_HANDLE_UPDATED:
	case SDL_EVENT_FINGER_DOWN:
	case SDL_EVENT_FINGER_UP:
	case SDL_EVENT_FINGER_MOTION:
	case SDL_EVENT_FINGER_CANCELED:
	case SDL_EVENT_CLIPBOARD_UPDATE:
	case SDL_EVENT_DROP_FILE:
	case SDL_EVENT_DROP_TEXT:
	case SDL_EVENT_DROP_BEGIN:
	case SDL_EVENT_DROP_COMPLETE:
	case SDL_EVENT_DROP_POSITION:
	case SDL_EVENT_AUDIO_DEVICE_ADDED:
	case SDL_EVENT_AUDIO_DEVICE_REMOVED:
	case SDL_EVENT_AUDIO_DEVICE_FORMAT_CHANGED:
	case SDL_EVENT_SENSOR_UPDATE:
	case SDL_EVENT_PEN_PROXIMITY_IN:
	case SDL_EVENT_PEN_PROXIMITY_OUT:
	case SDL_EVENT_PEN_DOWN:
	case SDL_EVENT_PEN_UP:
	case SDL_EVENT_PEN_BUTTON_DOWN:
	case SDL_EVENT_PEN_BUTTON_UP:
	case SDL_EVENT_PEN_MOTION:
	case SDL_EVENT_PEN_AXIS:
	case SDL_EVENT_CAMERA_DEVICE_ADDED:
	case SDL_EVENT_CAMERA_DEVICE_REMOVED:
	case SDL_EVENT_CAMERA_DEVICE_APPROVED:
	case SDL_EVENT_CAMERA_DEVICE_DENIED:
	default:
		break;
	}
}

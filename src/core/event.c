#include "core/event.h"
#include "SDL3/SDL_events.h"
#include "SDL3/SDL_timer.h"
#include "core/entry.h"
#include "core/log.h"
#include "core/macros.h"
#include "core/types.h"
#include "core/window.h"
#include "gfx/gfx_internal.h"
#include "input/input_internal.h"
#include <assert.h>
#include <string.h>

static SDL_Event event;

static uint64_t hash_str(const char* s)
{
	uint64_t h = 1469598103934665603ULL;
	while (*s)
	{
		h ^= (unsigned char)*s++;
		h *= 1099511628211ULL;
	}
	return h;
}

static void intern_grow(struct spel_event_intern_table* interns)
{
	size_t new_cap = interns->capacity ? interns->capacity * 2 : 16;
	struct spel_event_intern_entry* new_entries = spel_memory_malloc(
		new_cap * sizeof(struct spel_event_intern_entry), SPEL_MEM_TAG_CORE);

	for (size_t i = 0; i < interns->capacity; ++i)
	{
		struct spel_event_intern_entry* e = &interns->entries[i];
		if (!e->key)
		{
			continue;
		}

		uint64_t h = hash_str(e->key);
		size_t idx = h % new_cap;

		while (new_entries[idx].key)
		{
			idx = (idx + 1) % new_cap;
		}
		new_entries[idx] = *e;
	}

	spel_memory_free(interns->entries);
	interns->entries = new_entries;
	interns->capacity = new_cap;
}

spel_api spel_event_id spel_event_intern(const char* name)
{
	spel_assert(name, "expected an event name");

	if (spel.events.interns.count * 2 >= spel.events.interns.capacity)
	{
		intern_grow(&spel.events.interns);
	}

	uint64_t h = hash_str(name);
	size_t idx = h % spel.events.interns.capacity;

	for (;;)
	{
		struct spel_event_intern_entry* e = &spel.events.interns.entries[idx];

		if (!e->key)
		{
			e->key = spel_memory_strdup(name, SPEL_MEM_TAG_CORE);
			e->id = spel.events.interns.next_id++;
			spel.events.interns.count++;
			return e->id;
		}

		if (strcmp(e->key, name) == 0)
		{
			return e->id;
		}

		idx = (idx + 1) % spel.events.interns.capacity;
	}
}

static void event_table_grow(spel_events* events)
{
	size_t new_cap = events->capacity ? events->capacity * 2 : 16;
	struct spel_event_bucket* new_buckets =
		spel_memory_malloc(new_cap * sizeof(struct spel_event_bucket), SPEL_MEM_TAG_CORE);

	for (size_t i = 0; i < events->capacity; ++i)
	{
		struct spel_event_bucket* b = &events->buckets[i];
		if (!b->id)
		{
			continue;
		}

		size_t idx = b->id % new_cap;
		while (new_buckets[idx].id)
		{
			idx = (idx + 1) % new_cap;
		}
		new_buckets[idx] = *b;
	}

	spel_memory_free(events->buckets);
	events->buckets = new_buckets;
	events->capacity = new_cap;
}

static struct spel_event_bucket* get_bucket(spel_events* events, spel_event_id id,
											int create)
{
	if (events->count * 2 >= events->capacity)
	{
		event_table_grow(events);
	}

	size_t idx = id % events->capacity;

	for (;;)
	{
		struct spel_event_bucket* b = &events->buckets[idx];

		if (!b->id)
		{
			if (!create)
			{
				return NULL;
			}
			b->id = id;
			events->count++;
			return b;
		}

		if (b->id == id)
		{
			return b;
		}

		idx = (idx + 1) % events->capacity;
	}
}

spel_api void spel_event_register(spel_event_id id, spel_event_callback cb, void* user)
{
	spel_assert(cb, "expected a valid function");

	struct spel_event_bucket* b = get_bucket(&spel.events, id, 1);

	if (b->count >= b->cap)
	{
		size_t new_cap = b->cap ? b->cap * 2 : 4;
		b->entries = spel_memory_realloc(
			b->entries, new_cap * sizeof(struct spel_event_entry), SPEL_MEM_TAG_CORE);
		b->cap = new_cap;
	}

	b->entries[b->count++] = (struct spel_event_entry){.callback = cb, .user = user};
}

spel_api bool spel_event_emit(spel_event_id id, void* data)
{
	struct spel_event_bucket* b = get_bucket(&spel.events, id, 0);
	if (!b)
	{
		return true;
	}

	for (size_t i = 0; i < b->count; ++i)
	{
		if (!b->entries[i].callback(data, b->entries[i].user))
		{
			return false;
		}
	}

	return true;
}

spel_hidden void spel_event_terminate()
{
	for (size_t i = 0; i < spel.events.interns.capacity; ++i)
	{
		spel_memory_free((void*)spel.events.interns.entries[i].key);
	}
	spel_memory_free(spel.events.interns.entries);

	for (size_t i = 0; i < spel.events.capacity; ++i)
	{
		spel_memory_free(spel.events.buckets[i].entries);
	}
	spel_memory_free(spel.events.buckets);
}

spel_api void spel_event_poll()
{
	while ((int)SDL_PollEvent(&event) == 1)
	{
		spel_event_handle(&event);
	}
}

spel_hidden void spel_event_handle(void* event)
{
	SDL_Event* ev = (SDL_Event*)event;

	if (ev->type == SDL_EVENT_QUIT || ev->type == SDL_EVENT_TERMINATING ||
		ev->type == SDL_EVENT_WINDOW_CLOSE_REQUESTED ||
		ev->type == SDL_EVENT_WINDOW_DESTROYED)
	{
		spel_event_emit(SPEL_EVENT_QUIT, NULL);
		spel_window_close();
	}

	if (!spel_event_emit(SPEL_EVENT_INTERNAL_SDL_EVENT, ev))
	{
		return;
	}

	switch ((SDL_EventType)ev->type)
	{
	case SDL_EVENT_LOW_MEMORY:
		spel_event_emit(SPEL_EVENT_MEMORY_LOW, NULL);
		spel_callback(spel.app.low_memory);
		break;

	case SDL_EVENT_WINDOW_RESIZED:
		spel.window.width = ev->window.data1;
		spel.window.height = ev->window.data2;
		// Some platforms only fire RESIZED; record desired drawable size (debounced
		// in frame_begin).
		if (spel.gfx)
		{
			spel.gfx->fb_width = ev->window.data1;
			spel.gfx->fb_height = ev->window.data2;
			spel.gfx->fb_resize_request_ms = SDL_GetTicks();
		}
		spel_event_emit(SPEL_EVENT_WINDOW_RESIZE,
						&(spel_vec2){.x = spel.window.width, .y = spel.window.height});
		break;

	case SDL_EVENT_WINDOW_MOVED:
		spel.window.x = ev->window.data1;
		spel.window.y = ev->window.data2;
		spel_event_emit(SPEL_EVENT_WINDOW_MOVE,
						&(spel_vec2){.x = spel.window.x, .y = spel.window.y});
		break;

	case SDL_EVENT_WINDOW_DISPLAY_CHANGED:
		spel.window.display = ev->window.data1;
		break;

	case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
		spel.gfx->fb_width = ev->window.data1;
		spel.gfx->fb_height = ev->window.data2;
		spel.gfx->fb_resize_request_ms = SDL_GetTicks();
		break;

	case SDL_EVENT_WINDOW_ENTER_FULLSCREEN:
		spel.window.fullscreen = true;
		spel_event_emit(SPEL_EVENT_WINDOW_FULLSCREEN_ENTER, NULL);
		break;

	case SDL_EVENT_WINDOW_LEAVE_FULLSCREEN:
		spel.window.fullscreen = false;
		spel_event_emit(SPEL_EVENT_WINDOW_FULLSCREEN_LEAVE, NULL);
		break;

	case SDL_EVENT_WINDOW_OCCLUDED:
	case SDL_EVENT_WINDOW_HIDDEN:
		spel.window.occluded = true;
		spel_event_emit(SPEL_EVENT_WINDOW_HIDDEN, NULL);
		break;

	case SDL_EVENT_WINDOW_SHOWN:
	case SDL_EVENT_WINDOW_EXPOSED:
		spel.window.occluded = false;
		spel_event_emit(SPEL_EVENT_WINDOW_SHOWN, NULL);
		break;

	case SDL_EVENT_WINDOW_FOCUS_GAINED:
		spel.window.focused = true;
		spel_event_emit(SPEL_EVENT_WINDOW_FOCUS_GAIN, NULL);
		break;

	case SDL_EVENT_WINDOW_FOCUS_LOST:
		spel.window.focused = false;
		spel_event_emit(SPEL_EVENT_WINDOW_FOCUS_LOST, NULL);
		break;

	case SDL_EVENT_WINDOW_MINIMIZED:
		spel_event_emit(SPEL_EVENT_WINDOW_MINIMIZED, NULL);
		break;

	case SDL_EVENT_WINDOW_MAXIMIZED:
		spel_event_emit(SPEL_EVENT_WINDOW_MAXIMIZED, NULL);
		break;

	case SDL_EVENT_WINDOW_MOUSE_ENTER:
		spel_event_emit(SPEL_EVENT_WINDOW_MOUSE_ENTER, NULL);
		break;

	case SDL_EVENT_WINDOW_MOUSE_LEAVE:
		spel_event_emit(SPEL_EVENT_WINDOW_MOUSE_LEAVE, NULL);
		break;

	case SDL_EVENT_WINDOW_SAFE_AREA_CHANGED:
		spel_event_emit(SPEL_EVENT_WINDOW_SAFEAREA_CHANGED, NULL);
		break;

	case SDL_EVENT_WILL_ENTER_BACKGROUND:
	case SDL_EVENT_DID_ENTER_BACKGROUND:
	case SDL_EVENT_WILL_ENTER_FOREGROUND:
	case SDL_EVENT_DID_ENTER_FOREGROUND:
	case SDL_EVENT_LOCALE_CHANGED:
	case SDL_EVENT_SYSTEM_THEME_CHANGED:
	case SDL_EVENT_DISPLAY_ORIENTATION:
	case SDL_EVENT_WINDOW_METAL_VIEW_RESIZED:
	case SDL_EVENT_WINDOW_RESTORED:
	case SDL_EVENT_WINDOW_HIT_TEST:
	case SDL_EVENT_WINDOW_ICCPROF_CHANGED:
	case SDL_EVENT_WINDOW_DISPLAY_SCALE_CHANGED:
	case SDL_EVENT_WINDOW_HDR_STATE_CHANGED:
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
		spel_input_process_event(ev);
		break;
	}
}

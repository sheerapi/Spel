#ifndef SPEL_EVENTS
#define SPEL_EVENTS
#include "core/macros.h"

typedef uint64_t spel_event_id;
typedef void (*spel_event_callback)(void* data, void* user);

/// event names -> FNV hash (using spel-str2fnv)
enum
{
	/// @event window:resize
	SPEL_EVENT_WINDOW_RESIZE = 0x8a5fd4319ec21eeb,

	/// @event window:move
	SPEL_EVENT_WINDOW_MOVE = 0x6742e2a6a4bbb53a,

	/// @event window:focus.gain
	SPEL_EVENT_WINDOW_FOCUS_GAIN = 0x55636524c6c799ea,

	/// @event window:focus.lost
	SPEL_EVENT_WINDOW_FOCUS_LOST = 0x375a075b7549950f,

	/// @event window:fullscreen.enter
	SPEL_EVENT_WINDOW_FULLSCREEN_ENTER = 0x99311e9992d24712,

	/// @event window:fullscreen.leave
	SPEL_EVENT_WINDOW_FULLSCREEN_LEAVE = 0xf4b6a0ab8c52ec7b,

	/// @event window:hidden
	SPEL_EVENT_WINDOW_HIDDEN = 0x8b435c82b8e83f73,

	/// @event window:shown
	SPEL_EVENT_WINDOW_SHOWN = 0x712cd3dc0dbe05f8,

	/// @event window:maximized
	SPEL_EVENT_WINDOW_MAXIMIZED = 0x3d403f77aeeddfa7,

	/// @event window:minimized
	SPEL_EVENT_WINDOW_MINIMIZED = 0xa527f9c91719a945,

	/// @event window:mouse.enter
	SPEL_EVENT_WINDOW_MOUSE_ENTER = 0x3b91ab60af3271de,

	/// @event window:mouse.leave
	SPEL_EVENT_WINDOW_MOUSE_LEAVE = 0x4831c6a5169d2df7,

	/// @event window:safe_area.changed
	SPEL_EVENT_WINDOW_SAFEAREA_CHANGED = 0xccc598323accfc4,

	/// @event quit
	SPEL_EVENT_QUIT = 0xff151b3e27410fec,

	/// @event memory.low
	SPEL_EVENT_MEMORY_LOW = 0x5dd1fd69be535e6c
};

struct spel_event_intern_entry
{
	char* key;
	spel_event_id id;
};

struct spel_event_intern_table
{
	struct spel_event_intern_entry* entries;
	size_t capacity;
	size_t count;
	spel_event_id next_id;
};

struct spel_event_entry
{
	spel_event_callback callback;
	void* user;
};

struct spel_event_bucket
{
	spel_event_id id;
	struct spel_event_entry* entries;
	size_t count;
	size_t cap;
};

sp_api spel_event_id spel_event_intern(const char* name);
sp_api void spel_event_register(spel_event_id id, spel_event_callback cb, void* user);
sp_api void spel_event_emit(spel_event_id id, void* data);

sp_api void spel_event_poll();

sp_hidden void spel_event_handle(void* event);
sp_hidden void spel_event_terminate();

#endif
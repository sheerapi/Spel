#ifndef SPEL_EVENTS
#define SPEL_EVENTS
#include "core/macros.h"

typedef uint64_t spel_event_id;
typedef bool (*spel_event_callback)(void* data, void* user);

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

	/// @event mouse:move
	SPEL_EVENT_MOUSE_MOVE = 0x9a6d5598b9a80931,

	/// @event mouse:button.down
	SPEL_EVENT_MOUSE_BUTTON_DOWN = 0xd56f381d491ae442,

	/// @event mouse:button.up
	SPEL_EVENT_MOUSE_BUTTON_UP = 0x61e8f97cb888dce7,

	/// @event mouse:scroll
	SPEL_EVENT_MOUSE_SCROLL = 0xfc0b10d8848c660d,

	/// @event key:down
	SPEL_EVENT_KEY_DOWN = 0xf93607b99926ed2c,

	/// @event text:input.start
	SPEL_EVENT_TEXT_INPUT_START = 0x610143e43a96937c,

	/// @event text:input.stop
	SPEL_EVENT_TEXT_INPUT_STOP = 0xc36e9d69475e3f58,

	/// @event text:input.truncated
	SPEL_EVENT_TEXT_INPUT_TRUNCATED = 0xc36e9d69475e3f58,

	/// @event text:input
	SPEL_EVENT_TEXT_INPUT = 0xdc9c934a5fa460ee,

	/// @event key:up
	SPEL_EVENT_KEY_UP = 0xceb08b77f07012ed,

	/// @event gamepad:added
	SPEL_EVENT_GAMEPAD_ADDED = 0x528fb53dbccb209e,

	/// @event gamepad:removed
	SPEL_EVENT_GAMEPAD_REMOVED = 0x3994cbbc720e9390,

	/// @event gamepad:axis.motion
	SPEL_EVENT_GAMEPAD_AXIS_MOTION = 0x1c6b0a76957a271b,

	/// @event gamepad:button.down
	SPEL_EVENT_GAMEPAD_BUTTON_DOWN = 0xc5c29c025d4b9ea2,

	/// @event gamepad:button.up
	SPEL_EVENT_GAMEPAD_BUTTON_UP = 0xe5c7996a48955e47,

	/// @event gamepad:touchpad.down
	SPEL_EVENT_GAMEPAD_TOUCHPAD_DOWN = 0xacd7eda598cbc8fe,

	/// @event gamepad:touchpad.up
	SPEL_EVENT_GAMEPAD_TOUCHPAD_UP = 0xc81c806fd683eccb,

	/// @event gamepad:touchpad.motion
	SPEL_EVENT_GAMEPAD_TOUCHPAD_MOTION = 0xbc7a3dfd6dc7faf4,

	/// @event internal:sdl_event
	SPEL_EVENT_INTERNAL_SDL_EVENT = 0x65c48b40829bd3ee,

	/// same as the other one, but only for input
	/// @event internal:input.sdl_event
	SPEL_EVENT_INTERNAL_INPUT_SDL_EVENT = 0x65c48b40829bd3ee,

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

spel_api spel_event_id spel_event_intern(const char* name);
spel_api void spel_event_register(spel_event_id id, spel_event_callback cb, void* user);

/// @returns true when every callback was called, execution should continue normally
/// @returns false when a callback requested halting callback execution
spel_api bool spel_event_emit(spel_event_id id, void* data);

spel_api void spel_event_poll();

spel_hidden void spel_event_handle(void* event);
spel_hidden void spel_event_terminate();

#endif
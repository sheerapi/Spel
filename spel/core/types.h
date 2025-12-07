#ifndef SPEL_TYPES
#define SPEL_TYPES
#include "core/macros.h"
#include <stdbool.h>

typedef struct
{
	bool borderless;
	bool resizable;
	bool fullscreen;
	bool useDpiScaling;
	bool running;

	int width;
	int height;

	int minWidth;
	int minHeight;

	int display;
	int x;
	int y;

	const char* title;
	void* handle;

	struct
	{
		int vsync;
		int depth;
		int stencil;
		int msaa;
	} swapchain;
} spel_window;

typedef struct spel_string_intern spel_string_intern;
typedef struct spel_event_list spel_event_list;
typedef uint32_t spel_event_id;

typedef struct
{
	spel_string_intern* interns;
	spel_event_list* events;

	spel_event_id counter;
	int capacity;
} spel_events;

typedef struct spel_context
{
	int argc;
	const char** argv;

	spel_window window;
	spel_events events;
} spel_context;

sp_api extern spel_context spel;

#endif
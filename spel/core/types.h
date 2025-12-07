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

typedef struct spel_context
{
	int argc;
	const char** argv;

	spel_window window;
} spel_context;

sp_api extern spel_context spel_ctx;

#endif
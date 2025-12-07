#include "core/entry.h"
#include "core/event.h"
#include "core/types.h"
#include "SDL3/SDL_init.h"
#include "core/window.h"

spel_context spel_ctx = {0};

int main(int argc, const char** argv)
{
	spel_ctx.argc = argc;
	spel_ctx.argv = argv;

	sp_callback(spel_conf);

	if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS))
	{
		spel_error("Failed to initialize windowing backend");
		return -1;
	}

	spel_window_create();
	sp_callback(spel_load);

	sp_callback(spel_run);
	sp_callback(spel_quit);

	spel_window_cleanup();
	
	return 0;
}

#ifndef _WIN32
sp_weak void spel_run()
{
	while (spel_window_running())
	{
		spel_event_poll();
		
		if (spel_update)
		{
			spel_update(0.0);
		}

		sp_callback(spel_draw);
	}
}
#endif
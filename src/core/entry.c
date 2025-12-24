#include "core/entry.h"
#include "SDL3/SDL_init.h"
#include "core/event.h"
#include "core/log.h"
#include "core/types.h"
#include "core/window.h"
#include "gfx/gfx.h"
#include "utils/time.h"
#include <stdlib.h>

spel_context spel = {.window = {.title = "Spël",
								.width = 800,
								.height = 600,
								.swapchain = {.vsync = 1},
								.resizable = true}};

int main(int argc, const char** argv)
{
	spel.argc = argc;
	spel.argv = argv;
	spel.window.swapchain.vsync = 1; // vsync on by default

#ifdef DEBUG
	spel.debug = true;
#endif

	if (spel_args_has("--debug"))
	{
		spel.debug = true;
	}

	sp_callback(spel_conf);

	if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS))
	{
		spel_error("Failed to initialize windowing backend");
		return -1;
	}

	spel_time_init(&spel.time);

	spel_gfx_context_conf(SPEL_GFX_BACKEND_OPENGL);
	spel_window_create();

	spel_gfx_context_desc gfx_desc;
	gfx_desc.backend = SPEL_GFX_BACKEND_OPENGL;
	gfx_desc.vsync = spel.window.swapchain.vsync;
	gfx_desc.debug = spel.debug;

	spel.gfx = spel_gfx_context_create(&gfx_desc);

	sp_callback(spel_load);

	sp_callback(spel_run);
	sp_callback(spel_quit);

	spel_gfx_context_destroy(spel.gfx);
	spel_window_cleanup();

	return 0;
}

#ifndef _WIN32
sp_weak void spel_run()
{
	while (spel_window_running())
	{
		spel_time_frame_begin(&spel.time);
		spel_event_poll();

		if (spel_update)
		{
			spel_update(spel.time.delta_unscaled);
		}

		spel_gfx_frame_begin(spel.gfx);
		sp_callback(spel_draw);
		spel_gfx_frame_present(spel.gfx);
	}
}

sp_weak void spel_error(const char* msg)
{
	log_error(msg);
	log_error("panic!");
	exit(-1);
}

void spel_assert_fail(const char* assertion, const char* msg, const char* file, int line,
					  const char* function)
{
	log_log(LOG_FATAL, file, line, "Assertion `%s` failed at %s: %s", assertion, function,
			msg);
}
#endif

bool spel_args_has(const char* arg)
{
	for (int i = 1; i < spel.argc; i++)
	{
		if (strcmp(spel.argv[i], arg) == 0)
		{
			return true;
		}
	}

	return false;
}
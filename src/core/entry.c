#include "core/entry.h"
#include "SDL3/SDL_init.h"
#include "core/event.h"
#include "core/types.h"
#include "core/window.h"
#include "gfx/gfx.h"
#include "utils/time.h"

spel_context spel = {
	.window = {.title = "Spël", .width = 800, .height = 600, .swapchain = {.vsync = 1}, .resizable = true}};

int main(int argc, const char** argv)
{
	spel.argc = argc;
	spel.argv = argv;

	sp_callback(spel_conf);

	if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS))
	{
		spel_error("Failed to initialize windowing backend");
		return -1;
	}

	spel_time_init(&spel.time);

	spel_gfx_context_conf(GFX_BACKEND_OPENGL);

	spel_window_create();
	spel.gfx = spel_gfx_context_create(GFX_BACKEND_OPENGL);

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
		spel_gfx_frame_end(spel.gfx);
	}
}
#endif
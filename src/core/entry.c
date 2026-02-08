#include "core/entry.h"
#include "SDL3/SDL_init.h"
#include "core/event.h"
#include "core/log.h"
#include "core/panic.h"
#include "core/types.h"
#include "core/window.h"
#include "gfx/gfx.h"
#include "gfx/gfx_types.h"
#include "utils/terminal.h"
#include "utils/time.h"
#include <stdlib.h>

sp_api spel_context spel = {
	.window = {.title = "Spël",
			   .width = 800,
			   .height = 600,
			   .borderless = false,
			   .swapchain = {.vsync = 1, .depth = 16, .stencil = 0, .msaa = 0},
			   .resizable = true}};

#ifdef SP_WEAK_LINK
int main(int argc, const char** argv)
{
	spel_app_desc app = (spel_app_desc){.argc = argc,
										.argv = argv,
										.gfx_backend = SPEL_GFX_BACKEND_OPENGL,
										.debug = sp_debug_build,
										.conf = spel_conf,
										.load = spel_load,
										.update = spel_update,
										.draw = spel_draw,
										.quit = spel_quit,
										.run = spel_run,
										.low_memory = spel_low_memory};
	return spel_app_run(&app);
}
#endif

#ifdef SP_WEAK_LINK
#	ifndef _WIN32
sp_weak void spel_run()
#	else
sp_hidden void spel_run_fallback()
#	endif
#else
sp_api void spel_run()
#endif
{
	while (spel_window_running())
	{
		spel_run_frame();
	}
}

sp_hidden void spel_run_frame()
{
	spel_time_frame_begin(&spel.time);
	spel_event_poll();

	if (spel.app.update)
	{
		spel.app.update(spel.time.delta_unscaled);
	}

	spel_gfx_frame_begin(spel.gfx);
	sp_callback(spel.app.draw);
	spel_gfx_frame_present(spel.gfx);
}

sp_api bool spel_args_has(const char* arg)
{
	for (int i = 1; i < spel.process.argc; i++)
	{
		if (strcmp(spel.process.argv[i], arg) == 0)
		{
			return true;
		}
	}

	return false;
}

sp_api int spel_app_run(spel_app_desc* app)
{
	spel_app_transform(app);

	spel.window.swapchain.vsync = 1; // vsync on by default
	spel.log.function = NULL;
	spel.log.severity = SPEL_SEV_TRACE;

	spel.window.x = 0;
	spel.window.y = 0;

	if (spel.process.argc > 0)
	{
		spel.process.name = spel.process.argv[0];
	}

	spel_memory_sdl_setup();

	spel_runtime_info_setup();
	spel_build_info_init();
	spel_log_stderr_install();
	spel_terminal_detect_ansi();

#ifdef DEBUG
	spel.env.debug = true;
#endif

	if (spel_args_has("--debug"))
	{
		spel.env.debug = true;
	}

	sp_callback(spel.app.conf);

	if (!SDL_Init(SDL_INIT_VIDEO))
	{
		sp_panic(SPEL_ERR_WINDOWING_FAILED, "failed to initialize windowing backend: %s",
				 SDL_GetError());
		return -1;
	}

	spel_time_init(&spel.time);

	spel_gfx_context_conf(spel.app.gfx_backend);
	spel_window_create();

	spel_gfx_context_desc gfx_desc;
	gfx_desc.backend = spel.app.gfx_backend;
	gfx_desc.vsync = spel.window.swapchain.vsync;
	gfx_desc.debug = spel.env.debug;

	spel.gfx = spel_gfx_context_create(&gfx_desc);

	sp_callback(spel.app.load);

	sp_callback(spel.app.run);
	sp_callback(spel.app.quit);

	spel_gfx_context_destroy(spel.gfx);
	spel_window_cleanup();
	spel_event_terminate();

	return 0;
}

sp_hidden void spel_app_transform(spel_app_desc* app)
{
	spel.process.argv = app->argv;
	spel.process.argc = app->argc;

	spel.env.debug = app->debug;
	spel.app.gfx_backend = app->gfx_backend;

	spel.app.conf = app->conf;
	spel.app.load = app->load;
	spel.app.run = app->run;
	spel.app.update = app->update;
	spel.app.draw = app->draw;
	spel.app.quit = app->quit;

	spel.app.low_memory = app->low_memory;

	if (spel.app.run == NULL)
	{
		spel.app.run = spel_run;
	}
}

sp_api spel_app_desc spel_app_desc_default()
{
	return (spel_app_desc){.argc = 0,
						   .argv = NULL,
						   .gfx_backend = SPEL_GFX_BACKEND_OPENGL,
						   .debug = sp_debug_build,
						   .conf = NULL,
						   .load = NULL,
						   .update = NULL,
						   .draw = NULL,
						   .quit = NULL,
						   .run = NULL,
						   .low_memory = NULL};
}
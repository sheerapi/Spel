#include "core/window.h"
#include "SDL3/SDL_video.h"
#include "core/panic.h"
#include "core/types.h"

sp_hidden void spel_window_create()
{
	uint64_t flags = SDL_WINDOW_OPENGL;

	if (spel.window.use_dpi_scaling)
	{
		flags |= SDL_WINDOW_HIGH_PIXEL_DENSITY;
	}

	spel.window.handle =
		SDL_CreateWindow(spel.window.title, spel.window.width, spel.window.height, flags);

	if (spel.window.handle == NULL)
	{
		sp_panic(SPEL_ERR_WINDOWING_FAILED, "failed to create a window: %s",
				 SDL_GetError());
		return;
	}

	if (SDL_strcmp(SDL_GetCurrentVideoDriver(), "wayland") != 0 &&
		(spel.window.x != 0 || spel.window.y != 0))
	{
		SDL_SetWindowPosition(spel.window.handle, spel.window.x, spel.window.y);
	}

	SDL_SetWindowFullscreen(spel.window.handle, spel.window.fullscreen);
	SDL_SetWindowResizable(spel.window.handle, spel.window.resizable);
	SDL_SetWindowBordered(spel.window.handle, spel.window.borderless);

	spel.window.display = SDL_GetDisplayForWindow(spel.window.handle);

	SDL_GetWindowPosition(spel.window.handle, &spel.window.x, &spel.window.y);

	sp_trace("created window %s (%dx%d) at %d,%d (display %d)", spel.window.title,
			 spel.window.width, spel.window.height, spel.window.x, spel.window.y,
			 spel.window.display);

	spel.window.running = true;
}

sp_api void spel_window_close()
{
	spel.window.running = false;
}

sp_hidden void spel_window_cleanup()
{
	SDL_DestroyWindow(spel.window.handle);
}

sp_api bool spel_window_running()
{
	return spel.window.running;
}

sp_api void spel_window_flash(spel_window_flash_mode mode)
{
	SDL_FlashWindow(spel.window.handle, (SDL_FlashOperation)mode);
}

sp_api void spel_window_resize(int width, int height)
{
	SDL_SetWindowSize(spel.window.handle, width, height);
}

sp_api void spel_window_move(int xpos, int ypos)
{
	SDL_SetWindowPosition(spel.window.handle, xpos, ypos);
}

sp_api void spel_window_rename(const char* name)
{
	SDL_SetWindowTitle(spel.window.handle, name);
	spel.window.title = name;
}

sp_api void spel_window_min_size_set(int width, int height)
{
	SDL_SetWindowMinimumSize(spel.window.handle, width, height);
	spel.window.min_width = width;
	spel.window.min_height = height;
}

sp_api void spel_window_fullscreen_set(bool fullscreen)
{
	if (SDL_SetWindowFullscreen(spel.window.handle, fullscreen))
	{
		spel.window.fullscreen = fullscreen;
	}
}

sp_api spel_vec2 spel_window_framebuffer_size()
{
	int w;
	int h;
	SDL_GetWindowSizeInPixels(spel.window.handle, &w, &h);
	return (spel_vec2){.x = (float)w, .y = (float)h};
}

sp_api spel_vec2 spel_window_dpi()
{
	spel_vec2 fb = spel_window_framebuffer_size();
	return (spel_vec2){.x = fb.x / (float)spel.window.width,
					   .y = fb.y / (float)spel.window.height};
}
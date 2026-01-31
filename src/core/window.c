#include "core/window.h"
#include "SDL3/SDL_video.h"
#include "core/panic.h"
#include "core/types.h"

void spel_window_create()
{
	spel.window.handle = SDL_CreateWindow(spel.window.title, spel.window.width,
										  spel.window.height, SDL_WINDOW_OPENGL);

	if (spel.window.handle == nullptr)
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

	spel.window.display = SDL_GetDisplayForWindow(spel.window.handle);

	SDL_GetWindowPosition(spel.window.handle, &spel.window.x, &spel.window.y);

	sp_trace("created window %s (%dx%d) at %d,%d (display %d)", spel.window.title, spel.window.width,
			 spel.window.height, spel.window.x, spel.window.y, spel.window.display);

	spel.window.running = true;
}

void spel_window_close()
{
	spel.window.running = false;
}

void spel_window_cleanup()
{
	SDL_DestroyWindow(spel.window.handle);
}

bool spel_window_running()
{
	return spel.window.running;
}

void spel_window_flash(spel_window_flash_mode mode)
{
	SDL_FlashWindow(spel.window.handle, (SDL_FlashOperation)mode);
}

void spel_window_resize(int width, int height)
{
	SDL_SetWindowSize(spel.window.handle, width, height);
}

void spel_window_move(int xpos, int ypos)
{
	SDL_SetWindowPosition(spel.window.handle, xpos, ypos);
}

void spel_window_rename(const char* name)
{
	SDL_SetWindowTitle(spel.window.handle, name);
	spel.window.title = name;
}

void spel_window_min_size_set(int width, int height)
{
	SDL_SetWindowMinimumSize(spel.window.handle, width, height);
}

void spel_window_fullscreen_set(bool fullscreen)
{
	SDL_SetWindowFullscreen(spel.window.handle, fullscreen);
}
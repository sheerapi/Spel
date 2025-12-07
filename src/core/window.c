#include "core/window.h"
#include "SDL3/SDL_video.h"
#include "core/types.h"

void spel_window_create()
{
	spel.window.handle = SDL_CreateWindow(spel.window.title, spel.window.width,
										  spel.window.height, SDL_WINDOW_OPENGL);

	if (SDL_strcmp(SDL_GetCurrentVideoDriver(), "wayland") != 0)
	{
		SDL_SetWindowPosition(spel.window.handle, spel.window.x, spel.window.y);
	}

	SDL_SetWindowFullscreen(spel.window.handle, spel.window.fullscreen);
	SDL_SetWindowResizable(spel.window.handle, spel.window.resizable);

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
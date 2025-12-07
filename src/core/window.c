#include "core/window.h"
#include "SDL3/SDL_video.h"
#include "core/types.h"

void spel_window_create()
{
	spel_ctx.window.handle =
		SDL_CreateWindow(spel_ctx.window.title, spel_ctx.window.width,
						 spel_ctx.window.height, SDL_WINDOW_OPENGL);

	if (SDL_strcmp(SDL_GetCurrentVideoDriver(), "wayland") != 0)
	{
		SDL_SetWindowPosition(spel_ctx.window.handle, spel_ctx.window.x,
							  spel_ctx.window.y);
	}

	SDL_SetWindowFullscreen(spel_ctx.window.handle, spel_ctx.window.fullscreen);
	SDL_SetWindowResizable(spel_ctx.window.handle, spel_ctx.window.resizable);

	spel_ctx.window.running = true;
}

void spel_window_close()
{
	spel_ctx.window.running = false;
}

void spel_window_cleanup()
{
	SDL_DestroyWindow(spel_ctx.window.handle);
}

bool spel_window_running()
{
	return spel_ctx.window.running;
}
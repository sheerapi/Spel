#include "utils/display.h"
#include "SDL3/SDL_video.h"
#include "core/memory.h"

sp_api spel_rect spel_display_bounds_get(uint32_t id)
{
	static SDL_Rect rect;
	SDL_GetDisplayBounds(id, &rect);
	return (spel_rect){.x = rect.x, .y = rect.y, .width = rect.w, .height = rect.h};
}

sp_api spel_rect spel_display_safe_bounds_get(uint32_t id)
{
	static SDL_Rect rect;
	SDL_GetDisplayUsableBounds(id, &rect);
	return (spel_rect){.x = rect.x, .y = rect.y, .width = rect.w, .height = rect.h};
}

const sp_api char* spel_display_name_get(uint32_t id)
{
	return SDL_GetDisplayName(id);
}

sp_api uint32_t* spel_displays_get(int32_t* count)
{
	return SDL_GetDisplays(count);
}

sp_api spel_display_orientation spel_display_orientation_current(uint32_t id)
{
	return (spel_display_orientation)SDL_GetCurrentDisplayOrientation(id);
}

sp_api spel_display_orientation spel_display_orientation_natural(uint32_t id)
{
	return (spel_display_orientation)SDL_GetNaturalDisplayOrientation(id);
}

sp_api spel_display_mode spel_display_mode_current(uint32_t id)
{
	const SDL_DisplayMode* mode = SDL_GetCurrentDisplayMode(id);

	return (spel_display_mode){.id = id,
							   .width = mode->w,
							   .height = mode->h,
							   .pixel_density = mode->pixel_density,
							   .refresh_rate = mode->refresh_rate};
}

sp_api spel_display_mode* spel_display_modes_get(uint32_t id, int* count)
{
	SDL_DisplayMode** modes = SDL_GetFullscreenDisplayModes(id, count);

	spel_display_mode* result =
		spel_memory_malloc(sizeof(spel_display_mode), SPEL_MEM_TAG_CORE);

	for (size_t i = 0; i < *count; i++)
	{
		SDL_DisplayMode* sdl = modes[i];
		result[i].width = sdl->w;
		result[i].height = sdl->h;
		result[i].pixel_density = sdl->pixel_density;
		result[i].id = sdl->displayID;
		result[i].refresh_rate = sdl->refresh_rate;

		SDL_free(sdl);
	}

	return result;
}

sp_api spel_display_mode spel_display_mode_nearest(uint32_t id, int w, int h,
												   float refreshRrate, bool hidpi)
{
	SDL_DisplayMode mode;
	SDL_GetClosestFullscreenDisplayMode(id, w, h, refreshRrate, hidpi, &mode);

	return (spel_display_mode){.id = mode.displayID,
							   .width = mode.w,
							   .height = mode.h,
							   .pixel_density = mode.pixel_density,
							   .refresh_rate = mode.refresh_rate};
}
#ifndef SPEL_WINDOW
#define SPEL_WINDOW
#include "core/macros.h"
#include "utils/math.h"

spel_hidden void spel_window_create();
spel_hidden void spel_window_cleanup();

spel_api void spel_window_close();
spel_api bool spel_window_running();

typedef enum
{
	SPEL_WINDOW_FLASH_CANCEL,
	SPEL_WINDOW_FLASH_BRIEF,
	SPEL_WINDOW_FLASH_FOCUS
} spel_window_flash_mode;

spel_api void spel_window_flash(spel_window_flash_mode mode);

spel_api void spel_window_resize(int width, int height);
spel_api void spel_window_move(int xpos, int ypos);
spel_api void spel_window_rename(const char* name);
spel_api void spel_window_min_size_set(int width, int height);
spel_api void spel_window_fullscreen_set(bool fullscreen);

spel_api spel_vec2 spel_window_framebuffer_size();
spel_api spel_vec2 spel_window_dpi();

#endif
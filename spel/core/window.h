#ifndef SPEL_WINDOW
#define SPEL_WINDOW
#include "core/macros.h"
#include "utils/math.h"

sp_hidden void spel_window_create();
sp_hidden void spel_window_cleanup();

sp_api void spel_window_close();
sp_api bool spel_window_running();

typedef enum
{
	SPEL_WINDOW_FLASH_CANCEL,
	SPEL_WINDOW_FLASH_BRIEF,
	SPEL_WINDOW_FLASH_FOCUS
} spel_window_flash_mode;

sp_api void spel_window_flash(spel_window_flash_mode mode);

sp_api void spel_window_resize(int width, int height);
sp_api void spel_window_move(int xpos, int ypos);
sp_api void spel_window_rename(const char* name);
sp_api void spel_window_min_size_set(int width, int height);
sp_api void spel_window_fullscreen_set(bool fullscreen);

sp_api spel_vec2 spel_window_framebuffer_size();
sp_api spel_vec2 spel_window_dpi();

#endif
#ifndef SPEL_WINDOW
#define SPEL_WINDOW
#include "core/macros.h"

sp_hidden void spel_window_create();
sp_hidden void spel_window_cleanup();

sp_api void spel_window_close();
sp_api bool spel_window_running();

#endif
#ifndef SPEL_DISPLAY
#define SPEL_DISPLAY
#include "utils/math.h"
#include "core/macros.h"

typedef struct
{
	uint32_t id;
	int width;
	int height;
	float pixel_density;
	float refresh_rate;
} spel_display_mode;

typedef enum
{
	SPEL_DISPLAY_UNKNOWN,
	SPEL_DISPLAY_LANDSCAPE,
	SPEL_DISPLAY_LANDSCAPE_FLIPPED,
	SPEL_DISPLAY_PORTRAIT,
	SPEL_DISPLAY_PORTRAIT_FLIPPED
} spel_display_orientation;

sp_api spel_rect spel_display_bounds_get(uint32_t id);
sp_api spel_rect spel_display_safe_bounds_get(uint32_t id);

const sp_api char* spel_display_name_get(uint32_t id);

sp_api spel_display_orientation spel_display_orientation_current(uint32_t id);
sp_api spel_display_orientation spel_display_orientation_natural(uint32_t id);

sp_api spel_display_mode spel_display_mode_current(uint32_t id);
sp_api spel_display_mode spel_display_mode_nearest(uint32_t id, int w, int h, float refreshRrate, bool hidpi);
sp_api spel_display_mode* spel_display_modes_get(uint32_t id, int* count);

sp_api uint32_t* spel_displays_get(int32_t* count);

#endif
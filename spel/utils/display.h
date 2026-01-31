#ifndef SPEL_DISPLAY
#define SPEL_DISPLAY
#include "utils/math.h"
#include "core/macros.h"

typedef struct
{
	unsigned int id;
	int width;
	int height;
	float pixel_density;
	float refresh_rate;
} spel_display_mode;

typedef enum
{
	SPEL_DISPLAY_LANDSCAPE,
	SPEL_DISPLAY_LANDSCAPE_FLIPPED,
	SPEL_DISPLAY_PORTRAIT,
	SPEL_DISPLAY_PORTRAIT_FLIPPED
} spel_display_orientation;

sp_api spel_vec2 spel_display_bounds_get(unsigned int id);
sp_api spel_vec2 spel_display_safe_bounds_get(unsigned int id);

const sp_api char* spel_display_name_get(unsigned int id);

#endif
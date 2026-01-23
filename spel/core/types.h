#ifndef SPEL_TYPES
#define SPEL_TYPES
#include "core/build_info.h"
#include "core/macros.h"
#include "core/memory.h"
#include "gfx/gfx_types.h"
#include <stdbool.h>

typedef struct
{
	bool borderless;
	bool resizable;
	bool fullscreen;
	bool use_dpi_scaling;
	bool running;

	int width;
	int height;

	int min_width;
	int min_height;

	int display;
	int x;
	int y;

	const char* title;
	void* handle;

	struct
	{
		int vsync;
		int depth;
		int stencil;
		int msaa;
	} swapchain;
} spel_window;

typedef struct spel_string_intern spel_string_intern;
typedef struct spel_event_list spel_event_list;
typedef uint32_t spel_event_id;

typedef struct
{
	spel_string_intern* interns;
	spel_event_list* events;

	spel_event_id counter;
	int capacity;
} spel_events;

typedef struct spel_time
{
	double delta;
	double delta_unscaled;
	double delta_smoothed;
	double time;

	uint64_t frame_index;

	double fps;
	double fps_raw;

	double time_scale;
	double fixed_dt;
	double accumulator;

	uint64_t stamp_now;
	uint64_t stamp_last;
} spel_time;

enum
{
	SPEL_SEV_TRACE,
	SPEL_SEV_DEBUG,
	SPEL_SEV_INFO,
	SPEL_SEV_WARN,
	SPEL_SEV_ERROR,
	SPEL_SEV_FATAL,
};

typedef uint8_t spel_severity;
typedef struct spel_log_event_t* spel_log_event;
typedef void (*spel_log_fn)(spel_log_event evt, void* user);

typedef enum
{
	SPEL_PANIC_ABORT,
	SPEL_PANIC_TRAP_THEN_ABORT,
	SPEL_PANIC_EXIT_FAST,
} spel_panic_mode;

typedef struct spel_log
{
	spel_severity severity;
	spel_log_fn function;
	void* user;
} spel_log;

typedef struct spel_context
{
	int argc;
	const char** argv;
	bool debug;
	bool terminal_color;
	spel_panic_mode panic_mode;

	spel_window window;
	spel_events events;
	spel_time time;
	spel_gfx_context gfx;
	spel_memory memory;
	spel_log log;
	spel_build_info build_info;
} spel_context;

sp_api extern spel_context spel;

#endif
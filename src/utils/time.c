#include "utils/time.h"
#include "SDL3/SDL_timer.h"
#include <time.h>

static const double SMOOTH_ALPHA = 0.12;

void spel_time_init(spel_time* t)
{
	*t = (spel_time){.delta = 0.0,
					 .delta_unscaled = 0.0,
					 .delta_smoothed = 0.016, // assume 60 FPS initial
					 .time = 0,
					 .frame_index = 0,
					 .fps = 60.0,
					 .fps_raw = 60.0,
					 .time_scale = 1.0,
					 .fixed_dt = 1.0 / 60.0,
					 .accumulator = 0.0,
					 .stamp_last = 0,
					 .stamp_now = 0};
}

void spel_time_frame_begin(spel_time* t)
{
	if (t->stamp_now == 0)
	{
		spel.time.stamp_now = SDL_GetPerformanceCounter();
		spel.time.stamp_last = spel.time.stamp_now;
		spel.time.delta_unscaled = 0.0;
		spel.time.delta = 0.0;
	}

	t->stamp_last = t->stamp_now;
	t->stamp_now = SDL_GetPerformanceCounter();

	t->delta_unscaled =
		(double)(t->stamp_now - t->stamp_last) / (double)SDL_GetPerformanceFrequency();

	t->delta = t->delta_unscaled * t->time_scale;

	t->time += t->delta;

	t->frame_index++;

	t->delta_smoothed =
		(t->delta_smoothed * (1.0 - SMOOTH_ALPHA)) + (t->delta * SMOOTH_ALPHA);

	t->fps_raw = (t->delta_unscaled > 0.0) ? (1.0 / t->delta_unscaled) : 0.0;
	t->fps = (t->delta_smoothed > 0.0) ? (1.0 / t->delta_smoothed) : 0.0;

	t->accumulator += t->delta;
}

uint64_t spel_time_now_ns()
{
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return ((uint64_t)ts.tv_sec * 1000000000ULL) + ts.tv_nsec;
}

sp_api time_t spel_time_now_sec()
{
	struct timespec ts;
	clock_gettime(CLOCK_REALTIME, &ts);
	return ts.tv_sec;
}
#ifndef SPEL_TIME
#define SPEL_TIME
#include "core/types.h"
#include <time.h>
#include "core/macros.h"

sp_hidden void spel_time_init(spel_time* t);
sp_hidden void spel_time_frame_begin(spel_time* t);
sp_api uint64_t spel_time_now_ns();
sp_api time_t spel_time_now_sec();

#endif
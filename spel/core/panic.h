#ifndef SPEL_PANIC
#define SPEL_PANIC
#include "types.h"
#include "log.h"
#include "macros.h"

sp_api _Noreturn void spel_panic(spel_log_event evt);

#define sp_panic(err, msg, ...)                                                          \
	spel_panic(spel_log_fmt(&(spel_log_event_t){.severity = (SPEL_SEV_FATAL),            \
												.code = (err),                           \
												.message = nullptr,                      \
												.length = 0,                             \
												.file = __FILE__,                        \
												.line = __LINE__,                        \
												.data = nullptr,                         \
												.data_type = SPEL_DATA_NONE,             \
												.data_size = 0},                         \
							msg, ##__VA_ARGS__))

#endif
#ifndef SPEL_EVENTS
#	define SPEL_EVENTS
#include "core/types.h"
#include "utils/uthash.h"
#	include "core/macros.h"

typedef struct spel_string_intern
{
	const char* key;
	spel_event_id id;
	UT_hash_handle hh;
} spel_string_intern;

typedef void (*spel_event_callback)(void* data);

typedef struct spel_event_list
{
	spel_event_callback* callbacks;
	int count;
	int cap;
} spel_event_list;

sp_hidden spel_event_id spel_event_intern(const char* name);
sp_hidden void spel_event_ensure_capacity(spel_event_id id);

sp_api void spel_event_register(const char* name, spel_event_callback cb);
sp_api void spel_event_emit(const char* name, void* data);

sp_api void spel_event_poll();
sp_hidden void spel_event_handle(void* event);

#endif
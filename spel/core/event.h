#ifndef SPEL_EVENTS
#define SPEL_EVENTS
#include "core/macros.h"

typedef uint64_t spel_event_id;
typedef void (*spel_event_callback)(void* data, void* user);

sp_hidden static struct spel_event_intern_entry
{
	char* key;
	spel_event_id id;
} spel_event_intern_entry;

sp_hidden static struct spel_event_intern_table
{
	struct spel_event_intern_entry* entries;
	size_t capacity;
	size_t count;
	spel_event_id next_id;
} spel_event_intern_table;

sp_hidden static struct spel_event_entry
{
	spel_event_callback callback;
	void* user;
} spel_event_entry;

sp_hidden static struct spel_event_bucket
{
	spel_event_id id;
	struct spel_event_entry* entries;
	size_t count;
	size_t cap;
} spel_event_bucket;

sp_api spel_event_id spel_event_intern(const char* name);
sp_api void spel_event_register(spel_event_id id, spel_event_callback cb, void* user);
sp_api void spel_event_emit(spel_event_id id, void* data);

sp_api void spel_event_poll();

sp_hidden void spel_event_handle(void* event);
sp_hidden void spel_event_terminate();

#endif
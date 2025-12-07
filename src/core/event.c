#include "core/event.h"
#include "core/types.h"
#include <assert.h>

spel_event_id spel_event_intern(const char* name)
{
	spel_string_intern* found = nullptr;
	HASH_FIND_STR(spel_ctx.events.interns, name, found);

	if (found)
	{
		return found->id;
	}

	found = malloc(sizeof(*found));
	found->key = strdup(name);
	found->id = spel_ctx.events.counter++;

	HASH_ADD_KEYPTR(hh, spel_ctx.events.interns, found->key, strlen(found->key), found);

	return found->id;
}

void spel_event_ensure_capacity(spel_event_id id)
{
	if (id < (spel_event_id)spel_ctx.events.capacity)
	{
		return;
	}

	int new_cap = (spel_ctx.events.capacity == 0) ? 32 : spel_ctx.events.capacity;
	while (new_cap <= id)
	{
		new_cap *= 2;
	}

	spel_event_list* temp = (spel_event_list*)realloc(spel_ctx.events.events,
													  sizeof(spel_event_list) * new_cap);
	assert(temp != nullptr);
	spel_ctx.events.events = temp;

	for (int i = spel_ctx.events.capacity; i < new_cap; i++)
	{
		spel_ctx.events.events[i] = (spel_event_list){nullptr};
	}

	spel_ctx.events.capacity = new_cap;
}

void spel_on(const char* name, spel_event_callback cb)
{
	spel_event_id id = spel_event_intern(name);
	spel_event_ensure_capacity(id);

	spel_event_list* list = &spel_ctx.events.events[id];

	if (list->count == list->cap)
	{
		list->cap = (list->cap == 0) ? 4 : list->cap * 2;
		spel_event_callback* temp = (spel_event_callback*)realloc(
			(void*)list->callbacks, list->cap * sizeof(spel_event_callback));
		assert(temp != nullptr);
		list->callbacks = temp;
	}

	list->callbacks[list->count] = cb;
	list->count++;
}

void spel_emit(const char* name, void* data)
{
	spel_event_id id = spel_event_intern(name);

	if (id >= (spel_event_id)spel_ctx.events.capacity)
	{
		return;
	}

	spel_event_list* list = &spel_ctx.events.events[id];

	for (int i = 0; i < list->count; i++)
	{
		list->callbacks[i](data);
	}
}
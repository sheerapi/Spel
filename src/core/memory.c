#include "core/memory.h"
#include "core/types.h"

typedef struct spel_alloc_header
{
	size_t size;
	spel_memory_tag tag;
} spel_alloc_header;

void* spel_malloc(size_t size, spel_memory_tag tag)
{
	size_t total = sizeof(spel_alloc_header) + size;
	spel_alloc_header* h = malloc(total);
	if (!h)
	{
		return NULL;
	}

	h->size = size;
	h->tag = tag;

	spel.memory.current += size;
	spel.memory.total_allocated += size;
	spel.memory.alloc_count++;

	if (spel.memory.current > spel.memory.peak)
	{
		spel.memory.peak = spel.memory.current;
	}

	spel_memory_tag_stats* ts = &spel.memory.tags[tag];
	ts->bytes_current += size;
	ts->alloc_count++;

	if (ts->bytes_current > ts->bytes_peak)
	{
		ts->bytes_peak = ts->bytes_current;
	}

	return (void*)(h + 1);
}

void spel_free(void* ptr)
{
	if (!ptr)
	{
		return;
	}

	spel_alloc_header* h = ((spel_alloc_header*)ptr) - 1;
	size_t size = h->size;
	spel_memory_tag tag = h->tag;

	spel.memory.current -= size;
	spel.memory.total_freed += size;
	spel.memory.free_count++;

	spel.memory.tags[tag].bytes_current -= size;

	free(h);
}

void* spel_realloc(void* ptr, size_t newSize, spel_memory_tag tag)
{

	if (!ptr)
	{
		return spel_malloc(newSize, tag);
	}

	if (newSize == 0)
	{
		spel_free(ptr);
		return NULL;
	}

	spel_alloc_header* old_h = ((spel_alloc_header*)ptr) - 1;
	size_t old_size = old_h->size;
	spel_memory_tag old_tag = old_h->tag;

	spel_memory_tag use_tag = (tag == SPEL_MEM_TAG_TEMP) ? old_tag : tag;

	size_t total = sizeof(spel_alloc_header) + newSize;

	spel_alloc_header* new_h = realloc(old_h, total);
	if (!new_h)
	{
		return NULL;
	}

	if (newSize > old_size)
	{
		size_t delta = newSize - old_size;

		spel.memory.current += delta;
		spel.memory.total_allocated += delta;

		if (spel.memory.current > spel.memory.peak)
		{
			spel.memory.peak = spel.memory.current;
		}

		spel.memory.tags[use_tag].bytes_current += delta;
		if (spel.memory.tags[use_tag].bytes_current >
			spel.memory.tags[use_tag].bytes_peak)
		{
			spel.memory.tags[use_tag].bytes_peak =
				spel.memory.tags[use_tag].bytes_current;
		}
	}
	else if (newSize < old_size)
	{
		size_t delta = old_size - newSize;

		spel.memory.current -= delta;
		spel.memory.total_freed += delta;

		spel.memory.tags[use_tag].bytes_current -= delta;
	}

	new_h->size = newSize;
	new_h->tag = use_tag;

	return (void*)(new_h + 1);
}